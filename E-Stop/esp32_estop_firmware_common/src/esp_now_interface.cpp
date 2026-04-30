#include "esp_now_interface.h"
#include <WiFi.h>
#include <elapsedMillis.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <memory>

class ESPNowInterface::ESPNowConnection
{
public:
  ESPNowConnection( const esp_now_peer_info_t &peer_info ) : peer_info( peer_info ), rssi( 0 ) { }

  void onSent( const uint8_t *mac_addr, esp_now_send_status_t status )
  {
    if ( status != ESP_NOW_SEND_SUCCESS ) {
      transmission_failure_count++;
    } else {
      transmission_success_count++;
    }
  }

  void onReceived( const uint8_t *mac_addr, const uint8_t *data, int len );

  void writeProperty( uint8_t id, const std::vector<uint8_t> &data );

  esp_now_peer_info_t peer_info;
  elapsedMillis last_received_time = 100000;
  unsigned long transmission_success_count = 0;
  unsigned long transmission_failure_count = 0;
  int8_t rssi;
  struct Property {
    std::vector<uint8_t> data;
    elapsedMillis age_ms = 100000;
    bool updated = false;
    bool acknowledged = true;
    elapsedMillis send_time;
    bool awaiting_ack = false;
    int last_latency_ms = -1;
  };
  std::array<Property, NUM_COMM_PROPERTIES> properties;
  std::vector<uint8_t> send_buffer;
};

class ESPNowInterface::ESPNowManager
{
public:
  ESPNowManager();
  ~ESPNowManager() = default;
  ESPNowManager( const ESPNowManager & ) = delete;
  ESPNowManager &operator=( const ESPNowManager & ) = delete;
  ESPNowManager( ESPNowManager && ) = delete;
  ESPNowManager &operator=( ESPNowManager && ) = delete;

  std::shared_ptr<ESPNowInterface::ESPNowConnection> addConnection( const uint8_t peer_mac[6] )
  {
    if ( connection_count >= connections.size() ) {
      Serial.println( "Maximum number of ESP-NOW connections reached" );
      return nullptr;
    }
    esp_now_peer_info_t peer_info = {};
    std::copy( peer_mac, peer_mac + 6, peer_info.peer_addr );
    peer_info.channel = 0;
    peer_info.encrypt = false;

    // Add peer
    if ( esp_now_add_peer( &peer_info ) != ESP_OK ) {
      Serial.println( "Failed to add ESP-NOW peer" );
    } else {
      Serial.printf( "ESP-NOW peer (%02X:%02X:%02X:%02X:%02X:%02X) added successfully\n",
                     peer_info.peer_addr[0], peer_info.peer_addr[1], peer_info.peer_addr[2],
                     peer_info.peer_addr[3], peer_info.peer_addr[4], peer_info.peer_addr[5] );
    }
    auto connection = std::make_shared<ESPNowInterface::ESPNowConnection>( peer_info );
    connections[connection_count] = connection;
    connection_count++;
    return connection;
  }

  void removeConnection( std::shared_ptr<ESPNowInterface::ESPNowConnection> connection )
  {
    esp_now_del_peer( connection->peer_info.peer_addr );
    (void)std::remove( connections.begin(), connections.begin() + connection_count, connection );
    connections[connection_count - 1] = nullptr;
    connection_count--;
  }

  bool isBusy() const { return is_busy && busy_since < 5; }

  void setBusy( bool busy )
  {
    is_busy = busy;
    if ( busy ) {
      busy_since = 0;
    }
  }

  esp_err_t state = ESP_ERR_ESPNOW_NOT_INIT;
  std::array<std::shared_ptr<ESPNowInterface::ESPNowConnection>, 6> connections;
  int connection_count = 0;

private:
  elapsedMillis busy_since = 0;
  bool is_busy = false;
};

ESPNowInterface::ESPNowManager *ESPNowInterface::manager_ = nullptr;

ESPNowInterface::ESPNowInterface( const uint8_t peer_mac[6] )
{
  if ( manager_ == nullptr ) {
    manager_ = new ESPNowInterface::ESPNowManager();
  }
  connection_ = manager_->addConnection( peer_mac );
}

ESPNowInterface::~ESPNowInterface()
{
  // Note: We do not delete the manager_ here as it may be shared among multiple instances.
  // Proper cleanup of the manager_ should be handled at program termination if needed.
  manager_->removeConnection( connection_ );
}

