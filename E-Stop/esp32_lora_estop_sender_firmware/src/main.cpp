#include "comm_interface.h"
#include "mean_filter.h"
#include <Arduino.h>
#include <elapsedMillis.h>

#include <U8g2lib.h>

constexpr uint8_t ESTOP_PIN = D1;
constexpr uint8_t SOFT_ESTOP_PIN = D2;
constexpr uint8_t RELEASE_PIN = D7;
constexpr uint8_t BATTERY_PIN = A3;
constexpr uint8_t EXPERIMENT_VOLTAGE_INPUT_PIN = A0;
constexpr const char *TITLE = "Athena Remote E-Stop";

TaskHandle_t display_task_handle = nullptr;
SemaphoreHandle_t display_semaphore = nullptr;
U8G2_SH1106_128X64_NONAME_F_HW_I2C display( U8G2_R0, D6, D5, D4 );

CommInterface sender;

elapsedMillis last_print = 0;
elapsedMillis last_status_update_time = 0;
constexpr int STATUS_UPDATE_INTERVAL_MS = 50; // Resend status every 50ms even if not changed

bool soft_estop_button_pressed = false;
bool release_button_state = false;
bool display_initialized = false;
MeanFilter<uint32_t, 10> battery_voltage_filter;
bool experiment_running = false;

float readBatteryVoltage()
{
  battery_voltage_filter.addValue( analogRead( BATTERY_PIN ) );
  // Battery voltage is read using a voltage divider. Ratio = 4.127V / 2370
  return battery_voltage_filter.getMean() * ( 4.127f / 2370 );
}

int toBatteryPercentage( float voltage )
{
  // The percentage is calculated based on the voltage range of 3.0V (empty) to 4.2V (full).
  // The formula is: percentage = (voltage - 3.0) / (4.2 - 3.0) * 100
  if ( voltage < 3.0f ) {
    return 0; // Below minimum voltage, return 0%
  }
  if ( voltage > 4.2f ) {
    return 100; // Above maximum voltage, return 100%
  }
  return ( voltage - 3.0f ) / ( 4.2f - 3.0f ) * 100;
}

void drawBootScreen();

void updateDisplayTask( void * );

void runExperiment();

void setup()
{
  Serial.begin( 115200 );
  Serial.println( "Starting HECTOR E-Stop Remote..." );
  pinMode( BATTERY_PIN, INPUT );

  if ( display.begin() ) {
    display_initialized = true;
    drawBootScreen();
  } else {
    Serial.println( "Failed to initialize display!" );
  }

  sender.initialize( CommMode::CLIENT, RECEIVER_PEER_INFO );
  sender.setEStopState( true );      // Set E-Stop state to active initially
  sender.setSoftEStopState( false ); // Soft E-Stop is inactive initially

  Serial.println( "CommInterface initialized" );

  // Turn on LED
  pinMode( LED_BUILTIN, OUTPUT );
  digitalWrite( LED_BUILTIN, LOW );

  pinMode( ESTOP_PIN, INPUT_PULLDOWN );
  pinMode( SOFT_ESTOP_PIN, INPUT_PULLDOWN );
  pinMode( RELEASE_PIN, INPUT_PULLDOWN );
  pinMode( EXPERIMENT_VOLTAGE_INPUT_PIN, INPUT );

  soft_estop_button_pressed = digitalRead( SOFT_ESTOP_PIN ) == HIGH;
  release_button_state = digitalRead( RELEASE_PIN ) == HIGH;
  display_semaphore = xSemaphoreCreateMutex();
  xTaskCreatePinnedToCore( updateDisplayTask, "UpdateDisplay", 4096, nullptr, 0,
                           &display_task_handle, 0 );
}

struct DisplayStatus {
  CommStatus comm_status;
  bool estop_active;
  bool soft_estop_active;
} display_status;

constexpr int EXPERIMENT_STEPS = 1000;
struct ExperimentStatus {
  bool held_down = false;
  elapsedMillis held_time;
  int step = 0;
  bool estop_active = true;
  elapsedMillis step_time;
  int durations[EXPERIMENT_STEPS] = {};
  bool printed_results = false;
};

ExperimentStatus experiment_status;
elapsedMillis last_update_display_status;

void updateDisplay();

