#ifndef ESP32_ESTOP_CONFIG_H
#define ESP32_ESTOP_CONFIG_H

#include <cstdint>

#define ROBOT_NAME "EStop"
#define ESTOP_BLE_NAME "EStop Remote"
#define ESTOP_BLE_ENCRYPTION 0
#define ESTOP_BLE_PASSKEY 123456
// WiFi channel used by ESP-NOW. Must match on all devices.
// Change this to avoid conflicts with other E-Stop deployments nearby.
#define ESTOP_ESPNOW_CHANNEL 6

// Time after which the E-Stop is considered active if no messages are received
static constexpr int SAFETY_TIMEOUT_MS = 500;
// Resend status every x ms even if not changed
constexpr int STATUS_UPDATE_INTERVAL_MS = 20;
// Resend soft E-Stop at a slower rate - it is not safety-critical
constexpr int SOFT_ESTOP_UPDATE_INTERVAL_MS = 500;


struct CommPeerInfo {
  uint8_t esp_now_mac[6];
  uint8_t ble_mac[6];
};

// Update these addresses to match your devices.
// Flash each device and read the MAC from the serial output on startup, e.g.:
//   "BLE Device initialized with address: d8:3b:da:xx:xx:xx"
// The ESP-NOW MAC is typically the BLE MAC minus 1.
static constexpr CommPeerInfo RECEIVER_PEER_INFO = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
};

static constexpr CommPeerInfo SENDER_PEER_INFO = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
};

// If no deadman is used, just use dummy addresses
static constexpr CommPeerInfo DEADMAN_PEER_INFO = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
};

#endif // ESP32_ESTOP_CONFIG_H
