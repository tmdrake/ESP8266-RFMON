# ESP8266 RFMON

RFMON Created by GROK and Implemented by TMDrake

## Overview

ESP8266 RFMON is an RF (Radio Frequency) power monitoring system built on the **ESP8266 NodeMCU microcontroller**. It monitors RF power levels, temperature, and device health metrics, exposing all data via SNMP for remote network management and alerting.

## Features

### Core Capabilities

- **RF Power Monitoring**: Monitors RF power levels (0-1V analog input), converts readings to 0-100% power scale, and detects RF transmissions via PTT (Push-To-Talk)
- **SNMP Agent**: Exposes all monitoring data via SNMP (Simple Network Management Protocol) for remote network management tools and real-time monitoring
- **Temperature Sensing**: Reads DS18B20 digital temperature sensors via 1-Wire protocol for ambient or device temperature monitoring
- **WiFi Connectivity**: Connects to configured WiFi networks with support for both DHCP and static IP addressing
- **SNMP Traps**: Sends alert traps when PTT events occur or configuration values change
- **Persistent Configuration**: Uses EEPROM to save settings across device reboots
- **WiFi Signal Monitoring**: Tracks WiFi signal strength (RSSI)
- **Debug Mode**: Optional verbose serial output for troubleshooting

## Hardware Requirements

- **Microcontroller**: ESP8266 NodeMCU 1.0
- **Temperature Sensor**: DS18B20 digital thermometer with 4.7kΩ pull-up resistor to 3.3V
- **RF Power Tap**: 0-1V analog signal from RF detector circuit with 10kΩ pull-down recommended
- **PTT Input**: Push-to-talk signal detection on GPIO 5 (D1) with active-low logic
- **USB Serial Interface**: For configuration via UART menu

### Pin Assignments

| Function | GPIO Pin | Physical Pin |
|----------|----------|--------------|
| PTT Input | GPIO 5 | D1 |
| RF Power (Analog) | A0 | A0 |
| Temperature (1-Wire) | GPIO 2 | D4 |

## Configuration

The device provides an interactive serial menu for configuration. Connect via USB/serial at **115200 baud**:

### Menu Options

1. **Set SSID** - Configure WiFi network name
2. **Set Password** - Configure WiFi password
3. **Set Trap IP** - Set the destination IP for SNMP trap alerts
4. **Set sysContact** - Configure contact information string
5. **Set Community Strings** - Configure read/write SNMP community strings
6. **IP Setup** - Configure DHCP or static IP networking
7. **Show Current Settings** - Display all current configuration
8. **Save and Reboot** - Save settings to EEPROM and restart device
9. **Toggle Debug Output** - Enable/disable verbose serial debug output

### Persistent Settings

Configuration is stored in EEPROM and automatically loaded on startup:
- WiFi SSID & Password
- SNMP Trap Destination IP
- SNMP Community Strings (read/write)
- System Contact Information
- Network Configuration (DHCP/Static IP, Gateway, Subnet Mask)

## SNMP OIDs

### System Information (RFC1213-MIB)

| OID | Description |
|-----|-------------|
| `.1.3.6.1.2.1.1.1.0` | System Description |
| `.1.3.6.1.2.1.1.3.0` | System Uptime |
| `.1.3.6.1.2.1.1.4.0` | System Contact |
| `.1.3.6.1.2.1.1.5.0` | System Name |
| `.1.3.6.1.2.1.1.6.0` | System Location |
| `.1.3.6.1.2.1.1.7.0` | System Services |

### RF Monitor Data (Custom OIDs)

| OID | Description | Range |
|-----|-------------|-------|
| `.1.3.6.1.4.1.63637.1.0` | RF Power Level | 0-100% |
| `.1.3.6.1.4.1.63637.1.1` | PTT State | 0=Idle, 1=TX |
| `.1.3.6.1.4.1.63637.1.2` | Last PTT RF Power | 0-100% |
| `.1.3.6.1.4.1.63637.1.3` | WiFi Signal Strength | RSSI (dBm) |
| `.1.3.6.1.4.1.63637.1.4` | Temperature | Celsius × 100 |

### SNMP Traps

- **Settable Number Trap** (`.1.3.6.1.2.1.33.2`): Triggered when settable values are modified
- **PTT Trap** (`.1.3.6.1.4.1.63637.2.0`): Triggered when PTT transmit begins, includes RF power level

## Usage

1. **Flash Firmware**: Upload the sketch to your ESP8266 using Arduino IDE
2. **Configure Network**: Connect via serial terminal and use the menu to configure WiFi and SNMP settings
3. **Monitor Remotely**: Use any SNMP client (e.g., `snmpwalk`, Nagios, Zabbix) to query the device
4. **Alert Setup**: Configure SNMP trap destinations for real-time alerts

## Application

This device is designed for:
- **Amateur Radio**: RF power monitoring during transmissions
- **RF Lab Applications**: Power analysis and device health tracking
- **Network Monitoring**: SNMP-based remote device monitoring and alerting

## Dependencies

- ESP8266 Core (Arduino)
- WiFi Library (ESP8266)
- SNMP_Agent Library
- OneWire Library
- DallasTemperature Library
- EEPROM Library

## Author

**RFMON** created by GROK and implemented by TMDrake

---

*Revision 1.3.0*
