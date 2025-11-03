#include "deadman_comm_interface.h"
#include "ble_client_interface.h"
#include "ble_server_interface.h"
#include "esp_now_interface.h"

#include <elapsedMillis.h>

inline uint8_t to_uint8_t( bool value ) { return value ? 0xff : 0; }

class DeadmanCommInterface::Impl
{
public:
  Impl( BLEInterface *ble_interface, const CommPeerInfo peer_info );

  void update()
  {
    if ( ble_interface != nullptr ) {
      ble_interface->update();
    }
    esp_now_interface.update();

    updateDeadmanStates();

    if ( last_status_update_time > 500 ) {
      last_status_update_time = 0;
      if ( ble_interface != nullptr ) {
        status.ble_state = ble_interface->getCommState( peer_ble_address );
        status.ble_rssi = ble_interface->getRSSI( peer_ble_address );
      } else {
        status.ble_state = CommState::DISCONNECTED;
        status.ble_rssi = 0.0f;
      }
      status.esp_now_state = esp_now_interface.getCommState();
      status.esp_now_rssi = esp_now_interface.getRSSI();
      status.last_received_message_age_ms = last_transmit;
    }
  }

  void setProperty( uint8_t id, const std::vector<uint8_t> &data )
  {
    if ( ble_interface != nullptr ) {
      ble_interface->setProperty( id, data );
    }
    esp_now_interface.setProperty( id, data );
  }

  void updateStateIfNewer( unsigned long &most_recent_age, bool &state,
                           const std::vector<uint8_t> &data, unsigned long age_ms )
  {
    if ( age_ms < most_recent_age ) {
      most_recent_age = age_ms;
      state = readState( data );
    }
  }

  void updateDeadmanStates()
  {
    unsigned long most_recent_age_active = ULONG_MAX;
    unsigned long most_recent_age_triggered = ULONG_MAX;
    bool is_active_state = is_active_;
    bool is_triggered_state = is_triggered_;
    if ( ble_interface != nullptr &&
         ble_interface->getCommState( peer_ble_address ) == CommState::CONNECTED ) {
      unsigned long age_ms;
      ble_interface->readProperty( COMM_PROPERTY_ID_DEADMAN_ACTIVE, data, age_ms );
      updateStateIfNewer( most_recent_age_active, is_active_state, data, age_ms );
      ble_interface->readProperty( COMM_PROPERTY_ID_DEADMAN_TRIGGERED, data, age_ms );
      updateStateIfNewer( most_recent_age_triggered, is_triggered_state, data, age_ms );
    }
    if ( esp_now_interface.getCommState() == CommState::CONNECTED ) {
      unsigned long age_ms;
      esp_now_interface.readProperty( COMM_PROPERTY_ID_DEADMAN_ACTIVE, data, age_ms );
      updateStateIfNewer( most_recent_age_active, is_active_state, data, age_ms );
      esp_now_interface.readProperty( COMM_PROPERTY_ID_DEADMAN_TRIGGERED, data, age_ms );
      updateStateIfNewer( most_recent_age_triggered, is_triggered_state, data, age_ms );
    }
    is_triggered_ = most_recent_age_triggered > 300 ? true : is_triggered_state;
    is_active_ = is_active_state;
    last_transmit = std::min<unsigned long>( most_recent_age_active, last_transmit );
    last_transmit = std::min<unsigned long>( most_recent_age_triggered, last_transmit );
  }

  bool readState( const std::vector<uint8_t> &data ) const { return data.empty() || data[0] != 0; }

  CommStatus status;
  elapsedMillis last_status_update_time;
  elapsedMillis last_estop_transmission_time;
  bool is_active_ = false;
  bool is_triggered_ = false;

  ESPNowInterface esp_now_interface;
  BLEInterface *ble_interface;
  NimBLEAddress peer_ble_address;
  std::vector<uint8_t> data;
  elapsedMillis last_transmit = 1000000;
};

DeadmanCommInterface::Impl *DeadmanCommInterface::impl_ = nullptr;

DeadmanCommInterface::DeadmanCommInterface() { }

void DeadmanCommInterface::initialize( BLEInterface *ble_interface, const CommPeerInfo &peer_info )
{
  if ( impl_ != nullptr )
    return;
  impl_ = new DeadmanCommInterface::Impl( ble_interface, peer_info );
}

void DeadmanCommInterface::initialize( const CommPeerInfo &peer_info )
{
  if ( impl_ != nullptr )
    return;
  // Setup BLE
  Serial.println( "Initializing BLE in client mode..." );
  initialize( new BLEClientInterface( ESTOP_BLE_NAME, NimBLEAddress( peer_info.ble_mac, 0 ) ),
              peer_info );
  Serial.print( "BLE Device initialized with address: " );
  Serial.println( BLEDevice::getAddress().toString().c_str() );
}

DeadmanCommInterface::~DeadmanCommInterface() = default;

CommStatus DeadmanCommInterface::update()
{
  impl_->update();
  return impl_->status;
}

void DeadmanCommInterface::setActive( bool active )
{
  impl_->is_active_ = active;
  impl_->setProperty( COMM_PROPERTY_ID_DEADMAN_ACTIVE, { to_uint8_t( active ) } );
}

void DeadmanCommInterface::setTriggered( bool active )
{
  impl_->is_triggered_ = active;
  impl_->setProperty( COMM_PROPERTY_ID_DEADMAN_TRIGGERED, { to_uint8_t( active ) } );
}

bool DeadmanCommInterface::isActive() const { return impl_ ? impl_->is_active_ : false; }

bool DeadmanCommInterface::isTriggered() const
{
  return impl_ ? impl_->is_active_ && impl_->is_triggered_ : false;
}

// =========================================================================
// ============= Implementation of DeadmanCommInterface::Impl ==============
// =========================================================================

DeadmanCommInterface::Impl::Impl( BLEInterface *ble_interface, const CommPeerInfo peer_info )
    : esp_now_interface( peer_info.esp_now_mac ), ble_interface( ble_interface ),
      peer_ble_address( peer_info.ble_mac, 0 )
{
}
