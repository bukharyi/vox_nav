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

#include <rclcpp/rclcpp.hpp>
#include <vox_nav_control/common.hpp>
#include <vox_nav_control/controller_core.hpp>
#include <vox_nav_utilities/tf_helpers.hpp>
#include <vox_nav_msgs/msg/object_array.hpp>

#include <tf2_geometry_msgs/tf2_geometry_msgs.h>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>
#include <visualization_msgs/msg/marker_array.hpp>

#include <ompl/geometric/PathGeometric.h>
#include <ompl/base/SpaceInformation.h>
#include <ompl/base/spaces/SE2StateSpace.h>
#include <ompl/base/spaces/DubinsStateSpace.h>
#include <ompl/base/spaces/ReedsSheppStateSpace.h>
#include <ompl/base/ScopedState.h>

#include <string>
#include <memory>
#include <vector>

#include <vox_nav_control/mpc_controller_acado/auto_gen/acado_common.h>
#include <vox_nav_control/mpc_controller_acado/auto_gen/acado_auxiliary_functions.h>


#ifndef VOX_NAV_CONTROL__MPC_CONTROLLER__MPC_CONTROLLER_ACADO_ROS_HPP_
#define VOX_NAV_CONTROL__MPC_CONTROLLER__MPC_CONTROLLER_ACADO_ROS_HPP_

namespace vox_nav_control
{

  namespace mpc_controller_acado
  {

    enum STATE
    {
      kX = 0,
      kY = 1,
      kPsi = 2,
      kV = 3,
    };

    enum INPUT
    {
      kacc = 0,
      kdf = 1,
    };


/**
 * @brief A ROS wrapper around the Core MPC controller class.
 *        This class interfaces MPC controller with ControllerCore which is base class for plugins.
 *        Derivatives of ControllerCore class are used to expose Controller server for use
 *        The controller server is an ROS2 action named follow_path
 *
 */
    class MPCControllerAcadoROS : public vox_nav_control::ControllerCore
    {
    public:
      /**
       * @brief Construct a new MPCControllerAcadoROS object
       *
       */
      MPCControllerAcadoROS();

      /**
       * @brief Destroy the MPCControllerAcadoROS object
       *
       */
      ~MPCControllerAcadoROS();

      /**
       * @brief gets all parameters required by MPC controller. Sets up an ROS2 stuff, publisher etc.
       *
       * @param parent
       * @param plugin_name
       */
      void initialize(
        rclcpp::Node * parent,
        const std::string & plugin_name) override;

      /**
       * @brief Set the Plan from controller server
       *
       * @param path
       */
      void setPlan(const nav_msgs::msg::Path & path) override;

      /**
       * @brief Compute required velocity commands to drive the robot along the reference_trajectory_
       *
       * @param curr_robot_pose
       * @return geometry_msgs::msg::Twist
       */
      geometry_msgs::msg::Twist computeVelocityCommands(
        geometry_msgs::msg::PoseStamped curr_robot_pose)
      override;

      /**
       * @brief Compute required velocity commands recorrect heading. This function is used when robot already satisfies
       * goal_tolerance_distance
       *
       * @param curr_robot_pose
       * @return geometry_msgs::msg::Twist
       */
      geometry_msgs::msg::Twist computeHeadingCorrectionCommands(
        geometry_msgs::msg::PoseStamped curr_robot_pose)
      override;

      /**
       * @brief get the index of nearest trajectory state to current robot pose
       *
       * @param reference_traj
       * @param curr_robot_pose
       * @return int
       */
      int nearestStateIndex(
        nav_msgs::msg::Path reference_traj,
        geometry_msgs::msg::PoseStamped curr_robot_pose)
      override;

      /**
       * @brief Get the Interpolated Reference States, This is determined by the look ahead distance set in the parameters.
       *        As well as number of time Horizons(N)
       *
       * @param curr_robot_pose
       * @return std::vector<MPCControllerCasadiCore::States>
       */
      std::vector<States> getLocalInterpolatedReferenceStates(
        geometry_msgs::msg::PoseStamped curr_robot_pose);

      /**
       * @brief
       *
       * @param interpolated_reference_states
       * @param color
       * @param ns
       * @param publisher
       */
      void publishTrajStates(
        std::vector<States> interpolated_reference_states,
        std_msgs::msg::ColorRGBA color,
        std::string ns,
        const rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr publisher);

      /**
       * @brief
       *
       * @param msg
       */
      void obstacleTracksCallback(const vox_nav_msgs::msg::ObjectArray::SharedPtr msg);

      /**
       * @brief Convert ROS msg type of ObjectsArray to vector of Ellipsoid to feed to MPC controller
       *
       * @param tracks
       * @return std::vector<Ellipsoid>
       */
      std::vector<Ellipsoid> trackMsg2Ellipsoids(const vox_nav_msgs::msg::ObjectArray & tracks);

    private:
      // Given refernce traj to follow, this is set got from planner
      nav_msgs::msg::Path reference_traj_;
      // computed velocities as result of MPC
      geometry_msgs::msg::Twist computed_velocity_;
      // MPC core controller object
      // parameters struct used for MPC controller
      Parameters mpc_parameters_;
      // Keep a copy of previously applied control inputs, this is neded
      // by MPC algorithm
      std::vector<ControlInput> previous_control_;
      // Publish local trajecory currently being fed to controller
      rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr
        interpolated_local_reference_traj_publisher_;
      rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr
        mpc_computed_traj_publisher_;
      // while following global plan whats the max look ahead distance ?, thats global_plan_look_ahead_distance_
      double global_plan_look_ahead_distance_;

      rclcpp::Node * parent_;
      rclcpp::Subscription<vox_nav_msgs::msg::ObjectArray>::SharedPtr obstacle_tracks_sub_;
      vox_nav_msgs::msg::ObjectArray obstacle_tracks_;
      std::mutex obstacle_tracks_mutex_;

    };

  } // namespace mpc_controller_acado
}  // namespace vox_nav_control

#endif  // VOX_NAV_CONTROL__MPC_CONTROLLER__MPC_CONTROLLER_ACADO_ROS_HPP_
