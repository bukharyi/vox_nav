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

#include "message_filters/subscriber.h"
#include "message_filters/sync_policies/approximate_time.h"
#include "message_filters/synchronizer.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.h"
#include <nav_msgs/msg/odometry.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <visualization_msgs/msg/marker_array.hpp>

#include <pcl/PCLPointCloud2.h>
#include <pcl/common/common.h>
#include <pcl/conversions.h>
#include <pcl/filters/crop_box.h>
#include <pcl/filters/extract_indices.h>
#include <pcl/io/pcd_io.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl_ros/transforms.hpp>
#include <tf2_eigen/tf2_eigen.h>

#include <boost/geometry.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <queue>
#include <sstream>

#include "ApproxMVBB/AABB.hpp"
#include "ApproxMVBB/ComputeApproxMVBB.hpp"
#include "cupoch/collision/collision.h"
#include "cupoch/cupoch.h"
#include "cupoch/geometry/occupancygrid.h"
#include "cupoch_conversions/cupoch_conversions.hpp"
#include "vox_nav_cupoch_experimental/visibility_control.h"
#include "vox_nav_utilities/map_manager_helpers.hpp"
#include "vox_nav_utilities/pcl_helpers.hpp"
#include "vox_nav_utilities/tf_helpers.hpp"

namespace bg = boost::geometry;

class CloudSegmentation : public rclcpp::Node {

public:
  typedef std::tuple<sensor_msgs::msg::PointCloud2::SharedPtr,
                     nav_msgs::msg::Odometry::SharedPtr,
                     sensor_msgs::msg::Imu::SharedPtr>
      data_captute_t;

  CloudSegmentation();
  ~CloudSegmentation();

  typedef message_filters::sync_policies::ApproximateTime<
      sensor_msgs::msg::PointCloud2, nav_msgs::msg::Odometry,
      sensor_msgs::msg::Imu>
      CloudOdomApprxTimeSyncPolicy;
  typedef message_filters::Synchronizer<CloudOdomApprxTimeSyncPolicy>
      CloudOdomApprxTimeSyncer;

  void
  cloudOdomCallback(const sensor_msgs::msg::PointCloud2::ConstSharedPtr &cloud,
                    const nav_msgs::msg::Odometry::ConstSharedPtr &odom,
                    const sensor_msgs::msg::Imu::ConstSharedPtr &imu);

  pcl::PointCloud<pcl::PointXYZRGB>::Ptr
  denoise_segmented_cloud(const pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud,
                          double radius, double tolerated_divergence_rate,
                          int min_num_neighbours);

  void shoot(std::vector<data_captute_t> &cloud_vector);

  std::vector<geometry_msgs::msg::Point>
  Vector3List2GeometryMsgs(ApproxMVBB::TypeDefsPoints::Vector3List corners);

  std::vector<geometry_msgs::msg::Point>
  Eigen2GeometryMsgs(std::array<Eigen::Matrix<float, 3, 1>, 8> obbx_corners);

  cupoch::utility::device_vector<Eigen::Vector3f>
  Vector3List2Eigen(ApproxMVBB::TypeDefsPoints::Vector3List corners);

private:
  rclcpp::TimerBase::SharedPtr timer_;
  message_filters::Subscriber<sensor_msgs::msg::PointCloud2> cloud_subscriber_;
  message_filters::Subscriber<nav_msgs::msg::Odometry> odom_subscriber_;
  message_filters::Subscriber<sensor_msgs::msg::Imu> imu_subscriber_;
  std::shared_ptr<CloudOdomApprxTimeSyncer>
      cloud_odom_data_approx_time_syncher_;

  double dt_;
  double sensor_height_;

  std::vector<data_captute_t> cloud_odom_vector_;

  bool recieved_first_;

  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pub_;
  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr dyn_point_pub_;
  rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr vis_pub_;

  builtin_interfaces::msg::Time last_recieved_msg_stamp_;

  std::shared_ptr<cupoch::geometry::PointCloud> last_pointcloud_cupoch_;
  nav_msgs::msg::Odometry::SharedPtr last_odom_msg_;
};