void loop()
{
  // Read E-Stop button state. Here LOW means pressed, HIGH means not pressed
  const bool estop_state = digitalRead( ESTOP_PIN ) == LOW;
  // Read Soft E-Stop button state. Since these close the circuit when pressed, we use HIGH for pressed
  const bool soft_estop_state = digitalRead( SOFT_ESTOP_PIN ) == HIGH;
  // Read Release button state
  const bool last_release_state = release_button_state;
  release_button_state = digitalRead( RELEASE_PIN ) == HIGH;

  if ( !experiment_running ) {
    const bool release_button_pressed = release_button_state && !last_release_state;
    if ( release_button_pressed && !estop_state && !soft_estop_state ) {
      // If the Release button is pressed (and none of the estops are active), set E-Stop state to inactive
      sender.setEStopState( false );
      sender.setSoftEStopState( false );
      Serial.println( "Release button pressed, E-Stop and Soft E-Stop deactivated." );
    } else {
      // Only allow change to true. For release, we use the Release button
      const bool current_estop_state = sender.getEStopState();
      const bool current_soft_estop_state = sender.getSoftEStopState();
      if ( estop_state != current_estop_state || soft_estop_state != current_soft_estop_state ||
           last_status_update_time > STATUS_UPDATE_INTERVAL_MS ) {
        last_status_update_time = 0;
        sender.setEStopState( sender.getEStopState() || estop_state );
        sender.setSoftEStopState( sender.getSoftEStopState() || soft_estop_state );
      }
      if ( !estop_state && release_button_state && soft_estop_state ) {
        // If E-Stop is inactive and both Release and Soft E-Stop are pressed and held for 3s, start experiment
        if ( !experiment_status.held_down ) {
          experiment_status.held_down = true;
          experiment_status.held_time = 0;
        } else if ( experiment_status.held_time > 3000 && !experiment_running ) {
          uint32_t mv = analogReadMilliVolts( EXPERIMENT_VOLTAGE_INPUT_PIN );
          Serial.println( mv );
          if ( mv > 2000 ) {
            experiment_running = true;
            experiment_status.printed_results = false;
            sender.setEStopState( true );
            experiment_status.estop_active = true;
            experiment_status.step = 0;
            Serial.println( "Experiment started." );
          }
        }
      } else {
        experiment_status.held_down = false;
      }
    }
  } else {
    runExperiment();
  }
  if ( last_update_display_status > 50 && xSemaphoreTake( display_semaphore, 10 ) ) {
    last_update_display_status = 0;
    CommStatus status = sender.update();
    display_status = { status, sender.getEStopState(), sender.getSoftEStopState() };
    xSemaphoreGive( display_semaphore );
  }
  delay( 1 );
}

void updateDisplayTask( void * )
{
  while ( true ) {
    if ( !display_initialized ) {
      vTaskDelay( 500 / portTICK_PERIOD_MS );
      continue;
    }
    updateDisplay();
    vTaskDelay( 200 / portTICK_PERIOD_MS );
  }
}

bool first_connect = true;
elapsedMillis last_battery_draw_time = 0;

void drawBatteryLevel()
{
  float voltage = readBatteryVoltage();
  int percentage = toBatteryPercentage( voltage );
  const char *battery_icon = "0";
  if ( percentage >= 90 ) {
    battery_icon = "5"; // Full
  } else if ( percentage >= 70 ) {
    battery_icon = "4"; // High
  } else if ( percentage >= 50 ) {
    battery_icon = "3"; // Medium
  } else if ( percentage >= 30 ) {
    battery_icon = "2"; // Low
  } else if ( percentage >= 10 ) {
    battery_icon = "1"; // Critical
  }
  if ( percentage > 10 || last_battery_draw_time > 1000 ) {
    last_battery_draw_time = 0;
    display.setFont( u8g2_font_battery19_tn );
    display.drawStr( 119, 20, battery_icon );
  }
}

void drawBootScreen()
{
  display.clearBuffer();
  display.setFont( u8g2_font_squeezed_b7_tr );
  display.drawStr( 0, 12, "Athena Remote E-Stop" );
  drawBatteryLevel();
  display.setFont( u8g2_font_logisoso16_tf );
  display.drawStr( 0, 48, "Booting..." );
  display.sendBuffer();
}

void drawCommState( int x, int y, CommState state, float rssi )
{
  if ( state == CommState::ERROR ) {
    display.drawStr( x, y, "ERROR" );
    return;
  }
  if ( state == CommState::DISCONNECTED ) {
    display.drawStr( x, y, "-" );
    return;
  }
  std::string text = std::to_string( int( rssi ) ) + " dBm";
  display.drawStr( x, y, text.c_str() );
}

