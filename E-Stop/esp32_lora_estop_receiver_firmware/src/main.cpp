#include <Arduino.h>
#include <comm_interface.h>
#include <deadman_comm_interface.h>
#include <elapsedMillis.h>

#include "host_comm.h"
#include <HardwareSerial.h>

#include "crosstalk.hpp"
#include "crosstalk_hardware_serial_wrapper.hpp"

#define ESTOP_OUT_PIN D3

CommInterface remote_comm;
DeadmanCommInterface deadman_comm;

elapsedMillis last_estop_send = 0;
elapsedMillis last_comm_status_send = 0;
elapsedMillis last_print = 0;
crosstalk::CrossTalker<512, 64>
    host_comm( std::make_unique<crosstalk::HardwareSerialWrapper<HWCDC>>( Serial ) );

void setup()
{
  Serial.begin( 115200 );
  Serial.println( "Starting HECTOR E-Stop Remote..." );
  remote_comm.initialize( CommMode::SERVER, SENDER_PEER_INFO );
  Serial.println( "CommInterface initialized" );
  deadman_comm.initialize( remote_comm.getBLEInterface(), DEADMAN_PEER_INFO );
  Serial.println( "DeadmanCommInterface initialized" );
  pinMode( LED_BUILTIN, OUTPUT );
  digitalWrite( LED_BUILTIN, LOW );

  pinMode( ESTOP_OUT_PIN, OUTPUT );
  digitalWrite( ESTOP_OUT_PIN, LOW );
}

// If not enabled, the E-Stop output is always inactive (HIGH) and the status of the E-Stop is ignored.
bool enabled = true;
bool last_estop_active = true;
bool last_soft_estop_active = true;

void loop()
{
  const EStopReceiverStatus status = {
    remote_comm.update(),
    deadman_comm.update()
  };
  const bool deadman_active = deadman_comm.isActive();
  const bool deadman_triggered = deadman_comm.isTriggered();
  const bool current_estop_active = remote_comm.getEStopState() || (deadman_active && deadman_triggered);
  const bool current_soft_estop_active = remote_comm.getSoftEStopState();
  digitalWrite( ESTOP_OUT_PIN, enabled && current_estop_active ? LOW : HIGH );

  if ( last_estop_send > 100 || current_estop_active != last_estop_active ||
       current_soft_estop_active != last_soft_estop_active ) {
    last_estop_send = 0;
    last_estop_active = current_estop_active;
    last_soft_estop_active = current_soft_estop_active;
    host_comm.sendObject( EStopState{
        .enabled = enabled,
        .hard_estop_active = remote_comm.getEStopState(),
        .soft_estop_active = remote_comm.getSoftEStopState(),
        .deadman_active  = deadman_active,
        .deadman_triggered = deadman_triggered
    } );
    digitalWrite( LED_BUILTIN, last_estop_active ? LOW : HIGH );
  }

  if ( last_comm_status_send > 500 ) {
    last_comm_status_send = 0;
    host_comm.sendObject( status );
  }

  host_comm.processSerialData();
  if ( host_comm.available() )
    host_comm.skip();
  if ( host_comm.hasObject() ) {
    int16_t object_id = host_comm.getObjectId();
    switch ( object_id ) {
    case crosstalk::object_id<SetEnabledCommand>():
      SetEnabledCommand cmd;
      if ( host_comm.readObject( cmd ) == crosstalk::ReadResult::Success ) {
        enabled = cmd.enabled;
        Serial.printf( "Set E-Stop enabled to: %s\n", cmd.enabled ? "true" : "false" );
      } else {
        Serial.println( "Failed to read SetEnabledCommand object" );
      }
      break;
    }
  }

  // if ( last_print > 1000 ) {
  //   last_print = 0;
  //   Serial.printf( "BLE connected: %d (%.2f dBm), ESP Now Connected: %d (%.2f dBm), Radio "
  //                  "connected: %d (%.2f dBm). Age last message: %lu\n",
  //                  status.ble_state, status.ble_rssi, status.esp_now_state, status.esp_now_rssi,
  //                  status.radio_state, status.radio_rssi, status.last_received_message_age_ms );
  //   Serial.print( "Estop state: " + String( remote_comm.getEStopState() ? "active" : "inactive" ) );
  //   Serial.println( ". Soft EStop state: " +
  //                   String( remote_comm.getSoftEStopState() ? "active" : "inactive" ) );
  // }
  delay( 1 );
}
