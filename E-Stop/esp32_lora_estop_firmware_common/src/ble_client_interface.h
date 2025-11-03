#pragma once

#include "ble_interface.h"
#include <NimBLEDevice.h>
#include <elapsedMillis.h>

class BLEClientInterface : public BLEInterface, public NimBLEClientCallbacks, public NimBLEScanCallbacks
{
public:
  BLEClientInterface( const std::string &server_name, NimBLEAddress server_address );

  CommState getCommState( const NimBLEAddress & ) const override
  {
    if ( client_ && client_->isConnected() )
      return CommState::CONNECTED;
    if ( !NimBLEDevice::isInitialized() )
      return CommState::ERROR;
    return CommState::DISCONNECTED;
  }

  int8_t getRSSI( const NimBLEAddress & ) const override
  {
    return client_ ? client_->getRssi() : 0;
  }

  void update() override;

  void readProperty( uint8_t id, std::vector<uint8_t> &data, unsigned long &age_ms ) const override;
  void setProperty( uint8_t id, const std::vector<uint8_t> &data ) override;

private:
  enum class ClientState { DISCONNECTED, CONNECTING, CONNECTED };
  struct CharacteristicInfo {
    std::vector<uint8_t> data;
    NimBLEUUID id;
    elapsedMillis last_message;
    bool subscribed = false;
  };

  // BLEAdvertisedDeviceCallbacks::onResult
  void onResult( const NimBLEAdvertisedDevice *device ) override;

  void onConnect( NimBLEClient *client ) override;

  void onPassKeyEntry( NimBLEConnInfo &conn_info ) override;

  void onConnectFail( NimBLEClient *client, int reason ) override;

  void onDisconnect( NimBLEClient *client, int reason ) override;

  const CharacteristicInfo &
  getOrAddCharacteristicInfo( const NimBLEUUID &uuid,
                              NimBLERemoteCharacteristic *characteristic ) const;

  const std::string server_name_;
  NimBLEAddress server_address_;
  NimBLEScan *scan_ = nullptr;
  NimBLEClient *client_ = nullptr;
  NimBLERemoteService *service_ = nullptr;
  mutable std::vector<CharacteristicInfo> characteristics_;
  ClientState state_ = ClientState::DISCONNECTED;
  elapsedMillis scan_duration_;
  int get_service_tries_ = 0;
};