//  Home Assistant Harmony hub for ESP32
//  The purpose of the hub is to receive Harmony remote commands via nRF24L01+
//  and to format these and pass them on over MQTT to Home Assistant

// arduino_secrets.h MUST contain the following #define statements
// BROKER_ADDR - the IP address of the MQTT broker in format IPAddress(127,0,0,1)
// By default, USE_WIRED is false (see below), so, you must also have the WiFi credentials, not needed if USE_WIRED is true
//   SECRET_SSID - the SSID of the wireless network to which you want to connect
//   SECRET_PASS - the password for the wireless network to which you want to connect

// arduino_secrets.h CAN contain the following #define statements to override defaults
// USE_WIRED - whether to use wired network config (default false)
//   this only supports RTL8201 based LAN devices
// CE - the pin number or name for the CE connection to the radio (default 14)
// CSN - the pin number or name for the CSN connection to the radio (default 8)
// RADIO_CH - the channel to listen for remotes, Choose 5 (default),8,14,17,32,35,41,44,62,65,71 or 74
// DEVICE_NAME - the name of the remote (default Harmony OpenHub)
// CLICK_DURATION - maximal duration of a click in ms, and minimal duration for a long press (default 500)
// WAIT_DURATION - time in ms to wait after a click to register additional clicks (default 200)
// SECOND_REPEAT_DURATION - time in ms to start repeating inputs for type 1 command (defaullt 1000)
// FURTHER_REPEAT_DURATION - time in ms between subsequent repeated inputs for type 1 commands (default 250)
// BROKER_USER - if you are using MQTT auth, the MQTT username
// BROKER_PASS - if you are using MQTT auth the MQTT password
// REMOTE_ADR - the Harmony remote address for your remote in the format 0xF305984508
//   You only need this if you are trying to manually load a remote address because you don't have the hub anymore.
//   Once you reload the sketch with this, you can use the Migrate button in the HA device to save the remote address
//   to flash.  After that you can remove this and reload the sketch one more time.
// Harmony commands
//   Type 0 : Only accept single clicks separated nu button releases (most responsive)
//   Type 1 : Generated repeated clicks when button is held 
//   Type 2 : Registers single clicks, double clicks, multiple clicks (three or more), or long presses
// CMD_OK - key press (default 0)
// CMD_UP - key press (default 1)
// CMD_DOWN - key press (default 1)
// CMD_LEFT - key press (default 1)
// CMD_RIGHT - key press (default 1)
// CMD_VOL_UP - key press (default 1)
// CMD_VOL_DOWN - key press (default 1)
// CMD_CH_UP - key press (default 1)
// CMD_CH_DOWN - key press (default 1)
// CMD_MUTE - key press (default 0)
// CMD_RETURN - key press (default 0)
// CMD_EXIT - key press (default 0)
// CMD_MENU - key press (default 0)
// CMD_DVR - key press (default 0)
// CMD_GUIDE - key press (default 0)
// CMD_INFO - key press (default 0)
// CMD_RED - key press (default 0)
// CMD_GREEN - key press (default 0)
// CMD_YELLOW - key press (default 0)
// CMD_BLUE - key press (default 0)
// CMD_BACKWARD - key press (default 1)
// CMD_FORWARD - key press (default 1)
// CMD_PLAY - key press (default 0)
// CMD_PAUSE - key press (default 0)
// CMD_STOP - key press (default 0)
// CMD_REC - key press (default 0)
// CMD_MUSIC - key press (default 2)
// CMD_TV - key press (default 2)
// CMD_MOVIE - key press (default 2)
// CMD_OFF - key press (default 0)
// CMD_NUM0 - key press (default 0)
// CMD_NUM1 - key press (default 0)
// CMD_NUM2 - key press (default 0)
// CMD_NUM3 - key press (default 0)
// CMD_NUM4 - key press (default 0)
// CMD_NUM5 - key press (default 0)
// CMD_NUM6 - key press (default 0)
// CMD_NUM7 - key press (default 0)
// CMD_NUM8 - key press (default 0)
// CMD_NUM9 - key press (default 0)
// CMD_DOTDOT - key press (default 0)
// CMD_DOTE - key press (default 0)
// CMD_LIGHT1 - key press (default 0)
// CMD_LIGHT2 - key press (default 0)
// CMD_SOCKET1 - key press (default 0)
// CMD_SOCKET2 - key press (default 0)
// CMD_PLUS - key press (default 0)
// CMD_MINUS - key press (default 0)

