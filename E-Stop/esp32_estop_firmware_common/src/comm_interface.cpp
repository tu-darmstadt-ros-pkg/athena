#include "comm_interface.h"
#include "ble_client_interface.h"
#include "ble_server_interface.h"
#include "esp_now_interface.h"

#include <elapsedMillis.h>
#include <esp_wifi.h>

inline uint8_t to_uint8_t( bool value ) { return value ? 0xff : 0; }

class CommInterface::Impl
{
public:
  Impl( bool is_remote, const CommPeerInfo peer_info );

  void update()
  {
    if ( ble_interface != nullptr ) {
      ble_interface->update();
    }
    esp_now_interface->update();

    if ( !is_remote ) {
      updateEStopStates();
    }

    if ( last_status_update_time > 500 ) {
      last_status_update_time = 0;
      if ( ble_interface != nullptr ) {
        status.ble_state = ble_interface->getCommState( peer_ble_address );
        status.ble_rssi = ble_interface->getRSSI( peer_ble_address );
      } else {
        status.ble_state = CommState::DISCONNECTED;
        status.ble_rssi = 0.0f;
      }
      status.esp_now_state = esp_now_interface->getCommState();
      status.esp_now_rssi = esp_now_interface->getRSSI();
      status.last_received_message_age_ms = last_transmit;
      if ( ble_interface != nullptr ) {
        status.ble_latency_ms = ble_interface->getLatency();
      }
      status.esp_now_latency_ms = esp_now_interface->getLatency();
    }
  }

  void setProperty( uint8_t id, const std::vector<uint8_t> &data )
  {
    if ( ble_interface != nullptr ) {
      ble_interface->setProperty( id, data );
    }
    esp_now_interface->setProperty( id, data );
  }

  void updateEStopStateIfNewer( unsigned long &most_recent_age, bool &estop_state,
                                const std::vector<uint8_t> &data, unsigned long age_ms )
  {
    if ( age_ms < most_recent_age ) {
      most_recent_age = age_ms;
      estop_state = readEStopstate( data );
    }
  }

  void updateEStopStates()
  {
    unsigned long most_recent_age_estop = ULONG_MAX;
    unsigned long most_recent_age_soft_estop = ULONG_MAX;
    bool estop_state = true;
    bool soft_estop_state = soft_estop_active_; // Default to current state if no messages received
    bool ble_estop_state = true;
    bool esp_now_estop_state = true;
    unsigned long age_ms_ble;
    unsigned long age_ms_esp_now;
    if ( ble_interface != nullptr &&
         ble_interface->getCommState( peer_ble_address ) == CommState::CONNECTED ) {
      unsigned long age_ms;
      ble_interface->readProperty( COMM_PROPERTY_ID_ESTOP, data, age_ms );
      ble_estop_state = readEStopstate( data );
      age_ms_ble = age_ms;
      updateEStopStateIfNewer( most_recent_age_estop, estop_state, data, age_ms );
      ble_interface->readProperty( COMM_PROPERTY_ID_SOFT_ESTOP, data, age_ms );
      // Skip if the property has never been received: empty data would otherwise
      // be interpreted as active by readEStopstate, but soft E-Stop must NOT
      // fail-safe to active.
      if ( !data.empty() ) {
        updateEStopStateIfNewer( most_recent_age_soft_estop, soft_estop_state, data, age_ms );
      }
    }
    if ( esp_now_interface->getCommState() == CommState::CONNECTED ) {
      unsigned long age_ms;
      esp_now_interface->readProperty( COMM_PROPERTY_ID_ESTOP, data, age_ms );
      esp_now_estop_state = readEStopstate( data );
      age_ms_esp_now = age_ms;
      updateEStopStateIfNewer( most_recent_age_estop, estop_state, data, age_ms );
      esp_now_interface->readProperty( COMM_PROPERTY_ID_SOFT_ESTOP, data, age_ms );
      if ( !data.empty() ) {
        updateEStopStateIfNewer( most_recent_age_soft_estop, soft_estop_state, data, age_ms );
      }
    }
    // static elapsedMillis last_print;
    // if ( last_print > 1000 ) {
    //   last_print = 0;
    //   Serial.printf( "E-Stop states: BLE: %d (%dms), ESP-NOW: %d (%dms)\n",
    //                  ble_estop_state, age_ms_ble,
    //                  esp_now_estop_state, age_ms_esp_now );
    // }
    estop_active_ = most_recent_age_estop > SAFETY_TIMEOUT_MS ? true : estop_state;
    soft_estop_active_ = soft_estop_state;
    if ( estop_active_ ) {
      estop_inactive_time = 0;
    }
    if ( soft_estop_active_ ) {
      soft_estop_inactive_time = 0;
    }
    last_transmit = std::min<unsigned long>( most_recent_age_estop, last_transmit );
    last_transmit = std::min<unsigned long>( most_recent_age_soft_estop, last_transmit );
  }

