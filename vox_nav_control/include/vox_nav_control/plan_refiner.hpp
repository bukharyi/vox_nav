// Copyright (c) 2023 Norwegian University of Life Sciences, Fetullah Atas
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


#ifndef VOX_NAV_CONTROL__PLAN_REFINER_HPP_
#define VOX_NAV_CONTROL__PLAN_REFINER_HPP_
#pragma once

#include <string>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"

namespace vox_nav_control
{

  /**
   * @brief Base class for all plan refiner plugins, The aim of these plugins are readjust the plans that are generated by the global`
   *
   */
  class PlanRefiner
  {
  public:
    /**
     * @brief Construct a new Plan Refiner object
     *
     */
    PlanRefiner() {}

    /**
     * @brief Destroy the Plan Refiner object
     *
     */
    virtual ~PlanRefiner() {}

    /**
     * @brief
     *
     * @param parent rclcpp node
     * @param plugin_name refiner plugin name
     */
    virtual void initialize(
      const rclcpp::Node * parent,
      const std::string & plugin_name) = 0;

    /**
     * @brief Refine the plan locally, only in the vicinity of the robot
     *
     * @param curr_pose
     * @return true
     * @return false
     */
    virtual bool refinePlan(
      const geometry_msgs::msg::PoseStamped & curr_pose) = 0;
  };
}  // namespace vox_nav_control
#endif  // VOX_NAV_CONTROL__PLAN_REFINER_HPP_