CloudSegmentation::CloudSegmentation()
    : Node("dynamic_points_node"), recieved_first_(false) {
  cloud_subscriber_.subscribe(this, "points", rmw_qos_profile_sensor_data);
  odom_subscriber_.subscribe(this, "odom", rmw_qos_profile_sensor_data);
  imu_subscriber_.subscribe(this, "imu", rmw_qos_profile_sensor_data);

  declare_parameter("dt", 0.0);
  get_parameter("dt", dt_);

  declare_parameter("sensor_height", 0.0);
  get_parameter("sensor_height", sensor_height_);

  cloud_odom_data_approx_time_syncher_.reset(new CloudOdomApprxTimeSyncer(
      CloudOdomApprxTimeSyncPolicy(3000), cloud_subscriber_, odom_subscriber_,
      imu_subscriber_));

  cloud_odom_data_approx_time_syncher_->registerCallback(std::bind(
      &CloudSegmentation::cloudOdomCallback, this, std::placeholders::_1,
      std::placeholders::_2, std::placeholders::_3));

  pub_ = this->create_publisher<sensor_msgs::msg::PointCloud2>(
      "merged", rclcpp::SystemDefaultsQoS());

  dyn_point_pub_ = this->create_publisher<sensor_msgs::msg::PointCloud2>(
      "dyn_point_pub", rclcpp::SystemDefaultsQoS());

  vis_pub_ = this->create_publisher<visualization_msgs::msg::MarkerArray>(
      "markers", rclcpp::SystemDefaultsQoS());

  auto captured_data = std::make_tuple<>(
      std::make_shared<sensor_msgs::msg::PointCloud2>(
          sensor_msgs::msg::PointCloud2()),
      std::make_shared<nav_msgs::msg::Odometry>(nav_msgs::msg::Odometry()),
      std::make_shared<sensor_msgs::msg::Imu>(sensor_msgs::msg::Imu()));

  cloud_odom_vector_ = std::vector<data_captute_t>(2, captured_data);

  last_odom_msg_ = std::make_shared<nav_msgs::msg::Odometry>();
  last_pointcloud_cupoch_ = std::make_shared<cupoch::geometry::PointCloud>();

  last_recieved_msg_stamp_ = now();
}

CloudSegmentation::~CloudSegmentation() {}

void CloudSegmentation::cloudOdomCallback(
    const sensor_msgs::msg::PointCloud2::ConstSharedPtr &cloud,
    const nav_msgs::msg::Odometry::ConstSharedPtr &odom,
    const sensor_msgs::msg::Imu::ConstSharedPtr &imu) {

  // convert to pcl type
  pcl::PointCloud<pcl::PointXYZRGB>::Ptr curr_pcl(
      new pcl::PointCloud<pcl::PointXYZRGB>());
  pcl::PCLPointCloud2 pcl_pc;
  pcl_conversions::toPCL(*cloud, pcl_pc);
  pcl::fromPCLPointCloud2(pcl_pc, *curr_pcl);

  // remove ground points
  sensor_msgs::PointCloud2ConstIterator<float> iter_label(*cloud, "x");
  std::vector<u_int8_t> labels;

  cupoch::utility::device_vector<size_t> ground_points_indices;
  cupoch::utility::device_vector<size_t> dynamic_points_indices;
  cupoch::utility::device_vector<size_t> static_points_indices;

  size_t label_counter = 0;
  for (; (iter_label != iter_label.end()); ++iter_label) {
    labels.push_back(iter_label[3]);
    if (iter_label[3] == 40) { // ground point label
      ground_points_indices.push_back(label_counter);
    } else if (iter_label[3] == 30 ||
               iter_label[3] == 10) { // person/car point label
      dynamic_points_indices.push_back(label_counter);
    } else { // static obstacle point label
      static_points_indices.push_back(label_counter);
    }
    label_counter++;
  }

  /*curr_pcl = vox_nav_utilities::removeOutliersFromInputCloud(
    curr_pcl, 4, 0.5,
    vox_nav_utilities::OutlierRemovalType::RadiusOutlierRemoval);
  curr_pcl =
      vox_nav_utilities::downsampleInputCloud<pcl::PointXYZRGB>(curr_pcl,
  0.05);*/

  cupoch::geometry::PointCloud obstacle_cloud_cupoch;
  thrust::host_vector<Eigen::Vector3f> points;
  thrust::host_vector<Eigen::Vector3f> colors;

  for (int i = 0; i < curr_pcl->points.size(); ++i) {
    auto p = curr_pcl->points[i];
    points.push_back(Eigen::Vector3f(p.x, p.y, p.z));
    auto c = vox_nav_utilities::getColorByIndexEig(5);
    colors.push_back(Eigen::Vector3f(c.x(), c.y(), c.z()));
  }
  obstacle_cloud_cupoch.SetPoints(points);
  obstacle_cloud_cupoch.SetColors(colors);

  auto obstacle_cloud_cupoch_copy = obstacle_cloud_cupoch;

  auto dynamic_points_cupoch =
      obstacle_cloud_cupoch.SelectByIndex(dynamic_points_indices, false);
  auto static_points_cupoch =
      obstacle_cloud_cupoch_copy.SelectByIndex(static_points_indices, false);

  dynamic_points_cupoch->PaintUniformColor(
      vox_nav_utilities::getColorByIndexEig(2));
  static_points_cupoch->PaintUniformColor(
      vox_nav_utilities::getColorByIndexEig(1));

  if ((cloud->header.stamp.sec - last_recieved_msg_stamp_.sec) > dt_) {
    auto travel_dist =
        Eigen::Vector3f(
            odom->pose.pose.position.x - last_odom_msg_->pose.pose.position.x,
            odom->pose.pose.position.y - last_odom_msg_->pose.pose.position.y,
            odom->pose.pose.position.z - last_odom_msg_->pose.pose.position.z)
            .norm();

    double yaw_latest, pitch_latest, roll_latest;
    double yaw, pitch, roll;

    vox_nav_utilities::getRPYfromMsgQuaternion(
        odom->pose.pose.orientation, roll_latest, pitch_latest, yaw_latest);
    vox_nav_utilities::getRPYfromMsgQuaternion(
        last_odom_msg_->pose.pose.orientation, roll, pitch, yaw);

    auto rot = cupoch::geometry::GetRotationMatrixFromXYZ(Eigen::Vector3f(
        roll_latest - roll, pitch_latest - pitch, yaw_latest - yaw));

    auto trans =
        Eigen::Vector3f(travel_dist * cos(yaw_latest - yaw),
                        travel_dist * sin(yaw_latest - yaw), sensor_height_);
    Eigen::Matrix4f odom_T = Eigen::Matrix4f::Identity();

    odom_T.block<3, 3>(0, 0) = rot;
    odom_T.block<3, 1>(0, 3) = trans;

    last_pointcloud_cupoch_->Transform(odom_T);
    last_pointcloud_cupoch_->PaintUniformColor(
        vox_nav_utilities::getColorByIndexEig(4));

    last_recieved_msg_stamp_ = cloud->header.stamp;
    last_pointcloud_cupoch_ =
        std::make_shared<cupoch::geometry::PointCloud>(obstacle_cloud_cupoch);
    last_odom_msg_ = std::make_shared<nav_msgs::msg::Odometry>(*odom);

    last_pointcloud_cupoch_ =
        std::make_shared<cupoch::geometry::PointCloud>(*dynamic_points_cupoch);

    *dynamic_points_cupoch += *static_points_cupoch + *last_pointcloud_cupoch_;

    auto voxel_grid = cupoch::geometry::VoxelGrid::CreateFromPointCloud(
        *dynamic_points_cupoch, 0.2);

    cupoch::visualization::DrawGeometries({voxel_grid}, "Copoch", 640, 480, 50,
                                          50, true, true, false);
  }

  *dynamic_points_cupoch += *static_points_cupoch;

  auto voxel_grid = cupoch::geometry::VoxelGrid::CreateFromPointCloud(
      *dynamic_points_cupoch, 0.2);

  cupoch::visualization::DrawGeometries({voxel_grid}, "Copoch", 640, 480, 50,
                                        50, true, true, false);

  /*
    auto occupancy_grid =
      cupoch::geometry::OccupancyGrid::CreateFromVoxelGrid(*voxel_grid);
    auto distance_transform =
      cupoch::geometry::DistanceTransform::CreateFromOccupancyGrid(
          *occupancy_grid);*/

  /*sensor_msgs::msg::PointCloud2 denoised_cloud_msg;
  pcl::toROSMsg(*merged_clusters, denoised_cloud_msg);
  denoised_cloud_msg.header = cloud->header;
  RCLCPP_INFO(get_logger(), " Writing a cloud with %d points",
              merged_clusters->points.size());
  pub_->publish(denoised_cloud_msg);*/
}

