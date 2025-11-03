# ESP32 LoRa Remote E-Stop

## Overview

This repository contains the firmware, shared libraries, and ROS 2 interface for a wireless emergency-stop built around the Seeed Studio XIAO ESP32S3 with the LoRa/Long Range expansion board.
It delivers a handheld remote that broadcasts hard and soft E-Stop signals, a receiver that drives the physical safety relay, and an optional deadman switch that can be used for dangerous hardware experiments.
All nodes share a common communication stack that uses LoRa, ESP-NOW, and BLE for redundant, low-latency signaling.

>[!NOTE]
> This library uses [CrossTalk](https://github.com/StefanFabian/crosstalk) for host to serial communication.

## Highlights

- Triple-channel wireless transport (LoRa, ESP-NOW, BLE)
- Independently latched hard E-Stop and soft E-Stop states.
- Optional deadman transmitter that triggers the hard E-Stop while enabled and not pressed and will only release while held down.
- SH1106 OLED UI on the remote with connectivity status, E-Stop indicators, and battery level.
- ROS 2 interface package for logging, visualization, and software integration with robots or tooling.

## How it works

### Components

- **Remote handset (`esp32_lora_estop_sender_firmware`)** – Hosts the physical E-Stop, soft E-Stop, and release buttons, reads the battery voltage, and pushes state updates through `CommInterface`.
- **Receiver (`esp32_lora_estop_receiver_firmware`)** – Listens as the LoRa server, merges wireless channels, asserts the relay output on `D3`, and publishes telemetry over USB using the lightweight CrossTalk protocol.
- **Deadman transmitter (`esp32_deadman_sender_firmware`)** _(optional)_ – Sends "active" and "triggered" states via `DeadmanCommInterface`, allowing a foot pedal or lanyard to feed into the receiver.
- **ROS 2 interface (`esp32_lora_estop_ros` + `esp32_lora_estop_interface`)** – Wraps the CrossTalk datastream in ROS 2 messages (`CommStatus.msg`) and exposes a `SetEnabled` service to bypass the receiver output during testing.
- **Shared firmware library (`esp32_lora_estop_firmware_common`)** – Abstracts the radios, handles property replication, and centralizes MAC configuration inside `include/comm_interface.h`.

### Communication pipeline

1. The remote handset runs `CommInterface` in _client_ mode. Each button state is latched locally and shipped via LoRa packets (SX1262), ESP-NOW datagrams, and BLE characteristics.
2. The receiver instances `CommInterface` in _server_ mode. On every update it reads the newest packet from each transport, chooses the freshest data, and drives the relay output low (active) or high (released).
3. The optional deadman transmitter mirrors the pattern with `DeadmanCommInterface` and is OR'd into the receiver logic, so either the handheld or the deadman can trip the system.
4. Telemetry such as RSSI, link state, battery percentage, and message age are streamed to ROS 2 or any host that speaks CrossTalk over USB.

### Safety logic

- Hard E-Stop is asserted if **any** transport reports an active state or if an active deadman transmitter sees a trigger event.
- Soft E-Stop is tracked separately and can be leveraged by higher-level controllers for graceful deceleration.
- The receiver defaults to the safe state (output asserted) if no valid packets arrive within ~300 ms.

## Hardware

### Bill of materials

| Name                                | Count | Url                                                                                                                                                                        | Price       | Description                                                                                                        |
| ----------------------------------- | ----- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------- | ------------------------------------------------------------------------------------------------------------------ |
| Seeed XIAO ESP32S3                  | 2     | <https://www.seeedstudio.com/Wio-SX1262-with-XIAO-ESP32S3-p-5982.html>                                                                                                     | 10€         | Sender and Receiver ESP with BLE, ESP NOW and LoRa.                                                                |
| Joy-it SBC-OLED01.3                 | 1     | <https://www.conrad.de/de/p/joy-it-sbc-oled01-3-display-modul-3-3-cm-1-3-zoll-128-x-64-pixel-passend-fuer-entwicklungskits-arduino-raspberry-pi-2619578.html?refresh=true> | 9€          | Any 128x64 display supported by U8g2lib can be used without code modification.                                     |
| TRU COMPONENTS Not-Aus-Schalter 3 A | 1     | <https://www.conrad.de/de/p/tru-components-not-aus-schalter-3-a-1-oeffner-ip65-1-st-2304759.html>                                                                          | 14€         |                                                                                                                    |
| Samsung ICR18650                    | 1     | <https://www.conrad.de/de/p/samsung-icr18650-spezial-akku-18650-kabel-li-ion-3-7-v-2600-mah-251024.html>                                                                   | 9€          | Any Li-Ion should do. Other types have to be compatible with the ESP and might require a different charging board. |
| USB-C Charging Board                | 1     | <https://www.amazon.de/dp/B07XG5F9T3?ref=ppx_yo2ov_dt_b_fed_asin_title>                                                                                                    | 8€ (for 10) | Has to be compatible with battery. Note that they do not work with some smart USB-C chargers.                      |
| Button                              | 2     | <https://www.roboter-bausatz.de/p/drucktaster-button-schalter-rot-12mm-250v-1a>                                                                                            | 1€          | Any button should do. One for Soft E-Stop and one to release the E-Stops.                                          |
| On-Off Switch                       | 1     | <https://www.conrad.com/en/p/tru-components-1587504-toggle-switch-tc-r13-66a3-02-250-v-ac-6-a-1-x-off-on-latch-1-pc-s-1587504.html>                                        | 2€          | To toggle power from battery.                                                                                      |
| JST XT Connectors                   | Kit   | <https://www.amazon.de/YIXISI-Stecker-Weiblich-Adapter-M%C3%A4nnlich/dp/B082ZLYRRN?source=ps-sl-shoppingads-lpcontext&ref_=fplfs&smid=A6CSVRKTUBQZZ&th=1>                  | 8€          | Suggested connectors to connect ESP to buttons, battery and display.                                               |
| Breadboard                          | 1     | <https://www.reichelt.com/de/en/shop/product/breadboard_laminated_paper_50x100_mm-8268>                                                                                    | 1€          | To mount ESP32 and connectors, as well as display.                                                                 |
| PETG Filament                       | 1     | -                                                                                                                                                                          | 10€         |                                                                                                                    |
| **Total Cost**                      |       |                                                                                                                                                                            | 83€         |                                                                                                                    |

### ESP 32

![img](esp32-pinout.png)

#### Sender

| Pin         | Function                                    |
| ----------- | ------------------------------------------- |
| D0          | _Unused_                                    |
| D1          | E-Stop button                               |
| D2          | Soft E-Stop button                          |
| A3          | Battery with voltage divider                |
| D4, D5, D6  | Display                                     |
| D7          | Release button                              |
| D8, D9, D10 | SPI (Used to connect to SX1262 LoRa module) |

#### Receiver

| Pin | Function                             |
| --- | ------------------------------------ |
| D3  | Low if E-Stop active, High otherwise |

### Assembly notes

- Power the receiver through the USB-C port
- The remote is powered using a battery after the separate charging board, feed the battery voltage also to `A3` using a same ratio voltage divider (e.g. 100k & 100k).
- The receiver output is an active-low digital pin; use it to drive a safety relay coil, optocoupler, or logic-level input that enforces the actual stop.

## Repository layout

| Directory                             | Description                                                    |
| ------------------------------------- | -------------------------------------------------------------- |
| `esp32_lora_estop_sender_firmware/`   | PlatformIO project for the handheld remote with OLED UI.       |
| `esp32_lora_estop_receiver_firmware/` | PlatformIO project for the base station / relay driver.        |
| `esp32_deadman_sender_firmware/`      | PlatformIO project for the optional deadman switch node.       |
| `esp32_lora_estop_firmware_common/`   | Shared library with LoRa, BLE, ESP-NOW, and CrossTalk helpers. |
| `esp32_lora_estop_interface/`         | ROS 2 interface definitions (messages & services).             |
| `esp32_lora_estop_ros/`               | ROS 2 node that bridges the receiver to the middleware.        |

## Getting started

1. **Install PlatformIO** – Either use the VS Code extension or install the CLI (`pip install platformio`).
2. **Clone the repo** – Keep the subdirectories intact so local `lib_deps` references resolve correctly.
3. **Connect the boards** – Plug each XIAO ESP32S3 via USB. The provided `platformio.ini` files expect `/dev/tty_estop_sender`, `/dev/tty_estop_receiver`, and `/dev/tty_estop_deadman_switch`; adjust them or create udev symlinks accordingly.
4. **Configure peer addresses** – See the next section to point each device at the correct MAC addresses.
5. **Build & flash** – From the respective project folder run `pio run -t upload` (or use the PlatformIO VS Code UI). Repeat for the sender, receiver, and optional deadman switch.
6. **Monitor serial output** – Use `pio device monitor` to confirm link status, RSSI, and the printed MAC addresses during bring-up.

## Configuring peer addresses

All peer MAC addresses live in `esp32_lora_estop_firmware_common/include/comm_interface.h`:

- `RECEIVER_PEER_INFO` – The addresses for the receiver on the robot (ESP-NOW + BLE).
- `SENDER_PEER_INFO` – The remote sender's addresses.
- `DEADMAN_PEER_INFO` – The addresses for the optional deadman transmitter.

Each `CommPeerInfo` holds two 6-byte values: the ESP-NOW MAC and the BLE MAC. On the ESP32, the BLE address is typically the ESP-NOW/Wi-Fi address plus one in the least-significant byte. To adapt the system to your own hardware:

1. Flash a board with any of the firmware projects and open the serial monitor. During boot the firmware prints the local BLE address, e.g.:

   ```text
   BLE Device initialized with address: D8:3B:DA:73:E4:30
   ```

2. Record the ESP-NOW address (`WiFi.macAddress()` if you need to query it manually) and the BLE address for each physical unit.
3. Update the corresponding `CommPeerInfo` constants in `comm_interface.h`. Keep the BLE entry consistent with the ESP-NOW address + 1 if you follow the default convention.
4. Rebuild and flash all nodes so they carry the same lookup table. A mismatch here is the most common cause of "stuck in Connecting..." on the OLED.

## Adapting to your platform

- **Different boards** – Change the `board` field in each `platformio.ini` and adjust the pin mappings to match your target carrier. The shared library is agnostic to the MCU as long as the underlying Arduino core provides ESP-NOW, NimBLE, and RadioLib support.
- **Custom user interface** – The OLED rendering lives in `esp32_lora_estop_sender_firmware/src/main.cpp`; swap in your display driver or button layout without touching the transport layer.
- **Alternate radios** – `LoraInterface` currently targets the SX1262 via the RadioBoards abstraction. If your hardware uses another LoRa front-end, add a new implementation in `esp32_lora_estop_firmware_common/src/` and instantiate it in `lora_interface.cpp`.
- **Integrating with other controllers** – Consume the USB CrossTalk stream directly or extend the ROS 2 node (`esp32_lora_estop_ros/src/receiver_interface_node.cpp`) to publish additional diagnostics.

## Using the system

1. Power the receiver and ensure the relay output is wired to your safety circuit.
2. Power the remote handset; the OLED shows boot progress, then "Connecting..." until at least one transport is linked.
3. Once connected, the display reports RSSI for BLE and ESP-NOW. The top banner changes to "E-Stop Active" until you press the physical release button.
4. Pressing the hard E-Stop latches the state on every transport. The receiver LED goes LOW and the relay remains asserted until the release sequence completes.
5. If the deadman transmitter is enabled, releasing its button/pedal or hitting the panic trigger instantly asserts the hard E-Stop.
6. The ROS 2 node can be launched to visualize link quality and toggle the `SetEnabled` service to disable the receiver and always enable power if needed (Note that this is dangerous!).

## Troubleshooting tips

- **No connection** – Double-check that the MAC pairs in `comm_interface.h` mirror each device’s printed addresses. A single byte mismatch prevents BLE pairing and ESP-NOW peering.
- **LoRa errors in serial log** – Ensure the antenna is attached and the SX1262 wiring matches the RadioBoards defaults. Persistent `Radio transmit error` messages usually mean the radio never finished a transmission.
- **Relay stuck active** – Inspect the `last_received_message_age_ms` field from `CommStatus` via USB; values > 300 indicate packet loss that forces the receiver into the safe state.
- **Deadman not seen** – Verify it is enabled (LED on `D1`) and that its peer info is aligned with the receiver’s `DEADMAN_PEER_INFO` entry.
