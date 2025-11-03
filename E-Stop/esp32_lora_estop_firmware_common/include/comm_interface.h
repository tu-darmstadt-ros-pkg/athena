#pragma once

#include <cstdint>

struct CommPeerInfo {
  uint8_t esp_now_mac[6];
  uint8_t ble_mac[6];
};

// Update these addresses to match your devices
// Typically the ble mac address is the esp_now address + 1
static constexpr CommPeerInfo RECEIVER_PEER_INFO = {
    { 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x30 },
    { 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x31 },
};

static constexpr CommPeerInfo SENDER_PEER_INFO = {
    { 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0xF4 },
    { 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0xF5 },
};

// If not deadman is used, just use dummy addresses
static constexpr CommPeerInfo DEADMAN_PEER_INFO = {
    { 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x38 },
    { 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x39 },
};

enum class CommState : uint8_t {
  DISCONNECTED = 0,
  CONNECTED = 1,
  ERROR = 10,
};

struct CommStatus {
  uint32_t last_received_message_age_ms = UINT32_MAX;
  int8_t ble_rssi = 0;
  int8_t radio_rssi = 0;
  int8_t esp_now_rssi = 0;
  CommState ble_state = CommState::DISCONNECTED;
  CommState esp_now_state = CommState::DISCONNECTED;
  CommState radio_state = CommState::DISCONNECTED;
};

enum class CommMode {
  SERVER,
  CLIENT,
};

static constexpr uint8_t COMM_PROPERTY_ID_ESTOP = 0x00;
static constexpr uint8_t COMM_PROPERTY_ID_SOFT_ESTOP = 0x01;
static constexpr uint8_t COMM_PROPERTY_ID_BATTERY = 0x02;
static constexpr uint8_t COMM_PROPERTY_ID_DEADMAN_ACTIVE = 0x03;
static constexpr uint8_t COMM_PROPERTY_ID_DEADMAN_TRIGGERED = 0x04;
static constexpr uint8_t COMM_PROPERTY_UUIDS[] = {
    COMM_PROPERTY_ID_ESTOP, COMM_PROPERTY_ID_SOFT_ESTOP, COMM_PROPERTY_ID_BATTERY,
    COMM_PROPERTY_ID_DEADMAN_ACTIVE, COMM_PROPERTY_ID_DEADMAN_TRIGGERED };
static constexpr int NUM_COMM_PROPERTIES =
    sizeof( COMM_PROPERTY_UUIDS ) / sizeof( COMM_PROPERTY_UUIDS[0] );

class BLEInterface;

class CommInterface
{
public:
  CommInterface();
  ~CommInterface();

  void initialize( CommMode mode, const CommPeerInfo &peer_info );

  CommStatus update();

  bool getEStopState() const;
  void setEStopState( bool active );
  bool getSoftEStopState() const;
  void setSoftEStopState( bool active );
  void reportBatteryLevel( uint8_t level );

  BLEInterface *getBLEInterface();

  class Impl;
  static Impl *impl_;

private:
  uint8_t counter_ = 0;
};
