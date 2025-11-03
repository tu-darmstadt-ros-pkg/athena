# RSSI in dBm
float32 ble_rssi
float32 esp_now_rssi
float32 radio_rssi

uint8 DISCONNECTED = 0
uint8 CONNECTED = 1
uint8 CONNECTION_ERROR = 10

uint8 ble_connection_status
uint8 esp_now_connection_status
uint8 radio_connection_status

uint16 last_message_age_ms
