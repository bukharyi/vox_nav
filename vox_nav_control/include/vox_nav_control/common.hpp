// Copyright (c) 2022 Fetullah Atas, Norwegian University of Life Sciences
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef VOX_NAV_CONTROL__COMMON_HPP_
#define VOX_NAV_CONTROL__COMMON_HPP_

#include <rclcpp/rclcpp.hpp>
#include <visualization_msgs/msg/marker_array.hpp>
#include <std_msgs/msg/color_rgba.hpp>
#include <nav_msgs/msg/path.hpp>

#include <Eigen/Eigen>
#include <ompl/geometric/PathGeometric.h>
#include <ompl/base/SpaceInformation.h>
#include <ompl/base/spaces/SE2StateSpace.h>
#include <ompl/base/spaces/DubinsStateSpace.h>
#include <ompl/base/spaces/ReedsSheppStateSpace.h>
#include <ompl/base/ScopedState.h>

#include <vox_nav_utilities/tf_helpers.hpp>

namespace vox_nav_control
{
  namespace common
  {

    enum STATE_ENUM
    {
      kX = 0,
      kY = 1,
      kPsi = 2,
      kV = 3,
    };

    enum INPUT_ENUM
    {
      kacc = 0,
      kdf = 1,
    };

    /**
       * @brief We model obstacles with ellipsoids
       * They are then used in opti stack
       *
       */
    struct Ellipsoid
    {
      Eigen::Vector2f center;
      Eigen::Vector2f axes;
      bool is_dynamic;
      double heading;
    };

    /**
     * @brief a struct to keep states organized
     *
     */
    struct States
    {
      double x;   // X position
      double y;   // Y position
      double psi;   // heading angle
      double v;   // linear velocity
      States()
      : x(0.0),
        y(0.0),
        psi(0.0),
        v(0.0) {}
    };

    /**
     * @brief a struct to keep control inputs organized
     *
     */
    struct ControlInput
    {
      double acc;   // accelration command
      double df;   // steering angle command
      ControlInput()
      : acc(0.0),
        df(0.0) {}
    };

    /**
   * @brief all parameters used by MPC class,
   * user needs to create and reconfigure this
   *
   */
    struct Parameters
    {
      // timesteps in MPC Horizon
      int N;
      // discretization time between timesteps(s)
      double DT;
      // distance from CoG to front axle(m)
      double L_F;
      // distance from CoG to rear axle(m)
      double L_R;
      // min / max velocity constraint(m / s)
      double V_MIN;
      double V_MAX;
      // min / max acceleration constraint(m / s ^ 2)
      double A_MIN;
      double A_MAX;
      // min / max front steer angle constraint(rad)
      double DF_MIN;
      double DF_MAX;
      // min / max jerk constraint(m / s ^ 3)
      double A_DOT_MIN;
      double A_DOT_MAX;
      // min / max front steer angle rate constraint(rad / s)
      double DF_DOT_MIN;
      double DF_DOT_MAX;
      // weights on x, y, psi, and v.
      std::vector<double> Q;
      // weights on jerk and skew rate(steering angle derivative)
      std::vector<double> R;
      // enable/disable debug messages
      bool debug_mode;
      // set this true only if user figured the configration
      bool params_configured;

      // Assign meaningful default values to this parameters
      Parameters()
      : N(10),
        DT(0.1),
        L_F(0.65),
        L_R(0.65),
        V_MIN(-10.0),
        V_MAX(10.0),
        A_MIN(-1.0),
        A_MAX(1.0),
        DF_MIN(-0.5),
        DF_MAX(0.5),
        A_DOT_MIN(-1.0),
        A_DOT_MAX(1.0),
        DF_DOT_MIN(-0.5),
        DF_DOT_MAX(0.5),
        Q({100.0, 100.0, 10.0, 0.1}),
        R({10.0, 10.0}),
        debug_mode(true),
        params_configured(false) {}
    };

