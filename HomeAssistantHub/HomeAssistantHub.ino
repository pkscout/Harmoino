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
// CE - the pin number or name for the CE connection to the radio (default 2)
// CSN - the pin number or name for the CSN connection to the radio (default 4)
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
// OK - key press (default 0)
// UP - key press (default 1)
// DOWN - key press (default 1)
// LEFT - key press (default 1)
// RIGHT - key press (default 1)
// VOL_UP - key press (default 1)
// VOL_DOWN - key press (default 1)
// CH_UP - key press (default 1)
// CH_DOWN - key press (default 1)
// MUTE - key press (default 0)
// RETURN - key press (default 0)
// EXIT - key press (default 0)
// MENU - key press (default 0)
// DVR - key press (default 0)
// GUIDE - key press (default 0)
// INFO - key press (default 0)
// RED - key press (default 0)
// GREEN - key press (default 0)
// YELLOW - key press (default 0)
// BLUE - key press (default 0)
// BACKWARD - key press (default 1)
// FORWARD - key press (default 1)
// PLAY - key press (default 0)
// PAUSE - key press (default 0)
// STOP - key press (default 0)
// REC - key press (default 0)
// MUSIC - key press (default 2)
// TV - key press (default 2)
// MOVIE - key press (default 2)
// OFF - key press (default 0)
// NUM0 - key press (default 0)
// NUM1 - key press (default 0)
// NUM2 - key press (default 0)
// NUM3 - key press (default 0)
// NUM4 - key press (default 0)
// NUM5 - key press (default 0)
// NUM6 - key press (default 0)
// NUM7 - key press (default 0)
// NUM8 - key press (default 0)
// NUM9 - key press (default 0)
// DOTDOT - key press (default 0)
// DOTE - key press (default 0)
// LIGHT1 - key press (default 0)
// LIGHT2 - key press (default 0)
// SOCKET1 - key press (default 0)
// SOCKET2 - key press (default 0)
// PLUS - key press (default 0)
// MINUS - key press (default 0)

#define SOFTWARE_VERSION "2.3.0"
#define MANUFACTURER "pkscout"
#define MODEL "Harmony Companion OpenHub"
#define CONFIGURL "https://github.com/pkscout/Harmoino"

#include "arduino_secrets.h"
#include "defaults.h"
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <ArduinoHA.h>
#include <Preferences.h>
#if USE_WIRED
#include <ETH.h>
#define CONNECT ETH.begin(ETH_PHY_RTL8201, 0, 16, 17, -1, ETH_CLOCK_GPIO0_IN)
#define GET_MAC ETH.macAddress(MAC)
#define NETWORKDOWN ETH.localIP() == IPAddress(0, 0, 0, 0)
#define NETWORKCLIENT NetworkClient CLIENT
#define GET_LOCALIP ETH.localIP()
#define GET_WIFI_RSSI ETH.speed()
#else
#include <WiFi.h>
#define CONNECT WiFi.begin(SECRET_SSID, SECRET_PASS)
#define GET_MAC WiFi.macAddress(MAC)
#define NETWORKDOWN WiFi.status() != WL_CONNECTED
#define NETWORKCLIENT WiFiClient CLIENT
#define GET_LOCALIP WiFi.localIP()
#define GET_WIFI_RSSI WiFi.RSSI()
#endif

Preferences prefs;
uint64_t ADDRESS = 0;
char ADDRESS_CHAR[50];

typedef struct {
  uint32_t id;
  int type;
  char *name;
} harmony_command_t;

