#pragma once

#include "comm_interface.h"
#include "crosstalk.hpp"

REFL_AUTO( type( CommStatus, crosstalk::id( 0x01 ) ), field( last_received_message_age_ms ),
           field( ble_rssi ), field( radio_rssi ), field( esp_now_rssi ), field( ble_state ),
           field( esp_now_state ), field( radio_state ) )

struct EStopReceiverStatus {
  CommStatus remote_status;
  CommStatus deadman_status;
};

REFL_AUTO( type( EStopReceiverStatus, crosstalk::id( 0x03 ) ), field( remote_status ),
           field( deadman_status ) )

struct EStopState {
  bool enabled = false;
  bool hard_estop_active = false;
  bool soft_estop_active = false;
  bool deadman_active = false;
  bool deadman_triggered = false;
};

REFL_AUTO( type( EStopState, crosstalk::id( 0x02 ) ), field( enabled ), field( hard_estop_active ),
           field( soft_estop_active ), field( deadman_active ), field( deadman_triggered ) )

struct SetEnabledCommand {
  bool enabled = true;
};

REFL_AUTO( type( SetEnabledCommand, crosstalk::id( 0x03 ) ), field( enabled ) )
