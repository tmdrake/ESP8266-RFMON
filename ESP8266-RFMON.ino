#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
#include <SNMP_Agent.h>
#include <SNMPTrap.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Pin definitions
#define PTT_PIN 5  // D1 (GPIO 5), PTT active-low, internal pull-up
#define VOLTAGE_PIN A0  // A0, RF power tap (0-1V), 10k pull-down recommended
#define TEMP_PIN 2  // D4 (GPIO 2), DS18B20, 4.7k pull-up to 3.3V

// RFC1213-MIB System OIDs
const char* oidSysDescr = ".1.3.6.1.2.1.1.1.0";
const char* oidSysObjectID = ".1.3.6.1.2.1.1.2.0";
const char* oidSysUptime = ".1.3.6.1.2.1.1.3.0";
const char* oidSysContact = ".1.3.6.1.2.1.1.4.0";
const char* oidSysName = ".1.3.6.1.2.1.1.5.0";
const char* oidSysLocation = ".1.3.6.1.2.1.1.6.0";
const char* oidSysServices = ".1.3.6.1.2.1.1.7.0";

// Network settings
char ssid[32] = "deamonnet";
char password[64] = "<hidden>";
bool useDHCP = true;                      // New: DHCP flag
IPAddress staticIP(192, 168, 254, 247);   // Static IP
IPAddress subnetMask(255, 255, 255, 0);   // Subnet mask
IPAddress gateway(192, 168, 254, 1);      // Gateway
IPAddress destinationIP(192, 168, 254, 10);
char sysContact[64] = "Your Name <your.email@example.com>";
char readCommunity[32] = "public";
char writeCommunity[32] = "private";
int wifiRssi = 0;
bool debugEnabled = false;

// DS18B20 setup
OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);
int temperature = 0; // Celsius x 100
unsigned long lastTempRead = 0;
const unsigned long tempInterval = 5000; // 5 seconds

WiFiUDP udp;
SNMPAgent snmp(readCommunity, writeCommunity);

// Variables
int changingNumber = 1;
int settableNumber = 0;
uint32_t tensOfMillisCounter = 0;
uint8_t* stuff = 0;
std::string staticString = "This value will never change";
char* changingString;
int voltageRaw = 0; // RF power % (0-100)
int pttState = 1;
int lastPttState = 1;
int lastPttVoltage = 0; // Last RF power % (0-100)
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 50;

// System OIDs
std::string sysDescr = "NodeMCU 1.0 (ESP8266) RF Power Monitor by Drakes Computers, Rev 1.3.0";
std::string sysObjectID = ".1.3.6.1.4.1.63637";
std::string sysName = "NodeMCU-RF-Monitor";
std::string sysLocation = "Lab";
int sysServices = 72;
std::string sysContactStr;

// Callback pointers
ValueCallback* changingNumberOID;
ValueCallback* settableNumberOID;
ValueCallback* voltageCallback;
ValueCallback* pttCallback;
ValueCallback* lastPttVoltageCallback;
ValueCallback* wifiRssiCallback;
ValueCallback* tempCallback;
TimestampCallback* timestampCallbackOID;
ValueCallback* sysDescrCallback;
ValueCallback* sysObjectIDCallback;
ValueCallback* sysContactCallback;
ValueCallback* sysNameCallback;
ValueCallback* sysLocationCallback;
ValueCallback* sysServicesCallback;

// Traps
SNMPTrap* settableNumberTrap = nullptr;
SNMPTrap* pttTrap = nullptr;
bool enableTraps = false;

// EEPROM structure
#define EEPROM_SIZE 256
#define EEPROM_SSID_ADDR 0
#define EEPROM_PASS_ADDR 32
#define EEPROM_IP_ADDR 96
#define EEPROM_CONTACT_ADDR 100
#define EEPROM_READ_COMMUNITY_ADDR 164
#define EEPROM_WRITE_COMMUNITY_ADDR 196
#define EEPROM_STATIC_IP_ADDR 228
#define EEPROM_SUBNET_ADDR 232
#define EEPROM_GATEWAY_ADDR 236
#define EEPROM_DHCP_FLAG 240      // New: DHCP flag (1 byte)
#define EEPROM_INIT_FLAG 241      // Adjusted