void updateDisplay()
{
  xSemaphoreTake( display_semaphore, portMAX_DELAY );
  DisplayStatus status = display_status;
  xSemaphoreGive( display_semaphore );
  display.clearBuffer();

  if ( experiment_running ) {
    display.setFont( u8g2_font_squeezed_b7_tr );
    display.drawStr( 0, 12, "Running Experiment" );
    display.setFont( u8g2_font_prospero_bold_nbp_tf );
    display.drawStr( 24, 32,
                     ( "Step " + std::to_string( experiment_status.step ) + " / " +
                       std::to_string( EXPERIMENT_STEPS ) )
                         .c_str() );
  } else {
    display.setFont( u8g2_font_squeezed_b7_tr );
    display.drawStr( 0, 12, TITLE );
    drawBatteryLevel();
    if ( first_connect ) {
      if ( status.comm_status.ble_state == CommState::CONNECTED ||
           status.comm_status.esp_now_state == CommState::CONNECTED ) {
        first_connect = false;
      }
      display.setFont( u8g2_font_logisoso16_tf );
      display.drawStr( 0, 48, "Connecting..." );
    } else if ( status.comm_status.ble_state == CommState::DISCONNECTED &&
                status.comm_status.esp_now_state == CommState::DISCONNECTED ) {
      display.setFont( u8g2_font_logisoso16_tf );
      display.drawStr( 0, 48, "Reconnecting..." );
    } else {
      if ( status.estop_active ) {
        display.setFont( u8g2_font_open_iconic_check_2x_t );
        display.drawStr( 0, 35, "B" );
        display.setFont( u8g2_font_prospero_bold_nbp_tf );
        display.drawStr( 24, 32, "E-Stop Active!" );
      } else if ( status.soft_estop_active ) {
        display.setFont( u8g2_font_open_iconic_check_1x_t );
        display.drawStr( 0, 32, "D" );
        display.setFont( u8g2_font_prospero_bold_nbp_tf );
        display.drawStr( 14, 32, "Soft E-Stop Active!" );
      } else {
        display.setFont( u8g2_font_open_iconic_check_2x_t );
        display.drawStr( 0, 35, "@" );
        display.setFont( u8g2_font_prospero_bold_nbp_tf );
        display.drawStr( 24, 32, "E-Stop Inactive" );
      }
    }
  }
  if ( status.comm_status.ble_state != CommState::DISCONNECTED ||
       status.comm_status.esp_now_state != CommState::DISCONNECTED ) {
    display.setFont( u8g2_font_minuteconsole_tr );
    display.drawStr( 9, 48, "BLE" );
    drawCommState( 0, 60, status.comm_status.ble_state, status.comm_status.ble_rssi );
    display.drawStr( 54, 48, "ESP-NOW" );
    drawCommState( 52, 60, status.comm_status.esp_now_state, status.comm_status.esp_now_rssi );
    display.drawStr( 104, 48, "Radio" );
    if ( status.comm_status.radio_state == CommState::DISCONNECTED ) {
      display.drawStr( 108, 60, "N/A" );
    } else if ( status.comm_status.radio_state == CommState::ERROR ) {
      display.drawStr( 104, 60, "ERROR" );
    } else {
      display.drawStr( 109, 60, "OK" );
    }
  }
  display.sendBuffer();
}

void runExperiment()
{
  if ( experiment_status.step >= EXPERIMENT_STEPS ) {
    if ( !experiment_status.printed_results ) {
      experiment_status.printed_results = true;
      experiment_status.step_time = 0;
      xSemaphoreTake( display_semaphore, portMAX_DELAY );
      display.clearBuffer();
      display.setFont( u8g2_font_squeezed_b7_tr );
      display.drawStr( 0, 12, "Results" );
      display.setFont( u8g2_font_prospero_bold_nbp_tf );
      int sum = 0;
      Serial.println( "Experiment durations (s):" );
      for ( int i = 0; i < EXPERIMENT_STEPS; i++ ) {
        Serial.print( experiment_status.durations[i] );
        Serial.print( ", " );
        sum += experiment_status.durations[i] * 10;
      }
      Serial.println();
      int mean = sum / EXPERIMENT_STEPS;
      int var_sum = 0;
      for ( int i = 0; i < EXPERIMENT_STEPS; i++ ) {
        int diff = experiment_status.durations[i] * 10 - mean;
        var_sum += diff * diff;
      }
      float stddev = sqrt( static_cast<float>( var_sum ) / EXPERIMENT_STEPS );
      char buffer[64];
      sprintf( buffer, "Avg: %.1f +- %.1f", mean / 10.0f, stddev / 10.0f );
      display.drawStr( 24, 32, buffer );
      display.sendBuffer();
    }
    if ( experiment_status.step_time > 20000 ) {
      xSemaphoreGive( display_semaphore );
      experiment_running = false;
    }
    return;
  }
  uint32_t val = analogReadMilliVolts( EXPERIMENT_VOLTAGE_INPUT_PIN );
  // Serial.println(val);
  if ( experiment_status.estop_active ) {
    if ( val < 500 ) {
      experiment_status.step_time = 0;
      sender.setEStopState( false );
      last_status_update_time = 0;
      experiment_status.estop_active = false;
    }
  } else if ( val > 1500 ) {
    sender.setEStopState( true );
    last_status_update_time = 0;
    experiment_status.estop_active = true;
    experiment_status.durations[experiment_status.step] = experiment_status.step_time;
    experiment_status.step++;
  }
  if ( last_status_update_time > 50 ) {
    sender.setEStopState( experiment_status.estop_active );
    last_status_update_time = 0;
  }
}
