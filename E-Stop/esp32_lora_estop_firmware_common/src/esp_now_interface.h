#pragma once

#include "comm_interface.h"

#include <memory>
#include <vector>

class ESPNowInterface
{
public:
  ESPNowInterface( const uint8_t peer_mac[6] );

  ~ESPNowInterface();

  void update();

  CommState getCommState() const;

  int8_t getRSSI() const;

  unsigned long getLastReceivedMessageAge() const;

  unsigned long getTransmissionSuccessCount() const;

  unsigned long getTransmissionFailureCount() const;

  void resetTransmissionStats();

  bool hasProperty( uint8_t id ) const
  {
    return id == COMM_PROPERTY_ID_ESTOP || id == COMM_PROPERTY_ID_SOFT_ESTOP ||
           id == COMM_PROPERTY_ID_BATTERY;
  }

  void readProperty( uint8_t id, std::vector<uint8_t> &data, unsigned long &age_ms ) const;
  void setProperty( uint8_t id, const std::vector<uint8_t> &data );

  class ESPNowManager;
  class ESPNowConnection;
  static ESPNowManager *manager_;

private:
  std::shared_ptr<ESPNowConnection> connection_;
};