void loadNetworkSettings() {
  EEPROM.begin(EEPROM_SIZE);
  uint8_t initFlag = EEPROM.read(EEPROM_INIT_FLAG);
  if (initFlag == 0xAA) {
    for (int i = 0; i < 32; i++) ssid[i] = EEPROM.read(EEPROM_SSID_ADDR + i);
    for (int i = 0; i < 64; i++) password[i] = EEPROM.read(EEPROM_PASS_ADDR + i);
    uint8_t ip[4];
    for (int i = 0; i < 4; i++) ip[i] = EEPROM.read(EEPROM_IP_ADDR + i);
    destinationIP = IPAddress(ip[0], ip[1], ip[2], ip[3]);
    for (int i = 0; i < 64; i++) sysContact[i] = EEPROM.read(EEPROM_CONTACT_ADDR + i);
    for (int i = 0; i < 32; i++) readCommunity[i] = EEPROM.read(EEPROM_READ_COMMUNITY_ADDR + i);
    for (int i = 0; i < 32; i++) writeCommunity[i] = EEPROM.read(EEPROM_WRITE_COMMUNITY_ADDR + i);
    for (int i = 0; i < 4; i++) ip[i] = EEPROM.read(EEPROM_STATIC_IP_ADDR + i);
    staticIP = IPAddress(ip[0], ip[1], ip[2], ip[3]);
    for (int i = 0; i < 4; i++) ip[i] = EEPROM.read(EEPROM_SUBNET_ADDR + i);
    subnetMask = IPAddress(ip[0], ip[1], ip[2], ip[3]);
    for (int i = 0; i < 4; i++) ip[i] = EEPROM.read(EEPROM_GATEWAY_ADDR + i);
    gateway = IPAddress(ip[0], ip[1], ip[2], ip[3]);
    useDHCP = EEPROM.read(EEPROM_DHCP_FLAG);  // New
  }
  sysContactStr = std::string(sysContact);
  EEPROM.end();
}

void saveNetworkSettings() {
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < 32; i++) EEPROM.write(EEPROM_SSID_ADDR + i, ssid[i]);
  for (int i = 0; i < 64; i++) EEPROM.write(EEPROM_PASS_ADDR + i, password[i]);
  for (int i = 0; i < 4; i++) EEPROM.write(EEPROM_IP_ADDR + i, destinationIP[i]);
  for (int i = 0; i < 64; i++) EEPROM.write(EEPROM_CONTACT_ADDR + i, sysContact[i]);
  for (int i = 0; i < 32; i++) EEPROM.write(EEPROM_READ_COMMUNITY_ADDR + i, readCommunity[i]);
  for (int i = 0; i < 32; i++) EEPROM.write(EEPROM_WRITE_COMMUNITY_ADDR + i, writeCommunity[i]);
  for (int i = 0; i < 4; i++) EEPROM.write(EEPROM_STATIC_IP_ADDR + i, staticIP[i]);
  for (int i = 0; i < 4; i++) EEPROM.write(EEPROM_SUBNET_ADDR + i, subnetMask[i]);
  for (int i = 0; i < 4; i++) EEPROM.write(EEPROM_GATEWAY_ADDR + i, gateway[i]);
  EEPROM.write(EEPROM_DHCP_FLAG, useDHCP);  // New
  EEPROM.write(EEPROM_INIT_FLAG, 0xAA);
  EEPROM.commit();
  EEPROM.end();
}

void updateSysContactHandler() {
  sysContactStr = std::string(sysContact);
  snmp.removeHandler(sysContactCallback);
  sysContactCallback = snmp.addReadOnlyStaticStringHandler(oidSysContact, sysContactStr);
  snmp.sortHandlers();
}

