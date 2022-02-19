// Copyright (c) 2020 Fetullah Atas, Norwegian University of Life Sciences
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

/*
DISCLAIMER: some parts of code has been taken from; https://github.com/appinho/SARosPerceptionKitti
Credits to author: Simon Appel, https://github.com/appinho
*/

#include "vox_nav_cupoch_experimental/simple_icp.hpp"

#include <string>
#include <vector>
#include <memory>
#include <algorithm>

using namespace vox_nav_cupoch_experimental;

SimpleICP::SimpleICP()
    : Node("cloud_clustering_rclcpp_node")
{

  cloud_subscriber_ = this->create_subscription<sensor_msgs::msg::PointCloud2>(
      "/ouster/points",
      rclcpp::SensorDataQoS(),
      std::bind(
          &SimpleICP::cloudCallback, this, std::placeholders::_1));

  map_cloud_subscriber_ = this->create_subscription<sensor_msgs::msg::PointCloud2>(
      "vox_nav/map_server/octomap_pointcloud",
      rclcpp::SensorDataQoS(),
      std::bind(
          &SimpleICP::mapCloudCallback, this, std::placeholders::_1));

  /*cloud_clusters_pub_ = this->create_publisher<sensor_msgs::msg::PointCloud2>(
    "/vox_nav/detection/clusters", rclcpp::SystemDefaultsQoS());*/

  // setup TF buffer and listerner to read transforms
  tf_buffer_ = std::make_shared<tf2_ros::Buffer>(this->get_clock());
  tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

  RCLCPP_INFO(get_logger(), "Creating...");
}

SimpleICP::~SimpleICP()
{
  RCLCPP_INFO(get_logger(), "Destroying...");
}

void SimpleICP::cloudCallback(
    const sensor_msgs::msg::PointCloud2::ConstSharedPtr cloud)
{
  if (map_configured_)
  {

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr pcl_curr(new pcl::PointCloud<pcl::PointXYZRGB>());
    pcl::fromROSMsg(*cloud, *pcl_curr);

    RCLCPP_INFO(get_logger(), "Live Cloud with %d points...", pcl_curr->points.size());

    auto croppped_live_cloud = vox_nav_utilities::cropBox<pcl::PointXYZRGB>(
        pcl_curr,
        Eigen::Vector4f(-20, -20, -5, 1),
        Eigen::Vector4f(20, 20, 5, 1));

    geometry_msgs::msg::PoseStamped curr_robot_pose;

    vox_nav_utilities::getCurrentPose(
        curr_robot_pose, *tf_buffer_, "map", "base_link", 0.1);

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr map(new pcl::PointCloud<pcl::PointXYZRGB>(map_));

    auto croppped_map_cloud = vox_nav_utilities::cropBox<pcl::PointXYZRGB>(
        map,
        Eigen::Vector4f(-20 + curr_robot_pose.pose.position.x,
                        -20 + curr_robot_pose.pose.position.y, -5, 1),

        Eigen::Vector4f(20 + curr_robot_pose.pose.position.x,
                        20 + curr_robot_pose.pose.position.y, 5, 1));

    thrust::host_vector<Eigen::Vector3f> map_points, live_points;

    for (int i = 0; i < croppped_map_cloud->points.size(); ++i)
    {
      auto p = croppped_map_cloud->points[i];
      Eigen::Vector3f point_eig(p.x, p.y, p.z);
      map_points.push_back(point_eig);
    }

    for (int i = 0; i < croppped_live_cloud->points.size(); ++i)
    {
      auto p = croppped_live_cloud->points[i];
      Eigen::Vector3f point_eig(p.x, p.y, p.z);
      live_points.push_back(point_eig);
    }

    auto map_points_cupoch = std::make_shared<cupoch::geometry::PointCloud>();
    auto live_points_cupoch = std::make_shared<cupoch::geometry::PointCloud>();
    map_points_cupoch->SetPoints(map_points);
    live_points_cupoch->SetPoints(live_points);

    // ICP
    Eigen::Matrix4f eye = Eigen::Matrix4f::Identity();
    auto point_to_point =
        cupoch::registration::TransformationEstimationPointToPoint();
    cupoch::registration::ICPConvergenceCriteria criteria;
    criteria.max_iteration_ = 100;
    auto res = cupoch::registration::RegistrationICP(*live_points_cupoch, *map_points_cupoch, 3.0, eye,
                                                     point_to_point, criteria);
    //source->Transform(res.transformation_);
  }
}

void SimpleICP::mapCloudCallback(
    const sensor_msgs::msg::PointCloud2::ConstSharedPtr cloud)
{
  std::call_once(
      get_map_cloud_once_, [&]()
      {
        pcl::fromROSMsg(*cloud, map_);
        map_configured_ = true; 
            RCLCPP_INFO(get_logger(), "Map Cloud with %d points...", map_.points.size()); });
}

int main(int argc, char const *argv[])
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<SimpleICP>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}