harmony_command_t harmony_command_list[] = 
 {{0x005800C1,OK,"ok"},
  {0x005200C1,UP,"up"},
  {0x05100C1,DOWN,"down"},
  {0x005000C1,LEFT,"left"},
  {0x004F00C1,RIGHT,"right"},
  {0x0000E9C3,VOL_UP,"volume_up"},
  {0x0000EAC3,VOL_DOWN,"volume_down"},
  {0x00009CC3,CH_UP,"channel_up"},
  {0x00009DC3,CH_DOWN,"channel_down"},
  {0x0000E2C3,MUTE,"mute"},
  {0x000224C3,RETURN,"return"},
  {0x000094C3,EXIT,"exit"},
  {0x006500C1,MENU,"menu"},
  {0x00009AC3,DVR,"dvr"},
  {0x00008DC3,GUIDE,"guide"},
  {0x0001FFC3,INFO,"info"},
  {0x0001F7C3,RED,"red"},
  {0x0001F6C3,GREEN,"green"},
  {0x0001F5C3,YELLOW,"yellow"},
  {0x0001F4C3,BLUE,"blue"},
  {0x0000B4C3,BACKWARD,"backward"},
  {0x0000B3C3,FORWARD,"forward"},
  {0x0000B0C3,PLAY,"play"},
  {0x0000B1C3,PAUSE,"pause"},
  {0x0000B7C3,STOP,"stop"},
  {0x0000B2C3,REC,"rec"},
  {0x0001E8C3,MUSIC,"music"},
  {0x0001EDC3,TV,"tv"},
  {0x0001E9C3,MOVIE,"movie"},
  {0x0001ECC3,OFF,"off"},
  {0x001E00C1,NUM1,"number1"},
  {0x001F00C1,NUM2,"number2"},
  {0x002000C1,NUM3,"number3"},
  {0x002100C1,NUM4,"number4"},
  {0x002200C1,NUM5,"number5"},
  {0x002300C1,NUM6,"number6"},
  {0x002400C1,NUM7,"number7"},
  {0x002500C1,NUM8,"number8"},
  {0x002600C1,NUM9,"number9"},
  {0x002700C1,NUM0,"number0"},
  {0x005600C1,DOTDOT,"dotdot"},
  {0x002800C1,DOTE,"dote"},
  {0x000FF2C3,LIGHT1,"light1"},
  {0x000FF3C3,LIGHT2,"light2"},
  {0x000FF4C3,SOCKET1,"socket1"},
  {0x000FF5C3,SOCKET2,"socket2"},
  {0x000FF0C3,PLUS,"plus"},
  {0x000FF1C3,MINUS,"minus"},
  {0,0,"null"}};

// Network and Home Assistant mqtt clients
NETWORKCLIENT;
HADevice DEVICE;
HAMqtt MQTT(CLIENT, DEVICE);
unsigned long SHORT_LAST_UPDATE_AT = 0;
unsigned long LONG_LAST_UPDATE_AT = 0;
char mqtt_payload[50];
char UPTIME_CHAR[50];
char MAC_CHAR[50];
char IP_CHAR[20];
char RSSI_CHAR[50];
bool FIRSTPRESS = true;
bool RADIOACTIVE = false;
bool GOT_ADDRESS = false;
byte MAC[6];
HASensor KEY_PRESS("key_press");
HASensor UPTIME("uptime");
HASensor MAC_ADDRESS("mac_address");
HASensor IP_ADDRESS("ip_address");
HASensor WIFI_RSSI("wifi_rssi");
HASensor REMOTE_ADDRESS("remote_address");
HABinarySensor RADIO_STATUS("radio_status");
HAButton REBOOT_DEVICE("reboot_device");
HAButton RESET_DEVICE("reset_device");
HAButton MIGRATE_ADDRESS("migrate_address");

// nRF24L01+ radio
RF24 radio(CE_PIN, CSN_PIN);

// nRF24L01+ buffers
char dataReceived[32]; // this must match dataToSend in the TX (need more than 10?)
uint8_t payloadSize;
bool newData = false;

// Harmony RF24 network and radio parameters
const uint64_t pair_address = 0xBB0ADCA575; // Common pairing RF24 address
const uint8_t channels[12] = {5,8,14,17,32,35,41,44,62,65,71,74}; // Possible RF24 channels
int channelId = 0;

