#include "lora_interface.h"

#include <RadioLib.h>
#define RADIO_BOARD_AUTO
#include <RadioBoards.h>

#include <elapsedMillis.h>

class LoraInterface::Impl
{
public:
  Impl( bool is_server );

  void update()
  {
    if ( is_server ) {
      updateServer();
    } else {
      updateClient();
    }
  }

  void updateServer();

  void updateClient();

  void sendData( const std::vector<uint8_t> &data )
  {
    operation_done = false;
    radio_status = radio.startTransmit( data.data(), data.size() );
    last_send_time = 0;
    if ( radio_status != RADIOLIB_ERR_NONE ) {
      static elapsedMillis last_error_print = 5000;
      if ( last_error_print > 2000 ) {
        last_error_print = 0;
        Serial.printf( "Radio transmit error: %d\n", radio_status );
      }
      operation_done = true;
      return;
    }
  }

  void setProperty( uint8_t id, const std::vector<uint8_t> &data )
  {
    if ( id == COMM_PROPERTY_ID_ESTOP ) {
      estop_data.clear();
      estop_data.push_back( id ); // Store the property ID as the first byte
      estop_data.insert( estop_data.end(), data.begin(), data.end() );
    } else if ( id == COMM_PROPERTY_ID_SOFT_ESTOP ) {
      soft_estop_data.clear();
      soft_estop_data.push_back( id ); // Store the property ID as the first byte
      soft_estop_data.insert( soft_estop_data.end(), data.begin(), data.end() );
    }
    // Other property IDs are ignored due to bandwidth limitations
  }

  void readProperty( uint8_t id, std::vector<uint8_t> &data, unsigned long &age_ms ) const
  {
    data.clear();
    age_ms = ULONG_MAX; // Invalid property ID
    if ( id == COMM_PROPERTY_ID_ESTOP ) {
      if ( !estop_data.empty() ) {
        data.assign( estop_data.begin() + 1, estop_data.end() ); // Skip the first byte (property ID)
        age_ms = estop_data_age;
      }
    } else if ( id == COMM_PROPERTY_ID_SOFT_ESTOP ) {
      if ( !soft_estop_data.empty() ) {
        data.assign( soft_estop_data.begin() + 1,
                     soft_estop_data.end() ); // Skip the first byte (property ID)
        age_ms = soft_estop_data_age;
      }
    }
  }

  uint8_t buffer[256];
  Radio radio = new Module( RADIO_NSS, RADIO_IRQ, RADIO_RST, RADIO_GPIO );
  int radio_status = RADIOLIB_ERR_UNKNOWN;
  std::vector<uint8_t> estop_data;
  std::vector<uint8_t> soft_estop_data;
  elapsedMillis last_packet_received_time;
  elapsedMillis last_send_time;
  elapsedMillis estop_data_age = 0;
  elapsedMillis soft_estop_data_age = 0;

  volatile bool operation_done = false;
  bool is_server = false;
};

LoraInterface::Impl *LoraInterface::impl_ = nullptr;

IRAM_ATTR void setDoneFlag( void ) { LoraInterface::impl_->operation_done = true; }

LoraInterface::Impl::Impl( bool is_server ) : is_server( is_server )
{
  // These settings result in approx 9 packages per second
  radio_status = radio.begin( 868, 125, 9, 7, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, 20 );
  if ( radio_status != RADIOLIB_ERR_NONE ) {
    Serial.printf( "Radio initialization error: %d\n", radio_status );
    return;
  }
  radio.setDio1Action( setDoneFlag );
  if ( !is_server ) {
    radio_status = radio.startReceive();
  }
}

LoraInterface::LoraInterface( bool is_server )
{
  if ( impl_ != nullptr )
    return;
  impl_ = new Impl( is_server );
}

unsigned long LoraInterface::getLastReceivedMessageAge() const
{
  return impl_->last_packet_received_time;
}

void LoraInterface::setProperty( uint8_t id, const std::vector<uint8_t> &data )
{
  impl_->setProperty( id, data );
}

void LoraInterface::readProperty( uint8_t id, std::vector<uint8_t> &data, unsigned long &age_ms ) const
{
  impl_->readProperty( id, data, age_ms );
}

void LoraInterface::update() { impl_->update(); }

CommState LoraInterface::getCommState() const
{
  if ( impl_->radio_status != RADIOLIB_ERR_NONE ) {
    return CommState::ERROR;
  }
  if (impl_->is_server) return CommState::CONNECTED; // Server always connected
  return impl_->last_packet_received_time < 500 ? CommState::CONNECTED : CommState::DISCONNECTED;
}

float LoraInterface::getRSSI() const { return impl_->radio.getRSSI(); }

void LoraInterface::Impl::updateServer()
{
  if ( !operation_done ) {
    if ( last_send_time < 200 )
      return;
    static elapsedMillis last_error_print = 5000;
    if ( last_error_print > 2000 ) {
      last_error_print = 0;
      Serial.printf( "Sent was never set to true." );
    }
    radio.finishTransmit();
  }
  sendData( estop_data ); // Resend the last packet
}

void LoraInterface::Impl::updateClient()
{
  if ( !operation_done ) {
    return;
  }
  operation_done = false;
  int len = radio.getPacketLength();
  if ( len <= 0 )
    return;
  int result = radio.readData( buffer, len );
  if ( result != RADIOLIB_ERR_NONE ) {
    Serial.printf( "Radio read error: %d\n", result );
    return;
  }
  int id = buffer[0];
  if ( id == COMM_PROPERTY_ID_ESTOP ) {
    estop_data.assign( buffer, buffer + len );
    estop_data_age = 0; // Reset age on valid packet
  } else if ( id == COMM_PROPERTY_ID_SOFT_ESTOP ) {
    soft_estop_data.assign( buffer, buffer + len );
    soft_estop_data_age = 0; // Reset age on valid packet
  } else {
    Serial.printf( "Received unknown property ID: %d\n", id );
    return;
  }
  last_packet_received_time = 0;
}
