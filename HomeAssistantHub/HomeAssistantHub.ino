//  Home Assistant Harmony hub for ESP32 or similar (WiFi capable boards)
//  The purpose of the hub is to receive Harmony remote commands via nRF24L01+
//  and to format these and pass them on over mqtt to Home Assistant

// arduino_secrets.h needs to contain #define statements for
// SECRET_SSID - the SSID of the wifi network
// SECRET_PASS - the password of the wifi network
// BROKER_ADDR - the IP address of the broker in the format IPAddress(127,0,0,1)
// BROKER_USER - the MQTT username (you can put anything here if you're not using auth on MQTT)
// BROKER_PASS - the MQTT password (you can put anything here if you're not using auth on MQTT)
// DEVICE_NAME - the name of the remote

#define SOFTWARE_VERSION "1.0.0"
#define MANUFACTURER "pkscout"
#define MODEL "Harmony Companion OpenHub"
#define CONFIGURL "https://github.com/pkscout/Harmoino"

#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <ArduinoHA.h>
#include "arduino_secrets.h"

// nRF24L01+ SPI parameters
#define CE_PIN  14
#define CSN_PIN  8

// Harmony RF24 network and radio parameters
const uint64_t address = 0xF305984508; // Unique remote RF24 address. Use NetworkAddress script to find it
const uint8_t channel = 5; // Choose 5,8,14,17,32,35,41,44,62,65,71 or 74. Any one should work

// Settings for input functions (in ms)
const long harmony_click_duration = 500; // Maximal duration of a click, and minimal duration for a long press
const long harmony_wait_duration = 200; // Time to wait after a click to register additional clicks
const long harmony_second_repeat_duration = 1000; // Time to start repeating inputs for type 1 command
const long harmony_further_repeat_duration = 250; // Time between subsequent repeated inputs for type 1 commands

// Harmony commands
// Type 0 : Only accept single clicks separated nu button releases (most responsive)
// Type 1 : Generated repeated clicks when button is held 
// Type 2 : Registers single clicks, double clicks, multiple clicks (three or more), or long presses

typedef struct {
  uint32_t id;
  int type;
  char *name;
} harmony_command_t;

harmony_command_t harmony_command_list[] = 
 {{0x005800C1,0,"ok"},
  {0x005200C1,1,"up"},
  {0x05100C1,1,"down"},
  {0x005000C1,1,"left"},
  {0x004F00C1,1,"right"},
  {0x0000E9C3,1,"volume_up"},
  {0x0000EAC3,1,"volume_down"},
  {0x00009CC3,1,"channel_up"},
  {0x00009DC3,1,"channel_down"},
  {0x0000E2C3,0,"mute"},
  {0x000224C3,0,"return"},
  {0x000094C3,0,"exit"},
  {0x006500C1,0,"menu"},
  {0x00009AC3,0,"dvr"},
  {0x00008DC3,0,"guide"},
  {0x0001FFC3,0,"info"},
  {0x0001F7C3,0,"red"},
  {0x0001F6C3,0,"green"},
  {0x0001F5C3,0,"yellow"},
  {0x0001F4C3,0,"blue"},
  {0x0000B4C3,2,"backward"},
  {0x0000B3C3,2,"forward"},
  {0x0000B0C3,0,"play"},
  {0x0000B1C3,0,"pause"},
  {0x0000B7C3,0,"stop"},
  {0x0000B2C3,0,"rec"},
  {0x0001E8C3,2,"music"},
  {0x0001EDC3,2,"tv"},
  {0x0001E9C3,2,"movie"},
  {0x0001ECC3,2,"off"},
  {0x001E00C1,0,"number1"},
  {0x001F00C1,0,"number2"},
  {0x002000C1,0,"number3"},
  {0x002100C1,0,"number4"},
  {0x002200C1,0,"number5"},
  {0x002300C1,0,"number6"},
  {0x002400C1,0,"number7"},
  {0x002500C1,0,"number8"},
  {0x002600C1,0,"number9"},
  {0x002700C1,0,"number0"},
  {0x005600C1,0,"dotdot"},
  {0x002800C1,0,"dote"},
  {0x000FF2C3,0,"light1"},
  {0x000FF3C3,0,"light2"},
  {0x000FF4C3,0,"socket1"},
  {0x000FF5C3,0,"socket2"},
  {0x000FF0C3,0,"plus"},
  {0x000FF1C3,0,"minus"},
  {0,0,"null"}};

// End of user configurable parameters

// WiFi and Home Assistant mqtt clients
WiFiClient CLIENT;
HADevice DEVICE;
HAMqtt MQTT(CLIENT, DEVICE);
unsigned long SHORT_LAST_UPDATE_AT = 0;
unsigned long LONG_LAST_UPDATE_AT = 0;
char mqtt_payload[50];
char UPTIME_CHAR[40];
char MAC_CHAR[18];
HASensor KEY_PRESS("key_press");
HASensor UPTIME("uptime");
HASensor MAC_ADDRESS("mac_address");
HASensorNumber RSSI("rssi");
bool FIRSTPRESS = true;
bool FIRSTRUN = true;

// nRF24L01+ radio
RF24 radio(CE_PIN, CSN_PIN);

// nRF24L01+ buffers
char dataReceived[32]; // this must match dataToSend in the TX (need more than 10?)
uint8_t payloadSize;
bool newData = false;