void updateCommunityStrings() {
  snmp = SNMPAgent(readCommunity, writeCommunity);
  snmp.setUDP(&udp);
  snmp.begin();
  stuff = (uint8_t*)malloc(4);
  stuff[0] = 1; stuff[1] = 2; stuff[2] = 24; stuff[3] = 67;
  changingNumberOID = snmp.addIntegerHandler(".1.3.6.1.4.1.5.0", &changingNumber);
  settableNumberOID = snmp.addIntegerHandler(".1.3.6.1.4.1.5.1", &settableNumber, true);
  snmp.addIntegerHandler(".1.3.6.1.4.1.4.0", &changingNumber);
  snmp.addOpaqueHandler(".1.3.6.1.4.1.5.9", stuff, 4, true);
  snmp.addReadOnlyStaticStringHandler(".1.3.6.1.4.1.5.11", staticString);
  changingString = (char*)malloc(25 * sizeof(char));
  snprintf(changingString, 25, "This is changeable");
  snmp.addReadWriteStringHandler(".1.3.6.1.4.1.5.12", &changingString, 25, true);
  voltageCallback = snmp.addIntegerHandler(".1.3.6.1.4.1.63637.1.0", &voltageRaw, true);
  pttCallback = snmp.addIntegerHandler(".1.3.6.1.4.1.63637.1.1", &pttState, true);
  lastPttVoltageCallback = snmp.addIntegerHandler(".1.3.6.1.4.1.63637.1.2", &lastPttVoltage, true);
  wifiRssiCallback = snmp.addIntegerHandler(".1.3.6.1.4.1.63637.1.3", &wifiRssi, true);
  tempCallback = snmp.addIntegerHandler(".1.3.6.1.4.1.63637.1.4", &temperature, true);
  sysDescrCallback = snmp.addReadOnlyStaticStringHandler(oidSysDescr, sysDescr);
  sysObjectIDCallback = snmp.addReadOnlyStaticStringHandler(oidSysObjectID, sysObjectID);
  timestampCallbackOID = (TimestampCallback*)snmp.addTimestampHandler(oidSysUptime, &tensOfMillisCounter);
  sysContactCallback = snmp.addReadOnlyStaticStringHandler(oidSysContact, sysContactStr);
  sysNameCallback = snmp.addReadOnlyStaticStringHandler(oidSysName, sysName);
  sysLocationCallback = snmp.addReadOnlyStaticStringHandler(oidSysLocation, sysLocation);
  sysServicesCallback = snmp.addIntegerHandler(oidSysServices, &sysServices);
  if (enableTraps && WiFi.status() == WL_CONNECTED) {
    settableNumberTrap = new SNMPTrap(readCommunity, SNMP_VERSION_2C);
    pttTrap = new SNMPTrap(readCommunity, SNMP_VERSION_2C);
    settableNumberTrap->setUDP(&udp);
    settableNumberTrap->setTrapOID(new OIDType(".1.3.6.1.2.1.33.2"));
    settableNumberTrap->setSpecificTrap(1);
    settableNumberTrap->setUptimeCallback(timestampCallbackOID);
    settableNumberTrap->addOIDPointer(changingNumberOID);
    settableNumberTrap->addOIDPointer(settableNumberOID);
    settableNumberTrap->setIP(WiFi.localIP());
    pttTrap->setUDP(&udp);
    pttTrap->setTrapOID(new OIDType(".1.3.6.1.4.1.63637.2.0"));
    pttTrap->setSpecificTrap(1);
    pttTrap->setUptimeCallback(timestampCallbackOID);
    pttTrap->addOIDPointer(lastPttVoltageCallback);
    pttTrap->setIP(WiFi.localIP());
    pttTrap->setInform(true);
  }
  snmp.sortHandlers();
}

void displayMenu() {
  Serial.println("\n=== Network Settings Menu (Rev 1.3.0) ===");
  Serial.println("1. Set SSID");
  Serial.println("2. Set Password");
  Serial.println("3. Set Trap IP");
  Serial.println("4. Set sysContact");
  Serial.println("5. Set Community Strings");
  Serial.println("6. IP Setup");              // New: Moved to 6, renamed
  Serial.println("7. Show Current Settings");  // Shifted to 7
  Serial.println("8. Save and Reboot");       // Shifted to 8
  Serial.println("9. Toggle Debug Output");    // Shifted to 9
  Serial.print("Enter choice (1-9): ");
}

