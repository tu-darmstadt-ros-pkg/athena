#include "comm_interface.h"
#include "ble_client_interface.h"
#include "ble_server_interface.h"
#include "esp_now_interface.h"
#include "lora_interface.h"

#include <elapsedMillis.h>

inline uint8_t to_uint8_t( bool value ) { return value ? 0xff : 0; }

class CommInterface::Impl
{
public:
  Impl( bool is_remote, const CommPeerInfo peer_info );

  void update()
  {
    lora_interface.update();

    if ( ble_interface != nullptr ) {
      ble_interface->update();
    }
    esp_now_interface.update();

    if ( !is_remote ) {
      updateEStopStates();
    }

    if ( last_status_update_time > 500 ) {
      last_status_update_time = 0;
      status.radio_state = lora_interface.getCommState();
      status.radio_rssi = lora_interface.getRSSI();
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
    lora_interface.setProperty( id, data );
    if ( ble_interface != nullptr ) {
      ble_interface->setProperty( id, data );
    }
    esp_now_interface.setProperty( id, data );
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
    unsigned long moest_recent_age_soft_estop = ULONG_MAX;
    bool estop_state = true;
    bool soft_estop_state = true;
    bool lora_estop_state = true;
    bool ble_estop_state = true;
    bool esp_now_estop_state = true;
    unsigned long age_ms_lora;
    unsigned long age_ms_ble;
    unsigned long age_ms_esp_now;
    if ( lora_interface.getCommState() == CommState::CONNECTED ) {
      unsigned long age_ms;
      lora_interface.readProperty( COMM_PROPERTY_ID_ESTOP, data, age_ms );
      lora_estop_state = readEStopstate( data );
      age_ms_lora = age_ms;
      updateEStopStateIfNewer( most_recent_age_estop, estop_state, data, age_ms );
      lora_interface.readProperty( COMM_PROPERTY_ID_SOFT_ESTOP, data, age_ms );
      updateEStopStateIfNewer( moest_recent_age_soft_estop, soft_estop_state, data, age_ms );
      // Compensate sending duration
      most_recent_age_estop += 120;
      moest_recent_age_soft_estop += 120;
    }
    if ( ble_interface != nullptr &&
         ble_interface->getCommState( peer_ble_address ) == CommState::CONNECTED ) {
      unsigned long age_ms;
      ble_interface->readProperty( COMM_PROPERTY_ID_ESTOP, data, age_ms );
      ble_estop_state = readEStopstate( data );
      age_ms_ble = age_ms;
      updateEStopStateIfNewer( most_recent_age_estop, estop_state, data, age_ms );
      ble_interface->readProperty( COMM_PROPERTY_ID_SOFT_ESTOP, data, age_ms );
      updateEStopStateIfNewer( moest_recent_age_soft_estop, soft_estop_state, data, age_ms );
    }
    if ( esp_now_interface.getCommState() == CommState::CONNECTED ) {
      unsigned long age_ms;
      esp_now_interface.readProperty( COMM_PROPERTY_ID_ESTOP, data, age_ms );
      esp_now_estop_state = readEStopstate( data );
      age_ms_esp_now = age_ms;
      updateEStopStateIfNewer( most_recent_age_estop, estop_state, data, age_ms );
      esp_now_interface.readProperty( COMM_PROPERTY_ID_SOFT_ESTOP, data, age_ms );
      updateEStopStateIfNewer( moest_recent_age_soft_estop, soft_estop_state, data, age_ms );
    }
    // static elapsedMillis last_print;
    // if ( last_print > 1000 ) {
    //   last_print = 0;
    //   Serial.printf( "E-Stop states: Lora: %d (%dms), BLE: %d (%dms), ESP-NOW: %d (%dms)\n",
    //                  lora_estop_state, age_ms_lora, ble_estop_state, age_ms_ble,
    //                  esp_now_estop_state, age_ms_esp_now );
    // }
    estop_active_ = most_recent_age_estop > 300 ? true : estop_state;
    soft_estop_active_ = soft_estop_state;
    last_transmit = std::min<unsigned long>( most_recent_age_estop, last_transmit );
    last_transmit = std::min<unsigned long>( moest_recent_age_soft_estop, last_transmit );
  }

  bool readEStopstate( const std::vector<uint8_t> &data ) const
  {
    return data.empty() || data[0] != 0;
  }

  CommStatus status;
  elapsedMillis last_status_update_time;
  elapsedMillis last_estop_transmission_time;
  bool estop_active_ = false;
  bool soft_estop_active_ = false;

  ESPNowInterface esp_now_interface;
  LoraInterface lora_interface;
  std::unique_ptr<BLEInterface> ble_interface;
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

bool CommInterface::getEStopState() const { return impl_ ? impl_->estop_active_ : false; }

bool CommInterface::getSoftEStopState() const { return impl_ ? impl_->soft_estop_active_ : false; }

void CommInterface::reportBatteryLevel( uint8_t level )
{
  std::vector<uint8_t> battery_data = { level };
  impl_->setProperty( COMM_PROPERTY_ID_BATTERY, battery_data );
}

BLEInterface *CommInterface::getBLEInterface() { return impl_->ble_interface.get(); }

// ==================================================================
// ====deadman_comm.isActive()========= Implementation of CommInterface::Impl ==============
// ==================================================================

// For Lora is_server is switched as there the remote is the server
CommInterface::Impl::Impl( bool is_server, const CommPeerInfo peer_info )
    : esp_now_interface( peer_info.esp_now_mac ), lora_interface( !is_server ),
      is_remote( !is_server ), peer_ble_address( peer_info.ble_mac, 0 )
{
  // Setup BLE
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

  // Setup ESP-NOW
}