// Messages to send to Harmony Hub to get regular remote RF24 address
const byte pairMessage[22] = {242,95,1,225,154,157,218,83,40,64,30,4,2,7,12,0,0,0,0,0,102,100};
const byte pingMessage[5] = {242,64,1,225,236};
int pingRetries = 0;

// Harmony logic messages
const uint32_t harmony_hold = 0x98280040;
const uint32_t harmony_ping = 0x704C0440;
const uint32_t harmony_sleep = 0x0000034F;

char harmony_default_command_name[9];
harmony_command_t harmony_default_command = {0,0,harmony_default_command_name};

// Harmony state logic
unsigned long harmony_press_time = 0;
unsigned long harmony_release_time = 0;
unsigned long harmony_hold_time = 0;
unsigned long harmony_repeat_time = 0;
unsigned int harmony_press_counter = 0;
unsigned int harmony_repeat_counter = 0;
harmony_command_t harmony_current_command = harmony_default_command;

void setup() {
  // Setup communication protocols
  Serial.begin(115200);
  delay(10000);
  Serial.println("----Starting Device----");
  setup_preferences();
  setup_network();
  setup_nRF24(); 
  setup_homeAssistant();
}

void setup_preferences() {
  prefs.begin("harmonio", false);
  ADDRESS = prefs.getULong64("remote_address", 0);
  if (ADDRESS) {
    sprintf(ADDRESS_CHAR, "0x%llX", ADDRESS);
    Serial.print("The remote address is ");
    Serial.println(ADDRESS_CHAR);
  } else {
    sprintf(ADDRESS_CHAR, "none");
    Serial.println("No remote address, triggering initial setup");
  }
}

