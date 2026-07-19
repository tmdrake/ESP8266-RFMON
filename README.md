# ESP8266 RFMON

**RF Power Monitor & SNMP Agent for Amateur Radio / Ham Radio Applications**

Created with assistance from Grok (xAI) and implemented by [TMDrake](https://github.com/tmdrake).

![License](https://img.shields.io/github/license/tmdrake/ESP8266-RFMON)
![Language](https://img.shields.io/badge/language-C%2B%2B-blue)
![Platform](https://img.shields.io/badge/platform-ESP8266-orange)

## About

This project turns a low-cost **NodeMCU (ESP8266)** into a compact, network-enabled **RF Power Monitor** with full SNMP (Simple Network Management Protocol) support. It detects PTT (Push-To-Talk) activation, measures RF power via an analog tap, monitors temperature with a DS18B20 sensor, and exposes everything via SNMP for integration with network monitoring tools (e.g., Zabbix, LibreNMS, PRTG, or custom scripts).

Key use case: Monitor your amateur radio transceiver or amplifier remotely — get instant alerts when transmitting and track power levels, temperature, and device health.

**Rev 1.3.0** — Includes DHCP/static IP flexibility, improved EEPROM persistence, debug output, and robust SNMP trap support.

## Features

- **PTT Detection** — Active-low input (GPIO5/D1) with internal pull-up.
- **RF Power Monitoring** — Analog input (A0) scaled to 0-100% (configurable mapping).
- **Temperature Monitoring** — DS18B20 1-Wire sensor (GPIO2/D4).
- **Full SNMPv1/v2c Agent** — RFC1213-MIB system info + custom enterprise OIDs.
- **SNMP Traps** — Sends traps on PTT activation and settable value changes (with Inform support).
- **WiFi Configuration** — Persistent SSID/password, DHCP or static IP via serial menu.
- **EEPROM Persistence** — Saves all settings (network, communities, contact info) across reboots.
- **Serial Configuration Menu** — Easy setup without recompiling.
- **Debug Output** — Toggleable real-time monitoring via Serial.
- **Low Resource** — Runs comfortably on ESP8266 with free heap monitoring.

## Hardware Requirements

### Components
- **ESP8266 Board**: NodeMCU 1.0 (V3) or equivalent (recommended).
- **DS18B20 Temperature Sensor** — Waterproof or TO-92 package + 4.7kΩ pull-up resistor.
- **RF Power Tap** — Voltage divider or directional coupler outputting 0-1V (safe for ESP ADC).
- **PTT Signal** — Active-low from transceiver (or opto-isolated for safety).

### Recommended Wiring

| Pin       | GPIO   | Function                  | Notes |
|-----------|--------|---------------------------|-------|
| D1        | GPIO5  | PTT (active-low)          | Internal pull-up enabled |
| A0        | ADC    | RF Power (0-1V)           | 10kΩ pull-down recommended |
| D4        | GPIO2  | DS18B20 Data              | 4.7kΩ pull-up to 3.3V |
| GND       | -      | Ground                    | Common ground |
| 3.3V / 5V | -      | Power                     | DS18B20 can use 5V (level shift if needed) |

**Safety Note**: Use proper RF isolation / attenuators when tapping transmitter output. Do **not** connect high RF voltages directly to the ESP8266.

## Software Setup

### Prerequisites
- [Arduino IDE](https://www.arduino.cc/en/software) or PlatformIO.
- ESP8266 Board Support (add `http://arduino.esp8266.com/stable/package_esp8266com_index.json` in Preferences).
- Required Libraries (install via Library Manager):
  - `ESP8266WiFi`
  - `WiFiUDP` (included)
  - [SNMP_Agent](https://github.com/0neblock/SNMP_Agent) (and SNMPTrap)
  - `OneWire`
  - `DallasTemperature`
  - `EEPROM` (built-in)

### Installation
1. Clone the repo:
   ```bash
   git clone https://github.com/tmdrake/ESP8266-RFMON.git