void ESPNowInterface::update()
{
  for ( size_t index = 0; index < connection_->properties.size(); ++index ) {
    auto &property = connection_->properties[index];
    if ( manager_->isBusy() ) {
      return;
    }
    auto &send_buffer = connection_->send_buffer;
    if ( property.updated ) {
      manager_->setBusy( true );
      property.send_time = 0;
      property.awaiting_ack = true;
      send_buffer.clear();
      send_buffer.push_back( uint8_t( index ) );
      send_buffer.insert( send_buffer.end(), property.data.begin(), property.data.end() );

      // Result is checked in onSent callback. Can be ignored here.
      esp_err_t result =
          esp_now_send( connection_->peer_info.peer_addr, send_buffer.data(), send_buffer.size() );
      if ( result != ESP_OK ) {
        manager_->setBusy( false );
        connection_->transmission_failure_count++;

        static elapsedMillis last_print = 5000;
        if ( last_print > 2000 ) {
          last_print = 0;
          Serial.printf( "ESP-NOW send to %02X:%02X:%02X:%02X:%02X:%02X failed with result %s\n",
                         connection_->peer_info.peer_addr[0], connection_->peer_info.peer_addr[1],
                         connection_->peer_info.peer_addr[2], connection_->peer_info.peer_addr[3],
                         connection_->peer_info.peer_addr[4], connection_->peer_info.peer_addr[5],
                         esp_err_to_name( result ) );
        }
      } else {
        property.updated = false;
      }
    } else if ( !property.acknowledged ) {
      // Acknowledge the received packet (best-effort, no retry)
      property.acknowledged = true;
      manager_->setBusy( true );
      send_buffer.clear();
      send_buffer.push_back( uint8_t( 0xFF ) ); // Acknowledgment packet indicator
      send_buffer.push_back( uint8_t( index ) );
      esp_err_t result =
          esp_now_send( connection_->peer_info.peer_addr, send_buffer.data(), send_buffer.size() );
      if ( result != ESP_OK ) {
        manager_->setBusy( false );
        static elapsedMillis last_print = 5000;
        if ( last_print > 2000 ) {
          last_print = 0;
          Serial.printf( "ESP-NOW send to %02X:%02X:%02X:%02X:%02X:%02X failed with result %s\n",
                         connection_->peer_info.peer_addr[0], connection_->peer_info.peer_addr[1],
                         connection_->peer_info.peer_addr[2], connection_->peer_info.peer_addr[3],
                         connection_->peer_info.peer_addr[4], connection_->peer_info.peer_addr[5],
                         esp_err_to_name( result ) );
        }
        connection_->transmission_failure_count++;
      }
    }
  }
  static elapsedMillis last_print = 5000;
  if ( last_print > 2000 ) {
    last_print = 0;
    for ( int i = 0; i < manager_->connection_count; ++i ) {
      auto &conn = manager_->connections[i];
      Serial.printf( "Peer %02X:%02X:%02X:%02X:%02X:%02X - RSSI: %d dBm, Last Msg Age: %lu ms, "
                     "Tx Success: %lu, Tx Failures: %lu\n",
                     conn->peer_info.peer_addr[0], conn->peer_info.peer_addr[1],
                     conn->peer_info.peer_addr[2], conn->peer_info.peer_addr[3],
                     conn->peer_info.peer_addr[4], conn->peer_info.peer_addr[5], conn->rssi,
                     (unsigned long)conn->last_received_time, conn->transmission_success_count,
                     conn->transmission_failure_count );
    }
  }
}

CommState ESPNowInterface::getCommState() const
{
  if ( manager_->state != ESP_OK ) {
    return CommState::ERROR;
  }
  if ( connection_->last_received_time < 500 ) {
    return CommState::CONNECTED;
  }
  return CommState::DISCONNECTED;
}

int8_t ESPNowInterface::getRSSI() const { return connection_->rssi; }

unsigned long ESPNowInterface::getLastReceivedMessageAge() const
{
  return connection_->last_received_time;
}

unsigned long ESPNowInterface::getTransmissionSuccessCount() const
{
  return connection_->transmission_success_count;
}

unsigned long ESPNowInterface::getTransmissionFailureCount() const
{
  return connection_->transmission_failure_count;
}

void ESPNowInterface::resetTransmissionStats()
{
  connection_->transmission_success_count = 0;
  connection_->transmission_failure_count = 0;
}

