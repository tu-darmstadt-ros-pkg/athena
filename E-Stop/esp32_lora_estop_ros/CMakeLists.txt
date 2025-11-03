cmake_minimum_required(VERSION 3.8)
project(esp32_lora_estop_ros)

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(esp32_lora_estop_interface REQUIRED)
find_package(hector_ros2_utils REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(std_msgs REQUIRED)

find_package(PkgConfig)
pkg_check_modules(SERIAL libserial)

add_library(receiver_interface_node_component SHARED src/receiver_interface_node.cpp)
target_include_directories(receiver_interface_node_component PRIVATE ../esp32_lora_estop_firmware_common/include)

rclcpp_components_register_node(receiver_interface_node_component
  PLUGIN "esp32_lora_estop_ros::ReceiverInterfaceNode"
  EXECUTABLE receiver_interface_node
)

target_include_directories(receiver_interface_node_component PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

ament_target_dependencies(receiver_interface_node_component
  esp32_lora_estop_interface
  hector_ros2_utils
  rclcpp
  rclcpp_components
  rclcpp_lifecycle
  std_msgs
  SERIAL
)

install(TARGETS receiver_interface_node_component
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY config launch
  DESTINATION share/${PROJECT_NAME}
  OPTIONAL
)

ament_package()
