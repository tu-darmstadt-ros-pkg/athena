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

  void onSent( const uint8_t *mac_addr, esp_now_send_status_t status )
  {
    for ( auto &connection : connections ) {
      if ( memcmp( connection->peer_info.peer_addr, mac_addr, 6 ) == 0 ) {
        connection->onSent( mac_addr, status );
        return;
      }
    }
  }

  void onReceived( const uint8_t *mac_addr, const uint8_t *data, int len )
  {
    for ( auto &connection : connections ) {
      if ( memcmp( connection->peer_info.peer_addr, mac_addr, 6 ) == 0 ) {
        connection->onReceived( mac_addr, data, len );
        return;
      }
    }
  }

  void updateRSSI( const uint8_t sender_mac[6], int rssi )
  {
    for ( auto &connection : connections ) {
      if ( memcmp( connection->peer_info.peer_addr, sender_mac, 6 ) == 0 ) {
        connection->rssi = rssi;
        return;
      }
    }
  }

  std::shared_ptr<ESPNowInterface::ESPNowConnection> addConnection( const uint8_t peer_mac[6] )
  {
    esp_now_peer_info_t peer_info = {};
    std::copy( peer_mac, peer_mac + 6, peer_info.peer_addr );
    peer_info.channel = 0;
    peer_info.encrypt = false;

    // Add peer
    if ( esp_now_add_peer( &peer_info ) != ESP_OK ) {
      Serial.println( "Failed to add ESP-NOW peer" );
    } else {
      Serial.println( "ESP-NOW peer added successfully" );
    }
    auto connection = std::make_shared<ESPNowInterface::ESPNowConnection>( peer_info );
    connections.push_back( connection );
    return connection;
  }

  void removeConnection( std::shared_ptr<ESPNowInterface::ESPNowConnection> connection )
  {
    esp_now_del_peer( connection->peer_info.peer_addr );
    connections.erase( std::remove( connections.begin(), connections.end(), connection ),
                       connections.end() );
  }

  esp_err_t state = ESP_ERR_ESPNOW_NOT_INIT;
  std::vector<std::shared_ptr<ESPNowInterface::ESPNowConnection>> connections;
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

void ESPNowInterface::update() { /* Nothing to do */ }

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

IRAM_ATTR void onSentCallback( const uint8_t *mac_addr, esp_now_send_status_t status )
{
  ESPNowInterface::manager_->onSent( mac_addr, status );
}
IRAM_ATTR void onReceivedCallback( const uint8_t *mac_addr, const uint8_t *data, int len )
{
  ESPNowInterface::manager_->onReceived( mac_addr, data, len );
}

// Structures for retrieving packet data: RSSI, etc
typedef struct {
  unsigned frame_ctrl : 16;
  unsigned duration_id : 16;
  uint8_t addr1[6]; /* receiver address */
  uint8_t addr2[6]; /* sender address */
  uint8_t addr3[6]; /* filtering address */
  unsigned sequence_ctrl : 16;
  uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

IRAM_ATTR void promiscuous_rx_cb( void *buf, wifi_promiscuous_pkt_type_t type )
{
  // All espnow traffic uses action frames which are a subtype of the mgmnt frames so filter out everything else.
  if ( type != WIFI_PKT_MGMT )
    return;

  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buf;
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;

  ESPNowInterface::manager_->updateRSSI( hdr->addr2, ppkt->rx_ctrl.rssi );
}

ESPNowInterface::ESPNowManager::ESPNowManager()
{

  WiFi.mode( WIFI_STA );
  WiFi.setTxPower( WIFI_POWER_11dBm );
  WiFi.enableLongRange( true );
  state = esp_now_init();
  if ( state != ESP_OK )
    return;

  // Register callback
  esp_now_register_send_cb( onSentCallback );
  esp_now_register_recv_cb( onReceivedCallback );

  // Enable promiscuous mode to capture all ESP-NOW traffic and get RSSI
  esp_wifi_set_promiscuous( true );
  esp_wifi_set_promiscuous_rx_cb( promiscuous_rx_cb );
}

void ESPNowInterface::ESPNowConnection::onReceived( const uint8_t *mac_addr, const uint8_t *data,
                                                    int len )
{
  if ( len < 2 ) {
    return;
  }
  const uint8_t index = data[0];
  if ( index == 0xFF ) {
    // Acknowledgment packet, ignore
    last_received_time = 0;
    return;
  }
  if ( index < 0 || index >= properties.size() ) {
    Serial.println( "Received packet with invalid property index" );
    return;
  }
  properties[index].data.assign( data + 1, data + len );
  properties[index].age_ms = 0; // Reset age on valid packet
  last_received_time = 0;
  // Acknowledge the received packet
  uint8_t ack_package[2] = { uint8_t( 0xFF ), index };
  esp_err_t result = esp_now_send( mac_addr, ack_package, 2 );
  if ( result != ESP_OK ) {
    transmission_failure_count++;
  }
}

void ESPNowInterface::ESPNowConnection::writeProperty( uint8_t id, const std::vector<uint8_t> &data )
{
  if ( ESPNowInterface::manager_->state != ESP_OK )
    return;
  send_buffer.clear();
  send_buffer.push_back( id );
  send_buffer.insert( send_buffer.end(), data.begin(), data.end() );

  esp_err_t result = esp_now_send( peer_info.peer_addr, send_buffer.data(), send_buffer.size() );
  if ( result != ESP_OK ) {
    transmission_failure_count++;
  }
}