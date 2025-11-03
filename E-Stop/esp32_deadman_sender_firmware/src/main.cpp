#include "deadman_comm_interface.h"
#include <Arduino.h>
#include <elapsedMillis.h>

#define ENABLED_PIN D10
#define DEADMAN_PIN D8
#define PANIC_TRIGGER_PIN D7
#define CONNECTED_LED_PIN D1
#define ACTIVE_LED_PIN D2
#define INACTIVE_LED_PIN D3

DeadmanCommInterface sender;

elapsedMillis last_print = 0;

void setup()
{
  Serial.begin( 115200 );
  Serial.println( "Starting HECTOR Deadman Switch..." );

  sender.initialize( RECEIVER_PEER_INFO );
  sender.setActive( false );
  sender.setTriggered( false );

  Serial.println( "CommInterface initialized" );

  // Turn on LED
  pinMode( LED_BUILTIN, OUTPUT );
  digitalWrite( LED_BUILTIN, LOW );

  pinMode( ENABLED_PIN, INPUT_PULLDOWN );
  pinMode( DEADMAN_PIN, INPUT_PULLDOWN );
  pinMode( PANIC_TRIGGER_PIN, INPUT_PULLDOWN );
  pinMode( INACTIVE_LED_PIN, OUTPUT );
  pinMode( ACTIVE_LED_PIN, OUTPUT );
  pinMode( CONNECTED_LED_PIN, OUTPUT );
  digitalWrite( INACTIVE_LED_PIN, LOW );
  digitalWrite( ACTIVE_LED_PIN, LOW );
  digitalWrite( CONNECTED_LED_PIN, LOW );
}

struct {
  bool enabled;
  bool triggered;
  bool panic_triggered;
} State;

elapsedMillis last_send = 0;

void loop()
{
  const bool enabled_state = digitalRead( ENABLED_PIN ) == HIGH;
  const bool deadman_state = digitalRead( DEADMAN_PIN ) == HIGH;
  const bool panic_trigger_state = digitalRead( PANIC_TRIGGER_PIN ) == HIGH;

  CommStatus status = sender.update();

  if ( status.esp_now_state == CommState::CONNECTED || status.ble_state == CommState::CONNECTED ) {
    digitalWrite( CONNECTED_LED_PIN, HIGH );
  } else {
    digitalWrite( CONNECTED_LED_PIN, LOW );
  }

  if ( enabled_state != State.enabled ) {
    State.enabled = enabled_state;
    sender.setActive( State.enabled );
    if ( State.enabled ) {
      Serial.println( "Deadman Switch Enabled" );
    } else {
      Serial.println( "Deadman Switch Disabled" );
      digitalWrite( INACTIVE_LED_PIN, LOW );
    }
  }
  if ( !State.enabled ) {
    digitalWrite( ACTIVE_LED_PIN, LOW );
    digitalWrite( INACTIVE_LED_PIN, LOW );
    // Send periodic update even if state hasn't changed
    if ( last_send > 100 ) {
      last_send = 0;
      sender.setActive( State.enabled );
    }
  } else {
    const bool triggered = !deadman_state || panic_trigger_state || State.panic_triggered;
    if ( last_send > 100 || triggered != State.triggered ) {
      last_send = 0;
      State.triggered = triggered;
      sender.setTriggered( State.triggered );
      if ( State.triggered ) {
        digitalWrite( ACTIVE_LED_PIN, HIGH );
        digitalWrite( INACTIVE_LED_PIN, LOW );
      } else {
        digitalWrite( ACTIVE_LED_PIN, LOW );
        digitalWrite( INACTIVE_LED_PIN, HIGH );
      }
    }

    if ( panic_trigger_state != State.panic_triggered && State.panic_triggered != deadman_state ) {
      // Can only reset panic trigger if deadman button is also released
      State.panic_triggered = panic_trigger_state;
      if ( State.panic_triggered ) {
        Serial.println( "Panic Triggered" );
      } else {
        Serial.println( "Panic Released" );
      }
    }
  }

  if ( last_print > 2000 ) {
    last_print = 0;
    Serial.printf(
        "Status: ESP-NOW: %s, BLE: %s | Active: %s | Triggered: %s | RSSI: "
        "ESP-NOW: %d, BLE: %d\n",
        status.esp_now_state == CommState::CONNECTED
            ? "Connected"
            : ( status.esp_now_state == CommState::DISCONNECTED ? "Disconnected" : "Error" ),
        status.ble_state == CommState::CONNECTED
            ? "Connected"
            : ( status.ble_state == CommState::DISCONNECTED ? "Disconnected" : "Error" ),
        sender.isActive() ? "true" : "false", sender.isTriggered() ? "true" : "false",
        status.esp_now_rssi, status.ble_rssi );
  }

  // Run this loop every 2ms
  delay( 2 );
}