    static int nearestStateIndex(
      const nav_msgs::msg::Path & reference_traj,
      const geometry_msgs::msg::PoseStamped & curr_robot_pose)
    {
      int closest_state_index = -1;
      int closest_state_distance = 10000.0;
      for (int i = 0; i < reference_traj.poses.size(); i++) {

        double curr_distance =
          vox_nav_utilities::getEuclidianDistBetweenPoses(reference_traj.poses[i], curr_robot_pose);

        if (curr_distance < closest_state_distance) {
          closest_state_distance = curr_distance;
          closest_state_index = i;
        }
      }
      return closest_state_index;
    }

    static std::vector<States> getLocalInterpolatedReferenceStates(
      const geometry_msgs::msg::PoseStamped & curr_robot_pose,
      const Parameters & mpc_parameters,
      const nav_msgs::msg::Path & reference_traj,
      const double global_plan_look_ahead_distance)
    {
      double robot_roll, robot_pitch, robot_yaw;
      geometry_msgs::msg::PoseStamped front_axle_pose;
      vox_nav_utilities::getRPYfromMsgQuaternion(
        curr_robot_pose.pose.orientation, robot_roll, robot_pitch, robot_yaw);

      front_axle_pose.pose.position.x =
        curr_robot_pose.pose.position.x + 1.5 * mpc_parameters.L_F * std::cos(robot_yaw);
      front_axle_pose.pose.position.y =
        curr_robot_pose.pose.position.y + 1.5 * mpc_parameters.L_F * std::sin(robot_yaw);
      front_axle_pose.pose.position.z = 0;

      double kTARGETSPEED = 1.0;
      // Now lets find nearest trajectory point to robot base
      int nearsest_traj_state_index = nearestStateIndex(reference_traj, front_axle_pose);

      // Auto calculate interpolation steps
      double path_euclidian_length = 0.0;
      for (size_t i = 1; i < reference_traj.poses.size(); i++) {
        path_euclidian_length += vox_nav_utilities::getEuclidianDistBetweenPoses(
          reference_traj.poses[i], reference_traj.poses[i - 1]);
      }

      double interpolation_step_size = path_euclidian_length / reference_traj.poses.size();
      int states_to_see_horizon = global_plan_look_ahead_distance / interpolation_step_size;
      int local_goal_state_index = nearsest_traj_state_index + states_to_see_horizon;
      if (local_goal_state_index >= reference_traj.poses.size() - 1) {
        local_goal_state_index = reference_traj.poses.size() - 1;
      }
      // Define a state space, we basically need this only because we want to use OMPL's
      // geometric path, And then we can interpolate this path
      std::shared_ptr<ompl::base::RealVectorBounds> state_space_bounds =
        std::make_shared<ompl::base::RealVectorBounds>(2);
      ompl::base::StateSpacePtr state_space =
        std::make_shared<ompl::base::SE2StateSpace>();
      state_space->as<ompl::base::SE2StateSpace>()->setBounds(*state_space_bounds);
      ompl::base::SpaceInformationPtr state_space_information =
        std::make_shared<ompl::base::SpaceInformation>(state_space);
      ompl::geometric::PathGeometric path(state_space_information);

      ompl::base::ScopedState<ompl::base::SE2StateSpace>
      closest_ref_traj_state(state_space),
      ompl_local_goal_state(state_space);

      // Feed initial state, which is closest ref trajectory state
      double void_var, yaw;

      if (local_goal_state_index - nearsest_traj_state_index < mpc_parameters.N) {
        // Feed Intermediate state , which is nearest state in ref traj
        vox_nav_utilities::getRPYfromMsgQuaternion(
          curr_robot_pose.pose.orientation, void_var, void_var, yaw);
        closest_ref_traj_state[0] = curr_robot_pose.pose.position.x;
        closest_ref_traj_state[1] = curr_robot_pose.pose.position.y;
        closest_ref_traj_state[2] = yaw;
        path.append(static_cast<ompl::base::State *>(closest_ref_traj_state.get()));
      } else {
        // Feed Intermediate state , which is nearest state in ref traj
        vox_nav_utilities::getRPYfromMsgQuaternion(
          reference_traj.poses[nearsest_traj_state_index].pose.orientation, void_var, void_var,
          yaw);
        closest_ref_traj_state[0] =
          reference_traj.poses[nearsest_traj_state_index].pose.position.x;
        closest_ref_traj_state[1] =
          reference_traj.poses[nearsest_traj_state_index].pose.position.y;
        closest_ref_traj_state[2] = yaw;
        path.append(static_cast<ompl::base::State *>(closest_ref_traj_state.get()));
      }

      // Feed the final state, which the local goal for the current control effort.
      // This is basically the state in the ref trajectory, which is closest to global_plan_look_ahead_distance_
      vox_nav_utilities::getRPYfromMsgQuaternion(
        reference_traj.poses[local_goal_state_index].pose.orientation, void_var, void_var, yaw);
      ompl_local_goal_state[0] = reference_traj.poses[local_goal_state_index].pose.position.x;
      ompl_local_goal_state[1] = reference_traj.poses[local_goal_state_index].pose.position.y;
      ompl_local_goal_state[2] = yaw;
      path.append(static_cast<ompl::base::State *>(ompl_local_goal_state.get()));

      // The local ref traj now contains only 3 states, we will interpolate this states with OMPL
      // The count of states after interpolation must be same as horizon defined for the control problem , hence
      // it should be mpc_parameters_.N+1
      path.interpolate(mpc_parameters.N + 1);

      // Now the local ref traj is interpolated from current robot state up to state at global look ahead distance
      // Lets fill the native MPC type ref states and return to caller
      std::vector<States> interpolated_reference_states;
      for (std::size_t path_idx = 0; path_idx < path.getStateCount(); path_idx++) {
        // cast the abstract state type to the type we expect
        const ompl::base::SE2StateSpace::StateType * interpolated_state =
          path.getState(path_idx)->as<ompl::base::SE2StateSpace::StateType>();
        States curr_interpolated_state;
        curr_interpolated_state.v = kTARGETSPEED;
        curr_interpolated_state.x = interpolated_state->getX();
        curr_interpolated_state.y = interpolated_state->getY();
        curr_interpolated_state.psi = interpolated_state->getYaw();
        interpolated_reference_states.push_back(curr_interpolated_state);
      }
      return interpolated_reference_states;
    }

