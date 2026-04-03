//  Home Assistant Harmony hub for ESP32 or similar
//  The purpose of the hub is to receive Harmony remote commands via nRF24L01+
//  and to format these and pass them on over MQTT to Home Assistant

// arduino_secrets.h MUST contain the following #define statements
// REMOTE_ADR - the Harmony remote address you got from the NetworkAddress sketch in format 0xF305984508
// BROKER_ADDR - the IP address of the MQTT broker in format IPAddress(127,0,0,1)

// arduino_secrets.h CAN contain the following #define statements to override defaults
// USE_WIRED - whether to use wired network config (default true)
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
// Harmony commands
// Type 0 : Only accept single clicks separated nu button releases (most responsive)
// Type 1 : Generated repeated clicks when button is held 
// Type 2 : Registers single clicks, double clicks, multiple clicks (three or more), or long presses
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

#define SOFTWARE_VERSION "2.0.0"
#define MANUFACTURER "pkscout"
#define MODEL "Harmony Companion OpenHub"
#define CONFIGURL "https://github.com/pkscout/Harmoino"

#include "arduino_secrets.h"
#include "defaults.h"
#if USE_WIRED
#include <ETH.h>
#define CONNECT ETH.begin(ETH_PHY_RTL8201, 0, 16, 17, -1, ETH_CLOCK_GPIO0_IN)
#define GETMAC ETH.macAddress(MAC)
#define NETWORKUP ETH.linkUp()
#define NETWORKCLIENT NetworkClient CLIENT
#else
#include <WiFi.h>
#define CONNECT WiFi.begin(SECRET_SSID, SECRET_PASS)
#define GETMAC WiFi.macAddress(MAC)
#define NETWORKUP WiFi.status() == WL_CONNECTED
#define NETWORKCLIENT WiFiClient CLIENT
#endif
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <ArduinoHA.h>

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

// Ethernet and Home Assistant mqtt clients
NETWORKCLIENT;
HADevice DEVICE;
HAMqtt MQTT(CLIENT, DEVICE);
unsigned long SHORT_LAST_UPDATE_AT = 0;
unsigned long LONG_LAST_UPDATE_AT = 0;
char mqtt_payload[50];
char UPTIME_CHAR[100];
char RADIO_STATUS_CHAR[100];
char MAC_CHAR[100];
bool FIRSTPRESS = true;
bool RADIOACTIVE = false;
byte MAC[6];
HASensor KEY_PRESS("key_press");
HASensor UPTIME("uptime");
HASensor RADIO_STATUS("radio_active");
HASensor MAC_ADDRESS("mac_address");

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
  setup_network();
  setup_nRF24(); 
  setup_homeAssistant();
}

void setup_network() {
  delay(10);
  Serial.println("");
  if (USE_WIRED) {
    CONNECT;
    if ( NETWORKUP ) {
      Serial.println("Connected to network");
    } else {
      Serial.println("NOT connected to network");
    }
  } else {
      Serial.print("Connecting to ");
      Serial.println(SECRET_SSID);
      CONNECT;
      while ( !(NETWORKUP) ) {
        delay(500);
        Serial.print(".");
      }
  }
  GETMAC;
  sprintf(MAC_CHAR, "%2X:%2X:%2X:%2X:%2X:%2X", MAC[0], MAC[1], MAC[2], MAC[3], MAC[4], MAC[5]);
  Serial.print("Mac Address: ");
  Serial.println(MAC_CHAR);
}

void setup_nRF24() {
  SPI.begin();
  RADIOACTIVE = radio.begin(&SPI);
  if( !RADIOACTIVE ) {
    Serial.println("nRF24L01+ Radio hardware not responding");
    sprintf(RADIO_STATUS_CHAR,"off");
  } else {
    Serial.println("nRF24L01+ Radio hardware started");
    RADIOACTIVE = true;
    sprintf(RADIO_STATUS_CHAR,"active");
    // nRF24L01+ radio settings (fixed to match Harmony remotes)
    radio.setChannel(RADIO_CH);
    radio.setDataRate(RF24_2MBPS);
    radio.enableDynamicPayloads();
    radio.setCRCLength (RF24_CRC_16);
    radio.openReadingPipe(1, REMOTE_ADR & 0xFFFFFFFF00);
    radio.openReadingPipe(2, REMOTE_ADR & 0xFFFFFFFFFF);
    radio.startListening();
    Serial.println("nRF24L01+ Radio hardware configured");
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
  RADIO_STATUS.setName("Radio Status");
  RADIO_STATUS.setIcon("mdi:radio-tower");
  RADIO_STATUS.setEntityCategory("diagnostic");
 // start MQTT connection
  Serial.print("Starting connection to MQTT broker at ");
  Serial.println(BROKER_ADDR);
  MQTT.begin(BROKER_ADDR, BROKER_PORT, BROKER_USER, BROKER_PASS);
 
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
    MAC_ADDRESS.setValue(MAC_CHAR);
    RADIO_STATUS.setValue(RADIO_STATUS_CHAR);
    SHORT_LAST_UPDATE_AT = millis();
  }

  if ((millis() - LONG_LAST_UPDATE_AT) > 60000) { // update in 60s interval
    LONG_LAST_UPDATE_AT = millis();
  }

}
