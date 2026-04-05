// network connection
#ifndef USE_WIRED
#define USE_WIRED false
#endif
#ifndef SECRET_SSID
#define SECRET_SSID ""
#endif
#ifndef SECRET_PASS
#define SECRET_PASS ""
#endif

// default PINS for the Ethernet Port
#ifndef PHY_ADR
#define PHY_ADR 0
#endif
#ifndef PHY_RESET
#define PHY_RESET -1
#endif
#ifndef SMI_MDC
#define SMI_MDC 16
#endif
#ifndef SMI_MDIO
#define SMI_MDIO 17
#endif
#ifndef ETH_PHY_SET
#define ETH_PHY_SET ETH_PHY_RTL8201
#endif
#ifndef ETH_GPIO_CLK_SET
#define ETH_GPIO_CLK_SET ETH_CLOCK_GPIO0_IN
#endif


// nRF24L01+ parameters
#ifndef REMOTE_ADR
#define REMOTE_ADR 0
#endif
#ifndef CE_PIN
#define CE_PIN 14
#endif
#ifndef CSN_PIN
#define CSN_PIN 8
#endif
#ifndef RADIO_CH
#define RADIO_CH 5
#endif

// MQTT parameters
#ifndef BROKER_PORT
#define BROKER_PORT 1883
#endif
#ifndef BROKER_USER
#define BROKER_USER "mqtt user"
#endif
#ifndef BROKER_PASS
#define BROKER_PASS "mqtt password"
#endif

// Harmony parameters
#ifndef DEVICE_NAME
#define DEVICE_NAME "Harmoino OpenHub"
#endif
#ifndef CLICK_DURATION
#define CLICK_DURATION 500
#endif
#ifndef WAIT_DURATION
#define WAIT_DURATION 200
#endif
#ifndef SECOND_REPEAT_DURATION
#define SECOND_REPEAT_DURATION 1000
#endif
#ifndef FURTHER_REPEAT_DURATION
#define FURTHER_REPEAT_DURATION 250
#endif
#ifndef CMD_OK
#define CMD_OK 0
#endif
#ifndef CMD_UP
#define CMD_UP 1
#endif
#ifndef CMD_DOWN
#define CMD_DOWN 1
#endif
#ifndef CMD_LEFT
#define CMD_LEFT 1
#endif
#ifndef CMD_RIGHT
#define CMD_RIGHT 1
#endif
#ifndef CMD_VOL_UP
#define CMD_VOL_UP 1
#endif
#ifndef CMD_VOL_DOWN
#define CMD_VOL_DOWN 1
#endif
#ifndef CMD_CH_UP
#define CMD_CH_UP 1
#endif
#ifndef CMD_CH_DOWN
#define CMD_CH_DOWN 1
#endif
#ifndef CMD_MUTE
#define CMD_MUTE 0
#endif
#ifndef CMD_RETURN
#define CMD_RETURN 0
#endif
#ifndef CMD_EXIT
#define CMD_EXIT 0
#endif
#ifndef CMD_MENU
#define CMD_MENU 0
#endif
#ifndef CMD_DVR
#define CMD_DVR 0
#endif
#ifndef CMD_GUIDE
#define CMD_GUIDE 0
#endif
#ifndef CMD_INFO
#define CMD_INFO 0
#endif
#ifndef CMD_RED
#define CMD_RED 0
#endif
#ifndef CMD_GREEN
#define CMD_GREEN 0
#endif
#ifndef CMD_YELLOW
#define CMD_YELLOW 0
#endif
#ifndef CMD_BLUE
#define CMD_BLUE 0
#endif
#ifndef CMD_BACKWARD
#define CMD_BACKWARD 1
#endif
#ifndef CMD_FORWARD
#define CMD_FORWARD 1
#endif
#ifndef CMD_PLAY
#define CMD_PLAY 0
#endif
#ifndef CMD_PAUSE
#define CMD_PAUSE 0
#endif
#ifndef CMD_STOP
#define CMD_STOP 0
#endif
#ifndef CMD_REC
#define CMD_REC 0
#endif
#ifndef CMD_MUSIC
#define CMD_MUSIC 2
#endif
#ifndef CMD_TV
#define CMD_TV 2
#endif
#ifndef CMD_MOVIE
#define CMD_MOVIE 2
#endif
#ifndef CMD_OFF
#define CMD_OFF 0
#endif
#ifndef CMD_NUM0
#define CMD_NUM0 0
#endif
#ifndef CMD_NUM1
#define CMD_NUM1 0
#endif
#ifndef CMD_NUM2
#define CMD_NUM2 0
#endif
#ifndef CMD_NUM3
#define CMD_NUM3 0
#endif
#ifndef CMD_NUM4
#define CMD_NUM4 0
#endif
#ifndef CMD_NUM5
#define CMD_NUM5 0
#endif
#ifndef CMD_NUM6
#define CMD_NUM6 0
#endif
#ifndef CMD_NUM7
#define CMD_NUM7 0
#endif
#ifndef CMD_NUM8
#define CMD_NUM8 0
#endif
#ifndef CMD_NUM9
#define CMD_NUM9 0
#endif
#ifndef CMD_DOTDOT
#define CMD_DOTDOT 0
#endif
#ifndef CMD_DOTE
#define CMD_DOTE 0
#endif
#ifndef CMD_LIGHT1
#define CMD_LIGHT1 0
#endif
#ifndef CMD_LIGHT2
#define CMD_LIGHT2 0
#endif
#ifndef CMD_SOCKET1
#define CMD_SOCKET1 0
#endif
#ifndef CMD_SOCKET2
#define CMD_SOCKET2 0
#endif
#ifndef CMD_PLUS
#define CMD_PLUS 0
#endif
#ifndef CMD_MINUS
#define CMD_MINUS 0
#endif