#define SOFTWARE_VERSION "2.3.2"
#define MANUFACTURER "pkscout"
#define MODEL "Harmony Companion OpenHub"
#define CONFIG_URL "https://github.com/pkscout/Harmoino"

#include "arduino_secrets.h"
#include "defaults.h"
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <ArduinoHA.h>
#include <Preferences.h>
#if USE_WIRED
#include <ETH.h>
#define NETWORK_CONNECT ETH.begin(ETH_PHY_SET, PHY_ADR, SMI_MDC, SMI_MDIO, PHY_RESET, ETH_GPIO_CLK_SET)
#define GET_MAC ETH.macAddress(mac)
#define NETWORK_DOWN ETH.localIP() == IPAddress(0, 0, 0, 0)
#define NETWORK_CLIENT NetworkClient client
#define GET_LOCALIP ETH.localIP()
#define GET_WIFI_RSSI ETH.linkSpeed()
#else
#include <WiFi.h>
#define NETWORK_CONNECT WiFi.begin(SECRET_SSID, SECRET_PASS)
#define GET_MAC WiFi.macAddress(mac)
#define NETWORK_DOWN WiFi.status() != WL_CONNECTED
#define NETWORK_CLIENT WiFiClient client
#define GET_LOCALIP WiFi.localIP()
#define GET_WIFI_RSSI WiFi.RSSI()
#endif

Preferences prefs;
uint64_t address = 0;
char addressChar[50];

typedef struct {
  uint32_t id;
  int type;
  char *name;
} harmonyCommandT;

harmonyCommandT harmonyCommandList[] = 
 {{0x005800C1,CMD_OK,"ok"},
  {0x005200C1,CMD_UP,"up"},
  {0x05100C1,CMD_DOWN,"down"},
  {0x005000C1,CMD_LEFT,"left"},
  {0x004F00C1,CMD_RIGHT,"right"},
  {0x0000E9C3,CMD_VOL_UP,"volume_up"},
  {0x0000EAC3,CMD_VOL_DOWN,"volume_down"},
  {0x00009CC3,CMD_CH_UP,"channel_up"},
  {0x00009DC3,CMD_CH_DOWN,"channel_down"},
  {0x0000E2C3,CMD_MUTE,"mute"},
  {0x000224C3,CMD_RETURN,"return"},
  {0x000094C3,CMD_EXIT,"exit"},
  {0x006500C1,CMD_MENU,"menu"},
  {0x00009AC3,CMD_DVR,"dvr"},
  {0x00008DC3,CMD_GUIDE,"guide"},
  {0x0001FFC3,CMD_INFO,"info"},
  {0x0001F7C3,CMD_RED,"red"},
  {0x0001F6C3,CMD_GREEN,"green"},
  {0x0001F5C3,CMD_YELLOW,"yellow"},
  {0x0001F4C3,CMD_BLUE,"blue"},
  {0x0000B4C3,CMD_BACKWARD,"backward"},
  {0x0000B3C3,CMD_FORWARD,"forward"},
  {0x0000B0C3,CMD_PLAY,"play"},
  {0x0000B1C3,CMD_PAUSE,"pause"},
  {0x0000B7C3,CMD_STOP,"stop"},
  {0x0000B2C3,CMD_REC,"rec"},
  {0x0001E8C3,CMD_MUSIC,"music"},
  {0x0001EDC3,CMD_TV,"tv"},
  {0x0001E9C3,CMD_MOVIE,"movie"},
  {0x0001ECC3,CMD_OFF,"off"},
  {0x001E00C1,CMD_NUM1,"number1"},
  {0x001F00C1,CMD_NUM2,"number2"},
  {0x002000C1,CMD_NUM3,"number3"},
  {0x002100C1,CMD_NUM4,"number4"},
  {0x002200C1,CMD_NUM5,"number5"},
  {0x002300C1,CMD_NUM6,"number6"},
  {0x002400C1,CMD_NUM7,"number7"},
  {0x002500C1,CMD_NUM8,"number8"},
  {0x002600C1,CMD_NUM9,"number9"},
  {0x002700C1,CMD_NUM0,"number0"},
  {0x005600C1,CMD_DOTDOT,"dotdot"},
  {0x002800C1,CMD_DOTE,"dote"},
  {0x000FF2C3,CMD_LIGHT1,"light1"},
  {0x000FF3C3,CMD_LIGHT2,"light2"},
  {0x000FF4C3,CMD_SOCKET1,"socket1"},
  {0x000FF5C3,CMD_SOCKET2,"socket2"},
  {0x000FF0C3,CMD_PLUS,"plus"},
  {0x000FF1C3,CMD_MINUS,"minus"},
  {0,0,"null"}};