void setup_network() {
  if (USE_WIRED) {
    Serial.println("Connecting to network");
  } else {
    Serial.print("Connecting to ");
    Serial.print(SECRET_SSID);
  }
  CONNECT;
  while ( NETWORKDOWN ) {
    delay(500);
    Serial.print(".");
  }
  GET_MAC;
  sprintf(MAC_CHAR, "%02X:%02X:%02X:%02X:%02X:%02X", MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);
  Serial.println(""),
  Serial.print("Mac Address: ");
  Serial.println(MAC_CHAR);
  IPAddress ip = GET_LOCALIP;
  snprintf(IP_CHAR, sizeof(IP_CHAR), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  Serial.print("IP address: ");
  Serial.println(IP_CHAR);
  sprintf(RSSI_CHAR, "%d", GET_WIFI_RSSI);
  if (USE_WIRED){
    Serial.print("Link speed: ");
    Serial.print(RSSI_CHAR);
    Serial.println(" Mb/s");
  } else {
    Serial.print("RSSI: ");
    Serial.print(RSSI_CHAR);
    Serial.println(" dBm");
  }
  
}

void setup_nRF24() {
  SPI.begin();
  RADIOACTIVE = radio.begin(&SPI);
  if( !RADIOACTIVE ) {
    Serial.println("nRF24L01+ Radio hardware not responding");
  } else {
    Serial.println("nRF24L01+ Radio hardware started");
    RADIOACTIVE = true;
    // nRF24L01+ radio settings (fixed to match Harmony remotes)
    if (ADDRESS){
      radio.setChannel(RADIO_CH);
      radio.setDataRate(RF24_2MBPS);
      radio.enableDynamicPayloads();
      radio.setCRCLength (RF24_CRC_16);
      radio.openReadingPipe(1, ADDRESS & 0xFFFFFFFF00);
      radio.openReadingPipe(2, ADDRESS & 0xFFFFFFFFFF);
      radio.startListening();
      Serial.println("nRF24L01+ Radio hardware configured");
    } else {
      radio.setDataRate(RF24_2MBPS);
      radio.enableDynamicPayloads();
      radio.enableAckPayload();
      radio.setCRCLength (RF24_CRC_16);
      radio.openWritingPipe(pair_address);
      Serial.println("Power on the Harmony Smart Hub and press the pair/reset button");
    }
  }
}

void setup_homeAssistant() {
 // setup HA device
  DEVICE.setUniqueId(MAC, sizeof(MAC));
  DEVICE.setName(DEVICE_NAME);
  DEVICE.setSoftwareVersion(SOFTWARE_VERSION);
  DEVICE.setManufacturer(MANUFACTURER);
  DEVICE.setModel(MODEL);
  DEVICE.setConfigurationUrl(CONFIGURL);
  DEVICE.enableExtendedUniqueIds();
  DEVICE.enableSharedAvailability();
  DEVICE.enableLastWill();
  KEY_PRESS.setName("Key Press");
  KEY_PRESS.setIcon("mdi:button-pointer");
  KEY_PRESS.setForceUpdate(true);
  KEY_PRESS.setExpireAfter(2);
  UPTIME.setName("Uptime");
  UPTIME.setEntityCategory("diagnostic");
  UPTIME.setIcon("mdi:clock-check-outline");
  UPTIME.setExpireAfter(40);
  MAC_ADDRESS.setName("MAC Address");
  MAC_ADDRESS.setIcon("mdi:ethernet");
  MAC_ADDRESS.setEntityCategory("diagnostic");
  IP_ADDRESS.setName("IP Address");
  IP_ADDRESS.setIcon("mdi:network-outline");
  IP_ADDRESS.setEntityCategory("diagnostic");
  if (USE_WIRED) {
    WIFI_RSSI.setName("Link Speed");
    WIFI_RSSI.setIcon("mdi:speedometer");
    WIFI_RSSI.setUnitOfMeasurement("Mb/s");
  } else {
    WIFI_RSSI.setName("RSSI");
    WIFI_RSSI.setIcon("mdi:wifi");
    WIFI_RSSI.setUnitOfMeasurement("dBm");
  }
  WIFI_RSSI.setEntityCategory("diagnostic");
  RADIO_STATUS.setName("Radio Status");
  RADIO_STATUS.setIcon("mdi:radio-tower");
  RADIO_STATUS.setEntityCategory("diagnostic");
  REMOTE_ADDRESS.setName("Remote Address");
  REMOTE_ADDRESS.setIcon("mdi:remote");
  REMOTE_ADDRESS.setEntityCategory("diagnostic");
  REBOOT_DEVICE.setName("Reboot");
  REBOOT_DEVICE.setIcon("mdi:restart");
  REBOOT_DEVICE.setEntityCategory("config");
  REBOOT_DEVICE.onCommand(onButtonCommand);
  RESET_DEVICE.setName("Reset");
  RESET_DEVICE.setIcon("mdi:refresh");
  RESET_DEVICE.setEntityCategory("config");
  RESET_DEVICE.onCommand(onButtonCommand);
  MIGRATE_ADDRESS.setName("Migrate");
  MIGRATE_ADDRESS.setIcon("mdi:file-move-outline");
  MIGRATE_ADDRESS.setEntityCategory("config");
  MIGRATE_ADDRESS.onCommand(onButtonCommand);
  // start MQTT connection
  Serial.print("Starting connection to MQTT broker at ");
  Serial.println(BROKER_ADDR);
  MQTT.begin(BROKER_ADDR, BROKER_PORT, BROKER_USER, BROKER_PASS);
}

void onButtonCommand(HAButton* sender) {
  if (sender == &REBOOT_DEVICE) {
    Serial.println("rebooting device");
    ESP.restart();
  } else if (sender == &RESET_DEVICE) {
    prefs.remove("remote_address");
    Serial.print("resetting device");
    while (prefs.getULong64("remote_address", 0)) {
      Serial.print(".");
      delay(100);
    }
    Serial.println("");
    Serial.println("rebooting device");
    ESP.restart();
  } else if (sender == &MIGRATE_ADDRESS){
    if (REMOTE_ADR){
      Serial.println("migrating the remote address from ardunio_secrets.h to flash");
      sprintf(ADDRESS_CHAR, "0x%llX", REMOTE_ADR);
      save_preference();
      restart_device();
    } else {
      Serial.println("no remote address in ardunio_secrets.h to migrate");
    }
  }
}

void save_preference() {
  Serial.print("The remote RF24 address is: ");
  Serial.println(ADDRESS_CHAR);
  REMOTE_ADDRESS.setValue(ADDRESS_CHAR);
  prefs.putULong64("remote_address", strtoull(ADDRESS_CHAR, nullptr, 16));
  Serial.print("saving preference");
  while (!prefs.getULong64("remote_address", 0)) {
    Serial.print(".");
    delay(100);
  }
  Serial.println("");
}

void restart_device() {
  Serial.println("rebooting device");
  ESP.restart();
}

void initial_setup() {
  REMOTE_ADDRESS.setValue(ADDRESS_CHAR);
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
          sprintf(ADDRESS_CHAR, "0x");
          for(int i=3;i<8;i++) {
              char tmp[3];
              if(i < 7) sprintf(tmp,"%.2X",dataReceived[i]);
              else sprintf(tmp,"%.2X",dataReceived[i]-1);
              strcat(ADDRESS_CHAR, tmp);
          }
          Serial.println("");
          save_preference();
          restart_device();
      }      
  }
}

