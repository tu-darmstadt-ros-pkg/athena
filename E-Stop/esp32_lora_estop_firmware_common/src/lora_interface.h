#pragma once

#include "comm_interface.h"
#include <memory>
#include <vector>

class LoraInterface
{
public:
  LoraInterface( bool is_server );

  void update();

  CommState getCommState() const;

  float getRSSI() const;

  unsigned long getLastReceivedMessageAge() const;

  bool hasProperty( uint8_t id ) const
  {
    return id == COMM_PROPERTY_ID_ESTOP || id == COMM_PROPERTY_ID_SOFT_ESTOP;
  }

  void setProperty( uint8_t id, const std::vector<uint8_t> &data );
  void readProperty( uint8_t id, std::vector<uint8_t> &data, unsigned long &age_ms ) const;

  class Impl;
  static Impl *impl_;

private:
};