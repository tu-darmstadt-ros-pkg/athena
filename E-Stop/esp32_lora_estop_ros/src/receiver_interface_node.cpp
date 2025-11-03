#include "esp32_lora_estop_ros/receiver_interface_node.hpp"
#include "host_comm.h"
#include <algorithm>
#include <functional>

#include "crosstalk_lib_serial_wrapper.hpp"

#include <rclcpp_components/register_node_macro.hpp>
RCLCPP_COMPONENTS_REGISTER_NODE( esp32_lora_estop_ros::ReceiverInterfaceNode )

namespace esp32_lora_estop_ros
{

ReceiverInterfaceNode::ReceiverInterfaceNode( const rclcpp::NodeOptions &options )
    : LifecycleNode( "receiver_interface_node", options )
{

  int startup_state = lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE;
  declare_readonly_parameter( "startup_state", startup_state, "Initial lifecycle state" );
  declare_readonly_parameter(
      "serial_port", port_, "Serial port to use for communication with the ESP32 LoRa E-Stop device" );
  if ( startup_state > lifecycle_msgs::msg::State::PRIMARY_STATE_UNCONFIGURED )
    trigger_transition( lifecycle_msgs::msg::Transition::TRANSITION_CONFIGURE );
  if ( startup_state > lifecycle_msgs::msg::State::PRIMARY_STATE_INACTIVE )
    trigger_transition( lifecycle_msgs::msg::Transition::TRANSITION_ACTIVATE );
  set_enabled_service_ = create_service<esp32_lora_estop_interface::srv::SetEnabled>(
      "remote_estop/set_enabled",
      [this]( const std::shared_ptr<esp32_lora_estop_interface::srv::SetEnabled::Request> request,
              std::shared_ptr<esp32_lora_estop_interface::srv::SetEnabled::Response> ) {
        RCLCPP_INFO( get_logger(), "SetEnabled command received: %s",
                     request->enabled ? "ENABLED" : "DISABLED" );
        auto result = cross_talker_->sendObject( SetEnabledCommand{ request->enabled } );
        if ( result != crosstalk::WriteResult::Success ) {
          RCLCPP_ERROR( get_logger(), "Failed to send SetEnabled command: %s",
                        crosstalk::to_string( result ).c_str() );
        }
      } );
}

void ReceiverInterfaceNode::setup()
{

  // publisher for publishing outgoing messages
  estop_publisher_ = create_publisher<std_msgs::msg::Bool>(
      "remote_estop/hard_estop", rclcpp::QoS( 1 ).reliable().transient_local() );

  soft_estop_publisher_ = create_publisher<std_msgs::msg::Bool>(
      "remote_estop/soft_estop", rclcpp::QoS( 1 ).reliable().transient_local() );

  remote_comm_status_publisher_ = create_publisher<esp32_lora_estop_interface::msg::CommStatus>(
      "remote_estop/remote_comm_status", rclcpp::QoS( 1 ).reliable().transient_local() );

  deadman_comm_status_publisher_ = create_publisher<esp32_lora_estop_interface::msg::CommStatus>(
      "remote_estop/deadman_comm_status", rclcpp::QoS( 1 ).reliable().transient_local() );
}

rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
ReceiverInterfaceNode::on_configure( const rclcpp_lifecycle::State &state )
{

  RCLCPP_INFO( get_logger(), "Configuring to enter 'inactive' state from '%s' state",
               state.label().c_str() );

  initSerialPort();
  setup();

  return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
}

rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
ReceiverInterfaceNode::on_activate( const rclcpp_lifecycle::State &state )
{

  RCLCPP_INFO( get_logger(), "Activating to enter 'active' state from '%s' state",
               state.label().c_str() );

  estop_publisher_->on_activate();
  soft_estop_publisher_->on_activate();
  remote_comm_status_publisher_->on_activate();
  deadman_comm_status_publisher_->on_activate();

  estop_publisher_->publish( std_msgs::msg::Bool().set__data( true ) );
  soft_estop_publisher_->publish( { std_msgs::msg::Bool().set__data( true ) } );
  // timer for periodic loop
  loop_timer_ = create_wall_timer( std::chrono::milliseconds( 50 ), [this] { loopCallback(); } );

  return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
}

rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
ReceiverInterfaceNode::on_deactivate( const rclcpp_lifecycle::State &state )
{

  RCLCPP_INFO( get_logger(), "Deactivating to enter 'inactive' state from '%s' state",
               state.label().c_str() );

  estop_publisher_->on_deactivate();
  soft_estop_publisher_->on_deactivate();
  remote_comm_status_publisher_->on_deactivate();
  deadman_comm_status_publisher_->on_deactivate();
  loop_timer_->cancel();

  return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
}

rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
ReceiverInterfaceNode::on_cleanup( const rclcpp_lifecycle::State &state )
{

  RCLCPP_INFO( get_logger(), "Cleaning up to enter 'unconfigured' state from '%s' state",
               state.label().c_str() );

  estop_publisher_.reset();
  soft_estop_publisher_.reset();
  remote_comm_status_publisher_.reset();
  deadman_comm_status_publisher_.reset();
  cross_talker_.reset();
  if ( serial_port_ )
    serial_port_->Close();
  serial_port_.reset();

  return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
}

rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
ReceiverInterfaceNode::on_shutdown( const rclcpp_lifecycle::State &state )
{

  RCLCPP_INFO( get_logger(), "Shutting down to enter 'finalized' state from '%s' state",
               state.label().c_str() );

  if ( state.id() >= lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE )
    on_deactivate( state );
  if ( state.id() >= lifecycle_msgs::msg::State::PRIMARY_STATE_INACTIVE )
    on_cleanup( state );

  return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
}
void ReceiverInterfaceNode::initSerialPort()
{
  if ( serial_port_ && serial_port_->IsOpen() ) {
    RCLCPP_INFO( get_logger(), "Serial port already open, skipping initialization." );
    return;
  }
  RCLCPP_INFO( get_logger(), "Opening serial port: %s", port_.c_str() );
  serial_port_ = std::make_unique<LibSerial::SerialPort>();
  serial_port_->Open( port_ );
  cross_talker_ = std::make_unique<crosstalk::CrossTalker<2048, 128>>(
      std::make_unique<crosstalk::LibSerialWrapper>( *serial_port_ ) );
}

namespace
{
esp32_lora_estop_interface::msg::CommStatus toMsg( const CommStatus &status )
{
  esp32_lora_estop_interface::msg::CommStatus msg;
  msg.ble_rssi = status.ble_rssi;
  msg.esp_now_rssi = status.esp_now_rssi;
  msg.radio_rssi = status.radio_rssi;
  msg.ble_connection_status = static_cast<uint8_t>( status.ble_state );
  msg.esp_now_connection_status = static_cast<uint8_t>( status.esp_now_state );
  msg.radio_connection_status = static_cast<uint8_t>( status.radio_state );
  msg.last_message_age_ms = status.last_received_message_age_ms;
  return msg;
}
} // namespace

void ReceiverInterfaceNode::loopCallback()
{
  if ( !cross_talker_ ) {
    return;
  }
  try {
    cross_talker_->processSerialData();
    if ( cross_talker_->available() ) {
      std::vector<uint8_t> buffer;
      buffer.resize( cross_talker_->available() );
      cross_talker_->read( buffer.data(), buffer.size() );
      std::cout << reinterpret_cast<char *>( buffer.data() );
    }
    if ( cross_talker_->hasObject() ) {
      int16_t object_id = cross_talker_->getObjectId();
      switch ( object_id ) {
      case crosstalk::object_id<EStopState>(): {
        EStopState estop_state;
        if ( cross_talker_->readObject( estop_state ) == crosstalk::ReadResult::Success ) {
          if ( first_publish_ || estop_state_ != estop_state.hard_estop_active ) {
            estop_state_ = estop_state.hard_estop_active;
            estop_publisher_->publish( std_msgs::msg::Bool().set__data( estop_state_ ) );
            RCLCPP_INFO( get_logger(), "Hard E-Stop state changed: %s",
                         estop_state_ ? "ACTIVE" : "INACTIVE" );
          }
          if ( first_publish_ || soft_estop_state_ != estop_state.soft_estop_active ) {
            soft_estop_state_ = estop_state.soft_estop_active;
            soft_estop_publisher_->publish( std_msgs::msg::Bool().set__data( soft_estop_state_ ) );
            RCLCPP_INFO( get_logger(), "Soft E-Stop state changed: %s",
                         soft_estop_state_ ? "ACTIVE" : "INACTIVE" );
          }
          first_publish_ = false;
        }
        break;
      }
      case crosstalk::object_id<EStopReceiverStatus>(): {
        EStopReceiverStatus estop_status;

        if ( cross_talker_->readObject( estop_status ) == crosstalk::ReadResult::Success ) {
          esp32_lora_estop_interface::msg::CommStatus remote_msg =
              toMsg( estop_status.remote_status );
          remote_comm_status_publisher_->publish( remote_msg );
          esp32_lora_estop_interface::msg::CommStatus deadman_msg =
              toMsg( estop_status.deadman_status );
          deadman_comm_status_publisher_->publish( deadman_msg );
        } else {
          RCLCPP_WARN( get_logger(), "Failed to read CommStatus object" );
        }
        break;
      }
      default:
        RCLCPP_WARN( get_logger(), "Received object with unknown ID: %d", object_id );
        break;
      }
    }
    error_count_ = 0;
  } catch ( std::runtime_error &e ) {
    if ( ++error_count_ > 5 ) {
      cross_talker_.reset();
      serial_port_.reset();
      error_count_ = 0;
    }
  }
}

} // namespace esp32_lora_estop_ros
