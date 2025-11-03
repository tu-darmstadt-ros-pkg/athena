#pragma once
#include "ble_interface.h"
#include <NimBLEDevice.h>
#include <elapsedMillis.h>

class BLEServerInterface : public BLEInterface,
                           public NimBLEServerCallbacks,
                           public NimBLECharacteristicCallbacks
{
public:
  BLEServerInterface( const std::string &name );

  CommState getCommState( const NimBLEAddress &address) const override;

  int8_t getRSSI( const NimBLEAddress &address ) const override;

  void update() override;

  void setProperty( uint8_t id, const std::vector<uint8_t> &data ) override;
  void readProperty( uint8_t id, std::vector<uint8_t> &data, unsigned long &age_ms ) const override;

private:
  void onConnect( NimBLEServer *server, NimBLEConnInfo &conn_info ) override;

  void onDisconnect( NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason ) override;

  void onRead( NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo ) override;

  void onWrite( NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo ) override;

  NimBLEServer *server_ = nullptr;
  NimBLEService *service_ = nullptr;
  std::vector<NimBLEConnInfo> connected_clients_;
  NimBLEAdvertising *advertising_ = nullptr;
};