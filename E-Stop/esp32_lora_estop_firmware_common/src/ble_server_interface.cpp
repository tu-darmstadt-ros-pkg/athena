#include "ble_server_interface.h"
#include "comm_interface.h"

BLEServerInterface::BLEServerInterface( const std::string &name )
{
  NimBLEDevice::init( name );
  // NimBLEDevice::setPower( 20 ); // Set power to maximum (20 dBm)
#if ESTOP_BLE_ENCRYPTION
  NimBLEDevice::setSecurityAuth( true, true, true );
  NimBLEDevice::setSecurityPasskey( ESTOP_BLE_PASSKEY );
  NimBLEDevice::setSecurityIOCap( BLE_HS_IO_DISPLAY_ONLY );
#endif

  server_ = NimBLEDevice::createServer();
  server_->setCallbacks( this );
  service_ = server_->createService( NimBLEUUID( ESTOP_SERVICE_UUID ) ); // Battery Service UUID

  for ( uint16_t characteristic_uuid : COMM_PROPERTY_UUIDS ) {
#if ESTOP_BLE_ENCRYPTION
    NimBLECharacteristic *characteristic = service_->createCharacteristic(
        NimBLEUUID( characteristic_uuid ),
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::READ_AUTHEN |
            NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::WRITE_AUTHEN |
            NIMBLE_PROPERTY::NOTIFY );
#else
    NimBLECharacteristic *characteristic = service_->createCharacteristic(
        NimBLEUUID( characteristic_uuid ),
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY );
#endif
    characteristic->setCallbacks( this );
    service_->addCharacteristic( characteristic );
  }
  service_->start();

  advertising_ = server_->getAdvertising();
  advertising_->setName( name );
  advertising_->addServiceUUID( NimBLEUUID( ESTOP_SERVICE_UUID ) );
  advertising_->enableScanResponse( false );
  advertising_->start( 0 );
}

CommState BLEServerInterface::getCommState( const NimBLEAddress &address ) const
{
  if ( !NimBLEDevice::isInitialized() )
    return CommState::ERROR;
  for ( const NimBLEConnInfo &conn_info : connected_clients_ ) {
    if ( conn_info.getAddress() == address ) {
      return CommState::CONNECTED;
    }
  }
  return CommState::DISCONNECTED;
}

int8_t BLEServerInterface::getRSSI( const NimBLEAddress &address ) const
{
  for ( const NimBLEConnInfo &conn_info : connected_clients_ ) {
    if ( conn_info.getAddress() == address ) {
      int8_t rssi = 0;
      if ( ble_gap_conn_rssi( conn_info.getConnHandle(), &rssi ) != 0 ) {
        Serial.printf( "Failed to read RSSI for %s\n", address.toString().c_str() );
        return 0.0f;
      }
      return static_cast<float>( rssi );
    }
  }
  return 0.0f;
}

void BLEServerInterface::update()
{
  static elapsedMillis last_advertising_check;
  if ( last_advertising_check > 500 ) {
    last_advertising_check = 0;
    if ( !advertising_->isAdvertising() ) {
      advertising_->start( 0 );
    }
  }
}

void BLEServerInterface::setProperty( uint8_t id, const std::vector<uint8_t> &data )
{
  if ( service_ == nullptr ) {
    Serial.println( "Not connected or characteristic not found" );
    return;
  }

  NimBLECharacteristic *characteristic = service_->getCharacteristic( NimBLEUUID( uint16_t( id ) ) );
  if ( characteristic == nullptr ) {
    Serial.printf( "Characteristic %d not found\n", id );
    return;
  }
  characteristic->setValue( data );
  characteristic->notify();
}

void BLEServerInterface::readProperty( uint8_t id, std::vector<uint8_t> &data,
                                       unsigned long &age_ms ) const
{
  if ( service_ == nullptr ) {
    Serial.println( "Not connected or characteristic not found" );
    return;
  }
  NimBLECharacteristic *characteristic = service_->getCharacteristic( NimBLEUUID( uint16_t( id ) ) );
  if ( characteristic == nullptr ) {
    Serial.printf( "Characteristic %d not found\n", id );
    return;
  }
  NimBLEAttValue value = characteristic->getValue();
  data.assign( value.begin(), value.end() );
  time_t now = time( nullptr );
  age_ms = now < value.getTimeStamp() ? 0 : now - value.getTimeStamp();
}

void BLEServerInterface::onConnect( NimBLEServer *server, NimBLEConnInfo &conn_info )
{
  Serial.printf( "BLE client (%s) connected\n", conn_info.getAddress().toString().c_str() );
  auto it = std::find( BLE_WHITELIST.begin(), BLE_WHITELIST.end(), conn_info.getAddress() );
  if ( it == BLE_WHITELIST.end() ) {
    Serial.printf( "Connected to unexpected address: %s\n",
                   conn_info.getAddress().toString().c_str() );
    server->disconnect( conn_info.getConnHandle() );
    return;
  }
  auto cit = std::find_if(
      connected_clients_.begin(), connected_clients_.end(),
      [&]( const NimBLEConnInfo &info ) { return info.getAddress() == conn_info.getAddress(); } );
  if ( cit != connected_clients_.end() ) {
    Serial.println( "Client already in connected clients list" );
    return;
  }
  connected_clients_.push_back( conn_info );
}

void BLEServerInterface::onDisconnect( NimBLEServer *server, NimBLEConnInfo &conn_info, int reason )
{
  Serial.printf( "BLE client (%s) disconnected\n", conn_info.getAddress().toString().c_str() );
  NimBLEClient *client = server->getClient( conn_info );
  auto it = std::find_if(
      connected_clients_.begin(), connected_clients_.end(),
      [&]( const NimBLEConnInfo &info ) { return info.getAddress() == conn_info.getAddress(); } );
  if ( it != connected_clients_.end() ) {
    connected_clients_.erase( it );
  }
}

void BLEServerInterface::onRead( NimBLECharacteristic *characteristic, NimBLEConnInfo &conn_info )
{
  // Handle read request, if needed
}

void BLEServerInterface::onWrite( NimBLECharacteristic *characteristic, NimBLEConnInfo &conn_info )
{
  // Client can't write to this characteristic, so we ignore it
}
