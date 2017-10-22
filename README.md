# SONOFF_mod
Home brew SONOFF Switch code

## Goals
    - Use MQTT To control the switch
    - Use the Button to control the switch
    - To have a good working controlled power outlet.

## TODO
    - GPIO14 is use as additional external button to control the switch 

## Instructions : 
- In **"Define"** section you may adjust the debounce time
  ---> WARNING : EEPROM_STORE mode not yet tested, use your own responsibility
- In **"Global Variables"** section you shall set the your MQTT credentials 
- In **MQTT_SERVER_IP** -> you should adjust with your MQTT Server ip address
  ---> If you do not know the MQTT IP address you may needs to change the code. 
  
  For ex.:
  ------>const char* mqtt_server = "broker.mqtt-dashboard.com";
  ------>client.setServer(mqtt_server, 1883);
  
## Limitations:
This code can be used only the SONOFF switch devices.
 
## Used parts:
  - FTDI adapter
  - SONOFF device
  - Arduino IDE 1.8.4 or newer
