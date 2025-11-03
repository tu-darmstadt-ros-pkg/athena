#ifndef ESP32_LORA_ESTOP_ROS_INTERFACE_RECEIVER_INTERFACE_NODE_HPP
#define ESP32_LORA_ESTOP_ROS_INTERFACE_RECEIVER_INTERFACE_NODE_HPP

#include <memory>
#include <string>
#include <vector>

#include <esp32_lora_estop_interface/msg/comm_status.hpp>
#include <esp32_lora_estop_interface/srv/set_enabled.hpp>
#include <hector_ros2_utils/lifecycle_node.hpp>
#include <libserial/SerialPort.h>
#include <lifecycle_msgs/msg/state.hpp>
#include <lifecycle_msgs/msg/transition.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_publisher.hpp>
#include <std_msgs/msg/bool.hpp>

namespace crosstalk
{
template<int BUFFER_SIZE, int SERIALIZATION_BUFFER_SIZE>
class CrossTalker;
}

namespace esp32_lora_estop_ros
{

class ReceiverInterfaceNode : public hector::LifecycleNode
{
public:
  explicit ReceiverInterfaceNode( const rclcpp::NodeOptions &options );

protected:
  /**
   * @brief Processes 'configuring' transitions to 'inactive' state
   *
   * @param state previous state
   * @return transition result
   */
  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_configure( const rclcpp_lifecycle::State &state ) override;

  /**
   * @brief Processes 'activating' transitions to 'active' state
   *
   * @param state previous state
   * @return transition result
   */
  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_activate( const rclcpp_lifecycle::State &state ) override;

  /**
   * @brief Processes 'deactivating' transitions to 'inactive' state
   *
   * @param state previous state
   * @return transition result
   */
  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_deactivate( const rclcpp_lifecycle::State &state ) override;

  /**
   * @brief Processes 'cleaningup' transitions to 'unconfigured' state
   *
   * @param state previous state
   * @return transition result
   */
  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_cleanup( const rclcpp_lifecycle::State &state ) override;

  /**
   * @brief Processes 'shuttingdown' transitions to 'finalized' state
   *
   * @param state previous state
   * @return transition result
   */
  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_shutdown( const rclcpp_lifecycle::State &state ) override;

private:
  //! @brief Sets up subscribers, publishers, etc. to configure the node
  void setup();

  void initSerialPort();

  void loopCallback();

  void onSetEnabled( const esp32_lora_estop_interface::srv::SetEnabled::Request::SharedPtr request,
                     esp32_lora_estop_interface::srv::SetEnabled::Response::SharedPtr response );

private:
  template<typename Msg>
  using Publisher = rclcpp_lifecycle::LifecyclePublisher<Msg>;
  Publisher<std_msgs::msg::Bool>::SharedPtr estop_publisher_;
  Publisher<std_msgs::msg::Bool>::SharedPtr soft_estop_publisher_;
  Publisher<esp32_lora_estop_interface::msg::CommStatus>::SharedPtr remote_comm_status_publisher_;
  Publisher<esp32_lora_estop_interface::msg::CommStatus>::SharedPtr deadman_comm_status_publisher_;
  rclcpp::Service<esp32_lora_estop_interface::srv::SetEnabled>::SharedPtr set_enabled_service_;
  rclcpp::TimerBase::SharedPtr loop_timer_;
  std::string port_ = "/dev/tty_estop_receiver";
  std::unique_ptr<LibSerial::SerialPort> serial_port_;
  std::unique_ptr<crosstalk::CrossTalker<2048, 128>> cross_talker_;
  int error_count_ = 0;
  bool estop_state_ = true;
  bool soft_estop_state_ = true;
  bool first_publish_ = true;
};

} // namespace esp32_lora_estop_ros

#endif // ESP32_LORA_ESTOP_ROS_INTERFACE_RECEIVER_INTERFACE_NODE_HPP