void regular_run() {
  uint8_t pipeNum;
  if ( radio.available(&pipeNum) ) {
      // Read packet
      uint8_t dataReceived[32];
      int payloadSize = radio.getDynamicPayloadSize();
      radio.read(&dataReceived,payloadSize);
      if (pipeNum > 0){
        RADIOACTIVE = true;
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
      } else if (RADIOACTIVE) {
        Serial.println("Radio has gone offline. Check connections, then reboot");
        RADIOACTIVE = false;
      }

      // Interpret data from Harmony remote
      if(payloadSize >= 5) { // Should always be true but just to make sure
        uint32_t command_id = 0;
        for(int i=5; i > 0; i--) {
          command_id <<= 8;
          command_id += (uint32_t) dataReceived[i];
        }

        // Harmony state logic
        if((command_id & 0x000000F0) == 0x000000C0) { // Button id
          if(command_id == (harmony_current_command.id & 0x000000FF)) { // Button released
            harmony_release_time = millis();
            harmony_repeat_counter = 0;
            Serial.println("Button released...");
          } else { // New button press
            harmony_press_time = millis();
            harmony_repeat_counter = 1;
            if(command_id == harmony_current_command.id) { // Repeated press
              harmony_press_counter++;
            } else { // New press
              harmony_press_counter = 1;
              harmony_current_command = get_harmony_command(command_id);
              Serial.println("Button pressed...");
            }
          }
        }
        if(command_id == harmony_hold) { // Listen to repeated hold messages to registre longer presses
          harmony_hold_time = millis();
        }
      }
  }

  // Logic to send message to HA for different command types
  unsigned long now = millis();
  if(harmony_press_counter > 0) { // A button was pressed
    if(harmony_current_command.type <= 1) { // Click and and hold to repeat for type 1
      int publish = false;
      if(harmony_repeat_counter == 1) { // First repeat is immediate
        publish = true;        
      }
      if(harmony_current_command.type == 1) {
        if(harmony_repeat_counter == 2) { // Second press comes after some delay
          if(now > harmony_repeat_time + SECOND_REPEAT_DURATION) {
            publish = true;
          }
        }
        if(harmony_repeat_counter > 2) {// Second press comes after some delay
          if(now > harmony_repeat_time + FURTHER_REPEAT_DURATION) {
            publish = true;
          }
        }
      }
      if(publish) {
        Serial.print("Publishing to Home Assistant: ");
        Serial.println(harmony_current_command.name);
        KEY_PRESS.setValue(harmony_current_command.name);
        harmony_repeat_counter++;
        harmony_repeat_time = now;
      }
    }
    if(harmony_current_command.type == 2) { // Complex button with click, double, multiple, long button
      if(((harmony_release_time > harmony_press_time) && (now > harmony_release_time + WAIT_DURATION)) ||
          (now > harmony_press_time + CLICK_DURATION)) { // The button was pressed and released, or held for long
        if(harmony_release_time - harmony_press_time < CLICK_DURATION) { // Click
          if(harmony_press_counter == 1) {
            sprintf(mqtt_payload,"%s_clicked",harmony_current_command.name);
          }
          if(harmony_press_counter == 2) {
            sprintf(mqtt_payload,"%s_double",harmony_current_command.name);
          }
          if(harmony_press_counter > 2) {
            sprintf(mqtt_payload,"%s_multiple",harmony_current_command.name);
          }
        } else { // Long press
          sprintf(mqtt_payload,"%s_long",harmony_current_command.name);
        }
        Serial.print("Publishing to Home Assistant: ");
        Serial.println(mqtt_payload);
        KEY_PRESS.setValue(mqtt_payload);
        harmony_press_counter = 0;
      } 
    }
  }
  // Harmony timeout logic to reset state in case of lost packets
  if((now > harmony_press_time + WAIT_DURATION) &&
     (now > harmony_release_time + WAIT_DURATION) &&
     (now > harmony_hold_time + WAIT_DURATION)) {
     // Reset states
     harmony_press_counter = 0;
     harmony_repeat_counter = 0;
     harmony_current_command.id = 0;
  }  
}