void handleMenu() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input == "1") {
      Serial.print("Enter new SSID (max 31 chars): ");
      while (!Serial.available()) yield();
      input = Serial.readStringUntil('\n');
      input.trim();
      if (input.length() < 32) {
        strncpy(ssid, input.c_str(), 32);
        Serial.println("SSID updated.");
      } else {
        Serial.println("Error: SSID too long.");
      }
    } else if (input == "2") {
      Serial.print("Enter new Password (max 63 chars): ");
      while (!Serial.available()) yield();
      input = Serial.readStringUntil('\n');
      input.trim();
      if (input.length() < 64) {
        strncpy(password, input.c_str(), 64);
        Serial.println("Password updated.");
      } else {
        Serial.println("Error: Password too long.");
      }
    } else if (input == "3") {
      Serial.print("Enter new Trap IP (xxx.xxx.xxx.xxx): ");
      while (!Serial.available()) yield();
      input = Serial.readStringUntil('\n');
      input.trim();
      IPAddress newIP;
      if (newIP.fromString(input)) {
        destinationIP = newIP;
        Serial.println("Trap IP updated.");
      } else {
        Serial.println("Error: Invalid IP format.");
      }
    } else if (input == "4") {
      Serial.print("Enter new sysContact (max 63 chars): ");
      while (!Serial.available()) yield();
      input = Serial.readStringUntil('\n');
      input.trim();
      if (input.length() < 64) {
        strncpy(sysContact, input.c_str(), 64);
        updateSysContactHandler();
        Serial.println("sysContact updated.");
      } else {
        Serial.println("Error: sysContact too long.");
      }
    } else if (input == "5") {
      Serial.print("Enter new Read Community (max 31 chars): ");
      while (!Serial.available()) yield();
      input = Serial.readStringUntil('\n');
      input.trim();
      if (input.length() < 32) {
        strncpy(readCommunity, input.c_str(), 32);
        Serial.println("Read Community updated.");
      } else {
        Serial.println("Error: Read Community too long.");
      }
      Serial.print("Enter new Write Community (max 31 chars): ");
      while (!Serial.available()) yield();
      input = Serial.readStringUntil('\n');
      input.trim();
      if (input.length() < 32) {
        strncpy(writeCommunity, input.c_str(), 32);
        updateCommunityStrings();
        Serial.println("Write Community updated.");
      } else {
        Serial.println("Error: Write Community too long.");
      }
    } else if (input == "6") {  // New: IP Setup
      Serial.print("Use DHCP? (Y/N): ");
      while (!Serial.available()) yield();
      input = Serial.readStringUntil('\n');
      input.trim();
      if (input.equalsIgnoreCase("Y")) {
        useDHCP = true;
        Serial.println("DHCP enabled.");
      } else if (input.equalsIgnoreCase("N")) {
        useDHCP = false;
        Serial.print("Enter new Static IP (xxx.xxx.xxx.xxx): ");
        while (!Serial.available()) yield();
        input = Serial.readStringUntil('\n');
        input.trim();
        IPAddress newIP;
        if (newIP.fromString(input)) {
          staticIP = newIP;
          Serial.println("Static IP updated.");
        } else {
          Serial.println("Error: Invalid IP format.");
        }
        Serial.print("Enter new Subnet Mask (xxx.xxx.xxx.xxx): ");
        while (!Serial.available()) yield();
        input = Serial.readStringUntil('\n');
        input.trim();
        if (newIP.fromString(input)) {
          subnetMask = newIP;
          Serial.println("Subnet Mask updated.");
        } else {
          Serial.println("Error: Invalid Subnet Mask format.");
        }
        Serial.print("Enter new Gateway (xxx.xxx.xxx.xxx): ");
        while (!Serial.available()) yield();
        input = Serial.readStringUntil('\n');
        input.trim();
        if (newIP.fromString(input)) {
          gateway = newIP;
          Serial.println("Gateway updated.");
        } else {
          Serial.println("Error: Invalid Gateway format.");
        }
      } else {
        Serial.println("Error: Enter Y or N.");
      }
    } else if (input == "7") {
      Serial.println("\nCurrent Settings:");
      Serial.print("SSID: "); Serial.println(ssid);
      Serial.print("Password: "); Serial.println(password);
      Serial.print("Trap IP: "); Serial.println(destinationIP);
      Serial.print("sysContact: "); Serial.println(sysContact);
      Serial.print("Read Community: "); Serial.println(readCommunity);
      Serial.print("Write Community: "); Serial.println(writeCommunity);
      Serial.print("DHCP: "); Serial.println(useDHCP ? "Enabled" : "Disabled");  // New
      if (!useDHCP) {
        Serial.print("Static IP: "); Serial.println(staticIP);
        Serial.print("Subnet Mask: "); Serial.println(subnetMask);
        Serial.print("Gateway: "); Serial.println(gateway);
      }
    } else if (input == "8") {
      Serial.println("Saving settings and rebooting...");
      saveNetworkSettings();
      ESP.restart();
    } else if (input == "9") {
      debugEnabled = !debugEnabled;
      Serial.println(debugEnabled ? "Debug output enabled." : "Debug output disabled.");
    } else {
      Serial.println("Invalid choice.");
    }
    displayMenu();
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PTT_PIN, INPUT_PULLUP);
  pinMode(VOLTAGE_PIN, INPUT);
  sensors.begin();
  sensors.setResolution(9);
  loadNetworkSettings();
  displayMenu();
  if (!useDHCP) {  // New: Apply static IP only if DHCP is disabled
    WiFi.config(staticIP, gateway, subnetMask);
  }
  WiFi.begin(ssid, password);
  Serial.println("\nStarting WiFi...");
  unsigned long startAttempt = millis();
  const unsigned long wifiTimeout = 30000;
  while (WiFi.status() != WL_CONNECTED && (millis() - startAttempt < wifiTimeout)) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed. Use UART menu to update settings.");
    displayMenu();
  }
  snmp.setUDP(&udp);
  snmp.begin();
  stuff = (uint8_t*)malloc(4);
  stuff[0] = 1; stuff[1] = 2; stuff[2] = 24; stuff[3] = 67;
  changingNumberOID = snmp.addIntegerHandler(".1.3.6.1.4.1.5.0", &changingNumber);
  settableNumberOID = snmp.addIntegerHandler(".1.3.6.1.4.1.5.1", &settableNumber, true);
  snmp.addIntegerHandler(".1.3.6.1.4.1.4.0", &changingNumber);
  snmp.addOpaqueHandler(".1.3.6.1.4.1.5.9", stuff, 4, true);
  snmp.addReadOnlyStaticStringHandler(".1.3.6.1.4.1.5.11", staticString);
  changingString = (char*)malloc(25 * sizeof(char));
  snprintf(changingString, 25, "This is changeable");
  snmp.addReadWriteStringHandler(".1.3.6.1.4.1.5.12", &changingString, 25, true);
  voltageCallback = snmp.addIntegerHandler(".1.3.6.1.4.1.63637.1.0", &voltageRaw, true);
  pttCallback = snmp.addIntegerHandler(".1.3.6.1.4.1.63637.1.1", &pttState, true);
  lastPttVoltageCallback = snmp.addIntegerHandler(".1.3.6.1.4.1.63637.1.2", &lastPttVoltage, true);
  wifiRssiCallback = snmp.addIntegerHandler(".1.3.6.1.4.1.63637.1.3", &wifiRssi, true);
  tempCallback = snmp.addIntegerHandler(".1.3.6.1.4.1.63637.1.4", &temperature, true);
  sysDescrCallback = snmp.addReadOnlyStaticStringHandler(oidSysDescr, sysDescr);
  sysObjectIDCallback = snmp.addReadOnlyStaticStringHandler(oidSysObjectID, sysObjectID);
  timestampCallbackOID = (TimestampCallback*)snmp.addTimestampHandler(oidSysUptime, &tensOfMillisCounter);
  sysContactCallback = snmp.addReadOnlyStaticStringHandler(oidSysContact, sysContactStr);
  sysNameCallback = snmp.addReadOnlyStaticStringHandler(oidSysName, sysName);
  sysLocationCallback = snmp.addReadOnlyStaticStringHandler(oidSysLocation, sysLocation);
  sysServicesCallback = snmp.addIntegerHandler(oidSysServices, &sysServices);
  if (enableTraps && WiFi.status() == WL_CONNECTED) {
    settableNumberTrap = new SNMPTrap(readCommunity, SNMP_VERSION_2C);
    pttTrap = new SNMPTrap(readCommunity, SNMP_VERSION_2C);
    settableNumberTrap->setUDP(&udp);
    settableNumberTrap->setTrapOID(new OIDType(".1.3.6.1.2.1.33.2"));
    settableNumberTrap->setSpecificTrap(1);
    settableNumberTrap->setUptimeCallback(timestampCallbackOID);
    settableNumberTrap->addOIDPointer(changingNumberOID);
    settableNumberTrap->addOIDPointer(settableNumberOID);
    settableNumberTrap->setIP(WiFi.localIP());
    pttTrap->setUDP(&udp);
    pttTrap->setTrapOID(new OIDType(".1.3.6.1.4.1.63637.2.0"));
    pttTrap->setSpecificTrap(1);
    pttTrap->setUptimeCallback(timestampCallbackOID);
    pttTrap->addOIDPointer(lastPttVoltageCallback);
    pttTrap->setIP(WiFi.localIP());
    pttTrap->setInform(true);
  }
  snmp.sortHandlers();
}