void CloudSegmentation::shoot(std::vector<data_captute_t> &cloud_vector) {}

std::vector<geometry_msgs::msg::Point>
CloudSegmentation::Vector3List2GeometryMsgs(
    ApproxMVBB::TypeDefsPoints::Vector3List corners) {
  std::vector<geometry_msgs::msg::Point> corners_geometry_msgs;
  for (int i = 0; i < corners.size(); i++) {
    geometry_msgs::msg::Point korner_point;
    korner_point.x = corners[i].x();
    korner_point.y = corners[i].y();
    korner_point.z = corners[i].z();
    corners_geometry_msgs.push_back(korner_point);
  }
  return corners_geometry_msgs;
}

std::vector<geometry_msgs::msg::Point> CloudSegmentation::Eigen2GeometryMsgs(
    std::array<Eigen::Matrix<float, 3, 1>, 8> obbx_corners) {
  std::vector<geometry_msgs::msg::Point> corners_geometry_msgs;
  for (int i = 0; i < obbx_corners.size(); i++) {
    geometry_msgs::msg::Point korner_point;
    korner_point.x = obbx_corners[i].x();
    korner_point.y = obbx_corners[i].y();
    korner_point.z = obbx_corners[i].z();
    corners_geometry_msgs.push_back(korner_point);
  }
  return corners_geometry_msgs;
}

cupoch::utility::device_vector<Eigen::Vector3f>
CloudSegmentation::Vector3List2Eigen(
    ApproxMVBB::TypeDefsPoints::Vector3List corners) {
  cupoch::utility::device_vector<Eigen::Vector3f> corners_eigen;
  for (int i = 0; i < corners.size(); i++) {
    Eigen::Vector3f korner_point;
    korner_point.x() = corners[i].x();
    korner_point.y() = corners[i].y();
    korner_point.z() = corners[i].z();
    corners_eigen.push_back(korner_point);
  }
  return corners_eigen;
}

int main(int argc, char const *argv[]) {
  rclcpp::init(argc, argv);
  cupoch::utility::InitializeAllocator();
  auto node = std::make_shared<CloudSegmentation>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}