harmony_command_t
get_harmony_command(uint32_t id) {
  int i = 0;
  // Search regular commands
  while(harmony_command_list[i].id != 0) {
    if(harmony_command_list[i].id == id) return harmony_command_list[i];
    i++;
  }
  // Return undefined (default) command
  sprintf(harmony_default_command_name,"%.8X",id);
  harmony_default_command.id = id;
  return harmony_default_command;
}

void loop() {
  MQTT.loop();

  if (ADDRESS) {
    regular_run();
  } else if (!GOT_ADDRESS) {
    initial_setup();
  }

  if ((millis() - SHORT_LAST_UPDATE_AT) > 2000) { // update in 2s interval
    unsigned long seconds = millis() / 1000;
    int days = seconds / (24 * 3600);
    seconds = seconds % (24 * 3600);
    int hours = seconds / 3600;
    seconds = seconds % 3600;
    int minutes = seconds /  60;
    seconds = seconds % 60;
    if ( days > 3650 ) {
      sprintf(UPTIME_CHAR, "%ds", 0);
    } else if ( days ) {
      sprintf(UPTIME_CHAR, "%dd %dh %dm %ds", days,hours,minutes,seconds);
    } else if ( hours ) {
      sprintf(UPTIME_CHAR, "%dh %dm %ds", hours,minutes,seconds);
    } else if ( minutes ) {
      sprintf(UPTIME_CHAR, "%dm %ds", minutes,seconds);
    } else {
      sprintf(UPTIME_CHAR, "%ds", seconds);
    }
    UPTIME.setValue(UPTIME_CHAR);
    RADIO_STATUS.setState(RADIOACTIVE);
    REMOTE_ADDRESS.setValue(ADDRESS_CHAR);
    SHORT_LAST_UPDATE_AT = millis();
  }

  if ((millis() - LONG_LAST_UPDATE_AT) > 60000) { // update in 60s interval
    LONG_LAST_UPDATE_AT = millis();
    MAC_ADDRESS.setValue(MAC_CHAR);
    IP_ADDRESS.setValue(IP_CHAR);
    if (!USE_WIRED){
      sprintf(RSSI_CHAR, "%d", GET_WIFI_RSSI);
    }
    WIFI_RSSI.setValue(RSSI_CHAR);
  }

}