void loop() {
  snmp.loop();
  handleMenu();
  if (WiFi.status() == WL_CONNECTED) {
    wifiRssi = WiFi.RSSI();
  } else {
    wifiRssi = 0;
  }
  unsigned long currentMillis = millis();
  if (currentMillis - lastTempRead >= tempInterval) {
    sensors.requestTemperatures();
    float tempC = sensors.getTempCByIndex(0);
    if (tempC != DEVICE_DISCONNECTED_C) {
      temperature = (int)(tempC * 100);
    } else {
      temperature = -9999;
    }
    lastTempRead = currentMillis;
  }
  if (enableTraps && WiFi.status() == WL_CONNECTED && settableNumberOID->setOccurred) {
    Serial.printf("Number has been set to value: %i\n", settableNumber);
    if (settableNumber % 2 == 0) {
      settableNumberTrap->setVersion(SNMP_VERSION_2C);
      settableNumberTrap->setInform(true);
    } else {
      settableNumberTrap->setVersion(SNMP_VERSION_1);
      settableNumberTrap->setInform(false);
    }
    settableNumberOID->resetSetOccurred();
    if (snmp.sendTrapTo(settableNumberTrap, destinationIP, true, 2, 5000) != INVALID_SNMP_REQUEST_ID) {
      Serial.println("Sent settableNumber SNMP Trap");
    } else {
      Serial.println("Couldn't send settableNumber SNMP Trap");
    }
  }
  lastPttState = pttState;
  pttState = digitalRead(PTT_PIN);
  if (pttState == 0) {
    int raw = analogRead(VOLTAGE_PIN);
    if (raw >= 10) {
      voltageRaw = map(raw, 0, 310, 0, 100);
      voltageRaw = constrain(voltageRaw, 0, 100);
    } else {
      voltageRaw = 0;
    }
    lastPttVoltage = voltageRaw;
  } else {
    voltageRaw = 0;
  }
  if (enableTraps && WiFi.status() == WL_CONNECTED && pttState == 0 && lastPttState == 1) {
    Serial.println("Sending PTT trap...");
    Serial.print("Trap OID: .1.3.6.1.4.1.63637.2.0, RF Power: ");
    Serial.println(lastPttVoltage);
    if (snmp.sendTrapTo(pttTrap, destinationIP, true, 2, 5000) != INVALID_SNMP_REQUEST_ID) {
      Serial.println("PTT triggered, trap sent with RF power: " + String(lastPttVoltage) + "%");
    } else {
      Serial.println("Couldn't send PTT SNMP Trap");
    }
  }
  if (currentMillis - lastUpdate >= updateInterval) {
    changingNumber++;
    tensOfMillisCounter = millis() / 10;
    if (debugEnabled) {
      Serial.print("PTT: ");
      Serial.print(pttState);
      Serial.print(" | RF Power: ");
      Serial.print(voltageRaw);
      Serial.print("% | Last PTT Power: ");
      Serial.print(lastPttVoltage);
      Serial.print("% | Temperature: ");
      if (temperature == -9999) {
        Serial.print("Error");
      } else {
        Serial.print(temperature / 100.0);
        Serial.print("C");
      }
      Serial.print(" | Free Heap: ");
      Serial.println(ESP.getFreeHeap());
    }
    lastUpdate = currentMillis;
  }
}