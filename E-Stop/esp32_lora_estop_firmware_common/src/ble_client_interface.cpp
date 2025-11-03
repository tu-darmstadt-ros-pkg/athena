#include "ble_client_interface.h"
#include "comm_interface.h"
#include <elapsedMillis.h>

BLEClientInterface::BLEClientInterface( const std::string &server_name, NimBLEAddress server_address )
    : server_name_( server_name ), server_address_( server_address )
{
  // Client mode, just initialize BLE without creating server
  NimBLEDevice::init( "" );
  // NimBLEDevice::setPower( 7 ); // Set power to maximum (20 dBm)
#if ESTOP_BLE_ENCRYPTION
  NimBLEDevice::setSecurityAuth( true, true, true );
  NimBLEDevice::setSecurityPasskey( ESTOP_BLE_PASSKEY );
  NimBLEDevice::setSecurityIOCap( BLE_HS_IO_KEYBOARD_ONLY );
#endif
  scan_ = NimBLEDevice::getScan();
  scan_->setScanCallbacks( this, true );
  scan_->setActiveScan( true );
  scan_->setInterval( 100 );
  scan_->setWindow( 99 );
  scan_->start( 0, false, false );
  scan_duration_ = 0;
}

void BLEClientInterface::update()
{
  switch ( state_ ) {
  case ClientState::CONNECTED:
    return;
  case ClientState::DISCONNECTED:
    if ( scan_duration_ > 10000 ) { // Restart scanning every 10 seconds
      scan_duration_ = 0;
      scan_->stop();
    }
    if ( !scan_->isScanning() ) {
      scan_->clearResults();
      scan_->start( 0, false, false ); // Restart scanning if not already scanning
    }
    break;
  case ClientState::CONNECTING: {
    if ( client_ == nullptr || !client_->isConnected() )
      return; // Wait for connection
#if ESTOP_BLE_ENCRYPTION
    client_->secureConnection();
#endif
    NimBLERemoteService *service = client_->getService( NimBLEUUID( ESTOP_SERVICE_UUID ) );
    if ( service != nullptr ) {
      service_ = service; // Store the service for later use
      state_ = ClientState::CONNECTED;
      Serial.println( "BLE server connected" );
      return;
    }
    if ( ++get_service_tries_ < 5 )
      return; // Retry up to 5 times
    Serial.println( "Failed to find service" );
    client_->disconnect();
  }
  }
}

void BLEClientInterface::setProperty( uint8_t id, const std::vector<uint8_t> &data )
{
  if ( state_ != ClientState::CONNECTED || service_ == nullptr ) {
    return;
  }
  service_->setValue( NimBLEUUID( uint16_t( id ) ), data );
}

void BLEClientInterface::readProperty( uint8_t id, std::vector<uint8_t> &data,
                                       unsigned long &age_ms ) const
{
  data.clear();
  age_ms = ULONG_MAX;
  if ( state_ != ClientState::CONNECTED || service_ == nullptr ) {
    return;
  }
  NimBLERemoteCharacteristic *characteristic =
      service_->getCharacteristic( NimBLEUUID( uint16_t( id ) ) );
  if ( characteristic == nullptr ) {
    return;
  }
  const auto &info = getOrAddCharacteristicInfo( characteristic->getUUID(), characteristic );
  data = info.data; // Copy the data from the characteristic info
  age_ms = info.last_message;
}

void BLEClientInterface::onResult( const NimBLEAdvertisedDevice *device )
{
  if ( device->getAddress() != server_address_ ) {
    return; // Ignore devices that are not the target
  }
  if ( client_ != nullptr && client_->isConnected() )
    return;
  if ( client_ == nullptr ) {
    client_ = NimBLEDevice::createClient( device->getAddress() );
    client_->setClientCallbacks( this, false );
  }
  client_->connect( true, true, true );
}

void BLEClientInterface::onConnect( NimBLEClient *client )
{
  Serial.println( "onConnect" );
  state_ = ClientState::CONNECTING;
  scan_->stop();
}

void BLEClientInterface::onPassKeyEntry( NimBLEConnInfo &conn_info )
{
  Serial.printf( "Passkey entry requested.\n" );
  NimBLEDevice::injectPassKey( conn_info, ESTOP_BLE_PASSKEY ); // Inject the passkey
}

void BLEClientInterface::onConnectFail( NimBLEClient *client, int reason )
{
  Serial.printf( "Failed to connect to BLE server: %d\n", reason );
  state_ = ClientState::DISCONNECTED;
}

void BLEClientInterface::onDisconnect( NimBLEClient *client, int reason )
{
  Serial.println( "BLE server disconnected" );
  state_ = ClientState::DISCONNECTED;
  service_ = nullptr;
  characteristics_.clear();
}

const BLEClientInterface::CharacteristicInfo &
BLEClientInterface::getOrAddCharacteristicInfo( const NimBLEUUID &uuid,
                                                NimBLERemoteCharacteristic *characteristic ) const
{
  NimBLERemoteCharacteristic::notify_callback callback =
      [this, uuid]( NimBLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData,
                    size_t length, bool isNotify ) {
        for ( auto &info : characteristics_ ) {
          if ( info.id == uuid ) {
            info.last_message = 0;
            info.data.assign( pData, pData + length ); // Store the received data
            break;
          }
        }
      };
  for ( auto &info : characteristics_ ) {
    if ( info.id == uuid ) {
      if ( !info.subscribed ) {
        info.subscribed = characteristic->subscribe( true, callback, true );
      }
      return info; // Found existing characteristic
    }
  }
  // Not found, add new characteristic info
  CharacteristicInfo info;
  info.id = uuid;
  info.subscribed = characteristic->subscribe( true, callback, true );
  characteristics_.emplace_back( info );
  return characteristics_.back();
}
