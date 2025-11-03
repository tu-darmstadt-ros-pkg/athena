#pragma once

#include "comm_interface.h"
#include <NimBLEAddress.h>
#include <cstdint>
#include <vector>

#ifndef ESTOP_BLE_NAME
  #define ESTOP_BLE_NAME "Hector EStop"
#endif

#ifndef ESTOP_BLE_ENCRYPTION
  #define ESTOP_BLE_ENCRYPTION 0
#endif

#ifndef ESTOP_BLE_PASSKEY
  #define ESTOP_BLE_PASSKEY 3281254
#endif

static constexpr uint16_t ESTOP_SERVICE_UUID = 2308;
static constexpr uint16_t ESTOP_CHARACTERISTIC_UUID = 1994;


inline static const std::vector<NimBLEAddress> BLE_WHITELIST = {
    NimBLEAddress( RECEIVER_PEER_INFO.ble_mac, 0 ),
    NimBLEAddress( DEADMAN_PEER_INFO.ble_mac, 0 ),
    NimBLEAddress( SENDER_PEER_INFO.ble_mac, 0 ),
};

class BLEInterface
{
public:
  virtual void update() = 0;

  virtual CommState getCommState( const NimBLEAddress &address ) const = 0;

  virtual int8_t getRSSI( const NimBLEAddress &address ) const = 0;

  virtual void setProperty( uint8_t id, const std::vector<uint8_t> &data ) = 0;
  virtual void readProperty( uint8_t id, std::vector<uint8_t> &data, unsigned long &age_ms ) const = 0;
};