int ESPNowInterface::getLatency() const
{
  int min_latency = INT32_MAX;
  for ( const auto &property : connection_->properties ) {
    if ( property.last_latency_ms >= 0 && property.last_latency_ms < min_latency ) {
      min_latency = property.last_latency_ms;
    }
  }
  return min_latency == INT32_MAX ? -1 : min_latency;
}

void ESPNowInterface::readProperty( uint8_t id, std::vector<uint8_t> &data, unsigned long &age_ms ) const
{
  if ( id >= connection_->properties.size() ) {
    data.clear();
    age_ms = ULONG_MAX; // Invalid property ID
    return;
  }
  const auto &property = connection_->properties[id];
  data = property.data;
  age_ms = property.age_ms;
}

void ESPNowInterface::setProperty( uint8_t id, const std::vector<uint8_t> &data )
{
  connection_->writeProperty( id, data );
}

IRAM_ATTR void onSentCallback( const wifi_tx_info_t *info, esp_now_send_status_t status )
{
  ESPNowInterface::manager_->setBusy( false );
  for ( int i = 0; i < ESPNowInterface::manager_->connection_count; ++i ) {
    auto &connection = ESPNowInterface::manager_->connections[i];
    if ( memcmp( connection->peer_info.peer_addr, info->des_addr, 6 ) == 0 ) {
      connection->onSent( info->des_addr, status );
      return;
    }
  }
}
IRAM_ATTR void onReceivedCallback( const esp_now_recv_info_t *info, const uint8_t *data, int len )
{
  for ( int i = 0; i < ESPNowInterface::manager_->connection_count; ++i ) {
    auto &connection = ESPNowInterface::manager_->connections[i];
    if ( memcmp( connection->peer_info.peer_addr, info->src_addr, 6 ) == 0 ) {
      connection->onReceived( info->src_addr, data, len );
      connection->rssi = info->rx_ctrl->rssi;
      return;
    }
  }
}

ESPNowInterface::ESPNowManager::ESPNowManager()
{
  WiFi.mode( WIFI_STA );
  WiFi.setTxPower( WIFI_POWER_21dBm );
  esp_wifi_set_ps( WIFI_PS_NONE );

  // Explicitly set WiFi channel so both peers use the same channel.
  // In STA mode without an AP connection, the channel may be undefined or
  // inconsistent between devices, causing asymmetric communication failures.
  esp_wifi_set_channel( ESTOP_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_ABOVE );

  state = esp_now_init();
  if ( state != ESP_OK ) {
    Serial.println( "Failed to initialize ESP-NOW" );
    return;
  }
  Serial.println( "ESP-NOW initialized successfully" );
  Serial.println( "ESP-NOW MAC Address: " + WiFi.macAddress() );

  // Register callback
  esp_now_register_send_cb( onSentCallback );
  esp_now_register_recv_cb( onReceivedCallback );
}

void ESPNowInterface::ESPNowConnection::onReceived( const uint8_t *mac_addr, const uint8_t *data,
                                                    int len )
{
  if ( len < 2 ) {
    return;
  }
  const uint8_t index = data[0];
  if ( index == 0xFF ) {
    last_received_time = 0;
    const uint8_t acked_id = data[1];
    if ( acked_id < properties.size() && properties[acked_id].awaiting_ack ) {
      properties[acked_id].last_latency_ms = properties[acked_id].send_time;
      properties[acked_id].awaiting_ack = false;
    }
    return;
  }
  if ( index >= properties.size() ) {
    return;
  }
  properties[index].data.assign( data + 1, data + len );
  properties[index].age_ms = 0; // Reset age on valid packet
  properties[index].acknowledged = false;
  last_received_time = 0;
}

void ESPNowInterface::ESPNowConnection::writeProperty( uint8_t id, const std::vector<uint8_t> &data )
{
  if ( ESPNowInterface::manager_->state != ESP_OK )
    return;
  properties[id].data = data;
  properties[id].updated = true;
  properties[id].send_time = 0;
  properties[id].awaiting_ack = true;
  if ( ESPNowInterface::manager_->isBusy() ) {
    return;
  }
  ESPNowInterface::manager_->setBusy( true );
  send_buffer.clear();
  send_buffer.push_back( id );
  send_buffer.insert( send_buffer.end(), data.begin(), data.end() );

  // Result is checked in onSent callback. Can be ignored here.
  esp_err_t result = esp_now_send( peer_info.peer_addr, send_buffer.data(), send_buffer.size() );
  if ( result == ESP_OK ) {
    properties[id].updated = false;
  }
}