    static void publishTrajStates(
      const std::vector<States> & interpolated_reference_states,
      const std_msgs::msg::ColorRGBA & color,
      const std::string & ns,
      const rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr publisher)
    {
      visualization_msgs::msg::MarkerArray marker_array;
      for (int i = 0; i < interpolated_reference_states.size(); i++) {
        visualization_msgs::msg::Marker marker;
        marker.header.frame_id = "map";
        marker.header.stamp = rclcpp::Clock().now();
        marker.ns = ns;
        marker.id = i;
        marker.type = visualization_msgs::msg::Marker::ARROW;
        marker.action = visualization_msgs::msg::Marker::ADD;
        marker.lifetime = rclcpp::Duration::from_seconds(0);
        marker.pose.position.x = interpolated_reference_states[i].x;
        marker.pose.position.y = interpolated_reference_states[i].y;
        marker.pose.position.z = 1.3;
        marker.pose.orientation = vox_nav_utilities::getMsgQuaternionfromRPY(
          0, 0, interpolated_reference_states[i].psi);
        marker.scale.x = 0.25;
        marker.scale.y = 0.1;
        marker.scale.z = 0.1;
        marker.color = color;

        marker_array.markers.push_back(marker);
      }
      publisher->publish(marker_array);
    }
  }  //   namespace common

}  // namespace vox_nav_control

#endif  // VOX_NAV_CONTROL__COMMON_HPP_