// Network and Home Assistant mqtt clients
NETWORK_CLIENT;
HADevice device;
HAMqtt mqtt(client, device);
unsigned long shortLastUpdateAt = 0;
unsigned long longLastUpdateAt = 0;
char mqttPayload[50];
char uptimeChar[50];
char macChar[50];
char ipChar[20];
char rssiChar[50];
bool radioActive = false;
byte mac[6];
HASensor keyPress("keyPress");
HASensor upTime("upTime");
HASensor macAddress("macAddress");
HASensor ipAddress("ipAddress");
HASensor wifiRssi("wifiRssi");
HASensor remoteAddress("remoteAddress");
HABinarySensor radioStatus("radioStatus");
HAButton rebootDevice("rebootDevice");
HAButton resetDevice("resetDevice");
HAButton migrateAddress("migrateAddress");

// nRF24L01+ radio
RF24 radio(CE_PIN, CSN_PIN);

// nRF24L01+ buffers
char dataReceived[32]; // this must match dataToSend in the TX (need more than 10?)
uint8_t payloadSize;
bool newData = false;

// Harmony RF24 network and radio parameters
const uint64_t pairAddress = 0xBB0ADCA575; // Common pairing RF24 address
const uint8_t channels[12] = {5,8,14,17,32,35,41,44,62,65,71,74}; // Possible RF24 channels
int channelId = 0;

// Messages to send to Harmony Hub to get regular remote RF24 address
const byte pairMessage[22] = {242,95,1,225,154,157,218,83,40,64,30,4,2,7,12,0,0,0,0,0,102,100};
const byte pingMessage[5] = {242,64,1,225,236};
int pingRetries = 0;

// Harmony logic messages
const uint32_t harmonyHold = 0x98280040;
const uint32_t harmonyPing = 0x704C0440;
const uint32_t harmonySleep = 0x0000034F;

char harmonyDefaultCommandName[9];
harmonyCommandT harmonyDefaultCommand = {0,0,harmonyDefaultCommandName};

// Harmony state logic
unsigned long harmonyPressTime = 0;
unsigned long harmonyReleaseTime = 0;
unsigned long harmonyHoldTime = 0;
unsigned long harmonyRepeatTime = 0;
unsigned int harmonyPressCounter = 0;
unsigned int harmonyRepeatCounter = 0;
harmonyCommandT harmonyCurrentCommand = harmonyDefaultCommand;

void setup() {
  // Setup communication protocols
  Serial.begin(115200);
  delay(10000);
  Serial.println("----Starting Device----");
  setupPreferences();
  setupNetwork();
  setupNrf24(); 
  setupHomeAssistant();
}

void setupPreferences() {
  prefs.begin("harmonio", false);
  address = prefs.getULong64("remote_address", 0);
  if (address) {
    sprintf(addressChar, "0x%llX", address);
    Serial.print("The remote address is ");
    Serial.println(addressChar);
  } else {
    sprintf(addressChar, "ready for pairing");
    Serial.println("No remote address, triggering initial setup");
  }
}