// Harmont logic messages
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
  setup_nRF24();
  setup_wifi();
  setup_homeAssistant();
 
}

void setup_homeAssistant() {
 // setup HA device
  byte mac[6];
  WiFi.macAddress(mac);
  sprintf(MAC_CHAR, "%2X:%2X:%2X:%2X:%2X:%2X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.printf("Mac address: ");
  Serial.println(MAC_CHAR);
  DEVICE.setUniqueId(mac, sizeof(mac));
  DEVICE.setName(DEVICE_NAME);
  DEVICE.setSoftwareVersion(SOFTWARE_VERSION);
  DEVICE.setManufacturer(MANUFACTURER);
  DEVICE.setModel(MODEL);
  DEVICE.setConfigurationUrl(CONFIGURL);
  DEVICE.enableExtendedUniqueIds();
  DEVICE.enableLastWill();
  KEY_PRESS.setName("Key Press");
  KEY_PRESS.setIcon("mdi:button-pointer");
  KEY_PRESS.setForceUpdate(true);
  KEY_PRESS.setExpireAfter(5);
  KEY_PRESS.setAvailability(false);
  UPTIME.setName("Uptime");
  UPTIME.setEntityCategory("diagnostic");
  UPTIME.setIcon("mdi:clock-check-outline");
  UPTIME.setExpireAfter(10);
  MAC_ADDRESS.setName("MAC Address");
  MAC_ADDRESS.setIcon("mdi:ethernet");
  MAC_ADDRESS.setEntityCategory("diagnostic");
  RSSI.setName("WiFi Signal");
  RSSI.setIcon("mdi:wifi");
  RSSI.setUnitOfMeasurement("dBm");
  RSSI.setEntityCategory("diagnostic");
  RSSI.setForceUpdate(true);
  RSSI.setExpireAfter(70);

 // start MQTT connection
  Serial.print("Starting connection to MQTT broker at ");
  Serial.println(BROKER_ADDR);
  MQTT.begin(BROKER_ADDR, BROKER_PORT, BROKER_USER, BROKER_PASS);
 
}

void setup_nRF24() {
  SPI.begin();
  if(!radio.begin(&SPI)) {
  Serial.println("nRF24L01+ Radio hardware not responding");
    while(1); // Stop execution if nRF24L01+ hardware is not properly connected
  } else {
    Serial.println("nRF24L01+ Radio hardware started");
  }
  
  // nRF24L01+ radio settings (fixed to match Harmony remotes)
  radio.setChannel(channel);
  radio.setDataRate(RF24_2MBPS);
  radio.enableDynamicPayloads();
  radio.setCRCLength (RF24_CRC_16);
  radio.openReadingPipe(1, address & 0xFFFFFFFF00);
  radio.openReadingPipe(2, address & 0xFFFFFFFFFF);
  radio.startListening();
  Serial.println("nRF24L01+ Radio hardware configured");
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SECRET_SSID);

  WiFi.begin(SECRET_SSID, SECRET_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
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

  if (FIRSTRUN) {
    MAC_ADDRESS.setValue(MAC_CHAR);
    RSSI.setValue(WiFi.RSSI());
    FIRSTRUN = false;
  };

  uint8_t pipeNum;
  if ( radio.available(&pipeNum) ) {
      // Read packet
      uint8_t dataReceived[32];
      int payloadSize = radio.getDynamicPayloadSize();
      radio.read(&dataReceived,payloadSize);

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
          if(now > harmony_repeat_time + harmony_second_repeat_duration) {
            publish = true;
          }
        }
        if(harmony_repeat_counter > 2) {// Second press comes after some delay
          if(now > harmony_repeat_time + harmony_further_repeat_duration) {
            publish = true;
          }
        }
      }
      if(publish) {
        Serial.print("Publishing to Home Assistant: ");
        Serial.println(harmony_current_command.name);
        KEY_PRESS.setValue(harmony_current_command.name);
        if (FIRSTPRESS) {
          KEY_PRESS.setAvailability(true);
          FIRSTPRESS = false;
        };
        harmony_repeat_counter++;
        harmony_repeat_time = now;
      }
    }

    if(harmony_current_command.type == 2) { // Complex button with click, double, multiple, long button
      if(((harmony_release_time > harmony_press_time) && (now > harmony_release_time + harmony_wait_duration)) ||
          (now > harmony_press_time + harmony_click_duration)) { // The button was pressed and released, or held for long
        if(harmony_release_time - harmony_press_time < harmony_click_duration) { // Click
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
        if (FIRSTPRESS) {
          KEY_PRESS.setAvailability(true);
          FIRSTPRESS = false;
        };
        harmony_press_counter = 0;
      } 
    }

  }

  // Harmony timeout logic to reset state in case of lost packets
  if((now > harmony_press_time + harmony_wait_duration) &&
     (now > harmony_release_time + harmony_wait_duration) &&
     (now > harmony_hold_time + harmony_wait_duration)) {
     // Reset states
     harmony_press_counter = 0;
     harmony_repeat_counter = 0;
     harmony_current_command.id = 0;
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
    SHORT_LAST_UPDATE_AT = millis();
  }

  if ((millis() - LONG_LAST_UPDATE_AT) > 60000) { // update in 60s interval
    RSSI.setValue(WiFi.RSSI());
    LONG_LAST_UPDATE_AT = millis();
  }
}
