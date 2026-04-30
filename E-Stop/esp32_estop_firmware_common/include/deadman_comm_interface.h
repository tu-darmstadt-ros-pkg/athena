#pragma once

#include "comm_interface.h"
#include "ble_interface.h"

class DeadmanCommInterface
{
public:
  DeadmanCommInterface();
  ~DeadmanCommInterface();

  void initialize( BLEInterface *ble_interface, const CommPeerInfo &peer_info );

  void initialize( const CommPeerInfo &peer_info );

  CommStatus update();

  //! Gets if the deadman is connected and active to use. If active and triggered, the E-Stop will be engaged.
  bool isActive() const;
  void setActive( bool active );
  //! Gets if the deadman condition is currently triggered.
  //! If triggered while active, the E-Stop will be engaged.
  //! The deadman is triggered if the button is not pressed or pressed too hard triggering the panic button.
  bool isTriggered() const;
  void setTriggered( bool active );

  class Impl;
  static Impl *impl_;

private:
  uint8_t counter_ = 0;
};