void setupNetwork() {
  if (USE_WIRED) {
    Serial.println("Connecting to network");
  } else {
    Serial.print("Connecting to ");
    Serial.print(SECRET_SSID);
  }
  NETWORK_CONNECT;
  while ( NETWORK_DOWN ) {
    delay(500);
    Serial.print(".");
  }
  GET_MAC;
  sprintf(macChar, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println(""),
  Serial.print("Mac Address: ");
  Serial.println(macChar);
  IPAddress ip = GET_LOCALIP;
  snprintf(ipChar, sizeof(ipChar), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  Serial.print("IP address: ");
  Serial.println(ipChar);
  sprintf(rssiChar, "%d", GET_WIFI_RSSI);
  if (USE_WIRED){
    Serial.print("Link speed: ");
    Serial.print(rssiChar);
    Serial.println(" Mb/s");
  } else {
    Serial.print("RSSI: ");
    Serial.print(rssiChar);
    Serial.println(" dBm");
  }
  
}

void setupNrf24() {
  SPI.begin();
  radioActive = radio.begin(&SPI);
  if( !radioActive ) {
    Serial.println("nRF24L01+ Radio hardware not responding");
  } else {
    Serial.println("nRF24L01+ Radio hardware started");
    radioActive = true;
    // nRF24L01+ radio settings (fixed to match Harmony remotes)
    if (address){
      radio.setChannel(RADIO_CH);
      radio.setDataRate(RF24_2MBPS);
      radio.enableDynamicPayloads();
      radio.setCRCLength (RF24_CRC_16);
      radio.openReadingPipe(1, address & 0xFFFFFFFF00);
      radio.openReadingPipe(2, address & 0xFFFFFFFFFF);
      radio.startListening();
      Serial.println("nRF24L01+ Radio hardware configured");
    } else {
      radio.setDataRate(RF24_2MBPS);
      radio.enableDynamicPayloads();
      radio.enableAckPayload();
      radio.setCRCLength (RF24_CRC_16);
      radio.openWritingPipe(pairAddress);
      Serial.println("Power on the Harmony Smart Hub and press the pair/reset button");
    }
  }
}

void setupHomeAssistant() {
 // setup HA device
  device.setUniqueId(mac, sizeof(mac));
  device.setName(DEVICE_NAME);
  device.setSoftwareVersion(SOFTWARE_VERSION);
  device.setManufacturer(MANUFACTURER);
  device.setModel(MODEL);
  device.setConfigurationUrl(CONFIG_URL);
  device.enableExtendedUniqueIds();
  device.enableSharedAvailability();
  device.enableLastWill();
  keyPress.setName("Key Press");
  keyPress.setIcon("mdi:button-pointer");
  keyPress.setForceUpdate(true);
  keyPress.setExpireAfter(2);
  upTime.setName("Uptime");
  upTime.setEntityCategory("diagnostic");
  upTime.setIcon("mdi:clock-check-outline");
  upTime.setExpireAfter(40);
  macAddress.setName("MAC Address");
  macAddress.setIcon("mdi:ethernet");
  macAddress.setEntityCategory("diagnostic");
  ipAddress.setName("IP Address");
  ipAddress.setIcon("mdi:network-outline");
  ipAddress.setEntityCategory("diagnostic");
  if (USE_WIRED) {
    wifiRssi.setName("Link Speed");
    wifiRssi.setIcon("mdi:speedometer");
    wifiRssi.setUnitOfMeasurement("Mb/s");
  } else {
    wifiRssi.setName("RSSI");
    wifiRssi.setIcon("mdi:wifi");
    wifiRssi.setUnitOfMeasurement("dBm");
  }
  wifiRssi.setEntityCategory("diagnostic");
  radioStatus.setName("Radio Status");
  radioStatus.setIcon("mdi:radio-tower");
  radioStatus.setEntityCategory("diagnostic");
  remoteAddress.setName("Remote Address");
  remoteAddress.setIcon("mdi:remote");
  remoteAddress.setEntityCategory("diagnostic");
  rebootDevice.setName("Reboot");
  rebootDevice.setIcon("mdi:restart");
  rebootDevice.setEntityCategory("config");
  rebootDevice.onCommand(onButtonCommand);
  resetDevice.setName("Reset");
  resetDevice.setIcon("mdi:refresh");
  resetDevice.setEntityCategory("config");
  resetDevice.onCommand(onButtonCommand);
  migrateAddress.setName("Migrate");
  migrateAddress.setIcon("mdi:file-move-outline");
  migrateAddress.setEntityCategory("config");
  migrateAddress.onCommand(onButtonCommand);
  // start MQTT connection
  Serial.print("Starting connection to MQTT broker at ");
  Serial.println(BROKER_ADDR);
  mqtt.begin(BROKER_ADDR, BROKER_PORT, BROKER_USER, BROKER_PASS);
}

void onButtonCommand(HAButton* sender) {
  if (sender == &rebootDevice) {
    Serial.println("rebooting device");
    ESP.restart();
  } else if (sender == &resetDevice) {
    prefs.remove("remote_address");
    Serial.print("resetting device");
    while (prefs.getULong64("remote_address", 0)) {
      Serial.print(".");
      delay(100);
    }
    Serial.println("");
    Serial.println("rebooting device");
    ESP.restart();
  } else if (sender == &migrateAddress){
    if (REMOTE_ADR){
      Serial.println("migrating the remote address from ardunio_secrets.h to flash");
      sprintf(addressChar, "0x%llX", REMOTE_ADR);
      savePreference();
      restartDevice();
    } else {
      Serial.println("no remote address in ardunio_secrets.h to migrate");
    }
  }
}

void savePreference() {
  Serial.print("The remote RF24 address is: ");
  Serial.println(addressChar);
  remoteAddress.setValue(addressChar);
  prefs.putULong64("remote_address", strtoull(addressChar, nullptr, 16));
  Serial.print("saving preference");
  while (!prefs.getULong64("remote_address", 0)) {
    Serial.print(".");
    delay(100);
  }
  Serial.println("");
}

void restartDevice() {
  Serial.println("rebooting device");
  ESP.restart();
}

void initialSetup() {
  remoteAddress.setValue(addressChar);
  // Send out data to trigger the Hub
  if(pingRetries == 0) {
      radio.setChannel(channels[channelId]);
      if(radio.write(&pairMessage, sizeof(pairMessage))) {
          pingRetries = 10;
      } else {
          channelId++;
          if(channelId > 11) channelId = 0;
      }
  } else {
      radio.write(&pingMessage, sizeof(pingMessage));
      pingRetries--;
  }
  delay(100);
  // Look for and interpret ACK payloads
  if(radio.isAckPayloadAvailable()) {
      uint8_t dataReceived[32];
      int payloadSize = radio.getDynamicPayloadSize();
      radio.read(&dataReceived,payloadSize);

      if(payloadSize == 22) {
          sprintf(addressChar, "0x");
          for(int i=3;i<8;i++) {
              char tmp[3];
              if(i < 7) sprintf(tmp,"%.2X",dataReceived[i]);
              else sprintf(tmp,"%.2X",dataReceived[i]-1);
              strcat(addressChar, tmp);
          }
          Serial.println("");
          savePreference();
          restartDevice();
      }      
  }
}

void regularRun() {
  uint8_t pipeNum;
  if ( radio.available(&pipeNum) ) {
      // Read packet
      uint8_t dataReceived[32];
      int payloadSize = radio.getDynamicPayloadSize();
      radio.read(&dataReceived,payloadSize);
      if (pipeNum > 0){
        radioActive = true;
        // Print contents of nRF25L01+ packet
        Serial.print("nRF24L01 received 0x");
        for(int i=0; i<payloadSize; i++) {
          char tmp[3];
          sprintf(tmp,"%.2X",dataReceived[i]);
          Serial.print(tmp);
          if(i<payloadSize-1) Serial.print(".");
        }
        Serial.print(" (");
        Serial.print(payloadSize);
        Serial.print(" bytes on pipe ");
        Serial.print(pipeNum);
        Serial.println(")");
      } else if (radioActive) {
        Serial.println("Radio has gone offline. Check connections, then reboot");
        radioActive = false;
      }

      // Interpret data from Harmony remote
      if(payloadSize >= 5) { // Should always be true but just to make sure
        uint32_t commandId = 0;
        for(int i=5; i > 0; i--) {
          commandId <<= 8;
          commandId += (uint32_t) dataReceived[i];
        }

        // Harmony state logic
        if((commandId & 0x000000F0) == 0x000000C0) { // Button id
          if(commandId == (harmonyCurrentCommand.id & 0x000000FF)) { // Button released
            harmonyReleaseTime = millis();
            harmonyRepeatCounter = 0;
            Serial.println("Button released...");
          } else { // New button press
            harmonyPressTime = millis();
            harmonyRepeatCounter = 1;
            if(commandId == harmonyCurrentCommand.id) { // Repeated press
              harmonyPressCounter++;
            } else { // New press
              harmonyPressCounter = 1;
              harmonyCurrentCommand = getHarmonyCommand(commandId);
              Serial.println("Button pressed...");
            }
          }
        }
        if(commandId == harmonyHold) { // Listen to repeated hold messages to registre longer presses
          harmonyHoldTime = millis();
        }
      }
  }

  // Logic to send message to HA for different command types
  unsigned long now = millis();
  if(harmonyPressCounter > 0) { // A button was pressed
    if(harmonyCurrentCommand.type <= 1) { // Click and and hold to repeat for type 1
      int publish = false;
      if(harmonyRepeatCounter == 1) { // First repeat is immediate
        publish = true;        
      }
      if(harmonyCurrentCommand.type == 1) {
        if(harmonyRepeatCounter == 2) { // Second press comes after some delay
          if(now > harmonyRepeatTime + SECOND_REPEAT_DURATION) {
            publish = true;
          }
        }
        if(harmonyRepeatCounter > 2) {// Second press comes after some delay
          if(now > harmonyRepeatTime + FURTHER_REPEAT_DURATION) {
            publish = true;
          }
        }
      }
      if(publish) {
        Serial.print("Publishing to Home Assistant: ");
        Serial.println(harmonyCurrentCommand.name);
        keyPress.setValue(harmonyCurrentCommand.name);
        harmonyRepeatCounter++;
        harmonyRepeatTime = now;
      }
    }
    if(harmonyCurrentCommand.type == 2) { // Complex button with click, double, multiple, long button
      if(((harmonyReleaseTime > harmonyPressTime) && (now > harmonyReleaseTime + WAIT_DURATION)) ||
          (now > harmonyPressTime + CLICK_DURATION)) { // The button was pressed and released, or held for long
        if(harmonyReleaseTime - harmonyPressTime < CLICK_DURATION) { // Click
          if(harmonyPressCounter == 1) {
            sprintf(mqttPayload,"%s_clicked",harmonyCurrentCommand.name);
          }
          if(harmonyPressCounter == 2) {
            sprintf(mqttPayload,"%s_double",harmonyCurrentCommand.name);
          }
          if(harmonyPressCounter > 2) {
            sprintf(mqttPayload,"%s_multiple",harmonyCurrentCommand.name);
          }
        } else { // Long press
          sprintf(mqttPayload,"%s_long",harmonyCurrentCommand.name);
        }
        Serial.print("Publishing to Home Assistant: ");
        Serial.println(mqttPayload);
        keyPress.setValue(mqttPayload);
        harmonyPressCounter = 0;
      } 
    }
  }
  // Harmony timeout logic to reset state in case of lost packets
  if((now > harmonyPressTime + WAIT_DURATION) &&
     (now > harmonyReleaseTime + WAIT_DURATION) &&
     (now > harmonyHoldTime + WAIT_DURATION)) {
     // Reset states
     harmonyPressCounter = 0;
     harmonyRepeatCounter = 0;
     harmonyCurrentCommand.id = 0;
  }  
}

harmonyCommandT
getHarmonyCommand(uint32_t id) {
  int i = 0;
  // Search regular commands
  while(harmonyCommandList[i].id != 0) {
    if(harmonyCommandList[i].id == id) return harmonyCommandList[i];
    i++;
  }
  // Return undefined (default) command
  sprintf(harmonyDefaultCommandName,"%.8X",id);
  harmonyDefaultCommand.id = id;
  return harmonyDefaultCommand;
}

void loop() {
  mqtt.loop();

  if (address) {
    regularRun();
  } else {
    initialSetup();
  }
  if ((millis() - shortLastUpdateAt) > 2000) { // update in 2s interval
    unsigned long seconds = millis() / 1000;
    int days = seconds / (24 * 3600);
    seconds = seconds % (24 * 3600);
    int hours = seconds / 3600;
    seconds = seconds % 3600;
    int minutes = seconds /  60;
    seconds = seconds % 60;
    if ( days > 3650 ) {
      sprintf(uptimeChar, "%ds", 0);
    } else if ( days ) {
      sprintf(uptimeChar, "%dd %dh %dm %ds", days,hours,minutes,seconds);
    } else if ( hours ) {
      sprintf(uptimeChar, "%dh %dm %ds", hours,minutes,seconds);
    } else if ( minutes ) {
      sprintf(uptimeChar, "%dm %ds", minutes,seconds);
    } else {
      sprintf(uptimeChar, "%ds", seconds);
    }
    upTime.setValue(uptimeChar);
    radioStatus.setState(radioActive);
    remoteAddress.setValue(addressChar);
    shortLastUpdateAt = millis();
  }
  if ((millis() - longLastUpdateAt) > 60000) { // update in 60s interval
    longLastUpdateAt = millis();
    macAddress.setValue(macChar);
    ipAddress.setValue(ipChar);
    if (!USE_WIRED){
      sprintf(rssiChar, "%d", GET_WIFI_RSSI);
    }
    wifiRssi.setValue(rssiChar);
  }
}