  bool readEStopstate( const std::vector<uint8_t> &data ) const
  {
    return data.empty() || data[0] != 0;
  }

  bool isEstopActive() const { return estop_active_ || estop_inactive_time < 500; }

  bool isSoftEstopActive() const { return soft_estop_active_ || soft_estop_inactive_time < 200; }

  CommStatus status;
  elapsedMillis last_status_update_time;
  elapsedMillis last_estop_transmission_time;
  elapsedMillis estop_inactive_time;
  elapsedMillis soft_estop_inactive_time;
  bool estop_active_ = false;
  bool soft_estop_active_ = false;

  std::unique_ptr<BLEInterface> ble_interface;
  std::unique_ptr<ESPNowInterface> esp_now_interface;
  std::vector<uint8_t> data;
  elapsedMillis last_transmit = 1000000;
  NimBLEAddress peer_ble_address;

  bool is_remote;
};

CommInterface::Impl *CommInterface::impl_ = nullptr;

CommInterface::CommInterface() { }

void CommInterface::initialize( CommMode mode, const CommPeerInfo &peer_info )
{
  if ( impl_ != nullptr )
    return;
  impl_ = new CommInterface::Impl( mode == CommMode::SERVER, peer_info );
}

CommInterface::~CommInterface() = default;

CommStatus CommInterface::update()
{
  impl_->update();
  return impl_->status;
}

void CommInterface::setEStopState( bool active )
{
  impl_->estop_active_ = active;
  impl_->setProperty( COMM_PROPERTY_ID_ESTOP, { to_uint8_t( active ) } );
}

void CommInterface::setSoftEStopState( bool active )
{
  impl_->soft_estop_active_ = active;
  impl_->setProperty( COMM_PROPERTY_ID_SOFT_ESTOP, { to_uint8_t( active ) } );
}

bool CommInterface::getEStopState() const { return impl_ ? impl_->isEstopActive() : false; }

bool CommInterface::getSoftEStopState() const { return impl_ ? impl_->isSoftEstopActive() : false; }

void CommInterface::reportBatteryLevel( uint8_t level )
{
  std::vector<uint8_t> battery_data = { level };
  impl_->setProperty( COMM_PROPERTY_ID_BATTERY, battery_data );
}

BLEInterface *CommInterface::getBLEInterface() { return impl_->ble_interface.get(); }

// ==================================================================
// ============= Implementation of CommInterface::Impl ==============
// ==================================================================

CommInterface::Impl::Impl( bool is_server, const CommPeerInfo peer_info )
    : is_remote( !is_server ), peer_ble_address( peer_info.ble_mac, 0 )
{
  // Setup BLE first so the coexistence controller is active before WiFi channel is set
  if ( is_server ) {
    Serial.println( "Initializing BLE in server mode..." );
    ble_interface.reset( new BLEServerInterface( ESTOP_BLE_NAME ) );
  } else {
    Serial.println( "Initializing BLE in client mode..." );
    ble_interface.reset(
        new BLEClientInterface( ESTOP_BLE_NAME, NimBLEAddress( peer_info.ble_mac, 0 ) ) );
  }
  Serial.printf( "BLE Device initialized with address: %s\n",
                 BLEDevice::getAddress().toString().c_str() );

  // Setup ESP-NOW after BLE so WiFi channel is set with the coexistence controller already active
  esp_now_interface = std::make_unique<ESPNowInterface>( peer_info.esp_now_mac );
}
