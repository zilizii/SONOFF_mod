#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <EEPROM.h>
//
// Define
//
#define WIFI_LED 13
#define BUTTON_GPIO_1 0
#define FREETOUSEGPIO 14
#define RELAY 12
#define MQTT_VERSION    MQTT_VERSION_3_1_1
#define DEBOUNCETIME 50
#define EEPROM_STORE false
#define fDebug true

//
//Global Variables
//
const PROGMEM uint16_t  MQTT_SERVER_PORT          = 1883;
const PROGMEM char*     MQTT_CLIENT_ID            = "SONOFF_Switch_1";
const PROGMEM char*     MQTT_USER                 = "mqtt_user";
const PROGMEM char*     MQTT_PASSWORD             = "mqtt_psw";
const char*             MQTT_STATE_TOPIC          = "SONOFF_Switch_1/switch/status";
const char*             MQTT_COMMAND_TOPIC        = "SONOFF_Switch_1/switch/switch";
const char*             STATE_ON                  = "ON";
const char*             STATE_OFF                 = "OFF";

WiFiClient espClient;
PubSubClient client(espClient);

//Update with your MQTT IP address
IPAddress MQTT_SERVER_IP(192, 168, X, Y);

volatile bool stateOfRelay;
volatile bool lastStateOfRelay;
volatile unsigned long lastDebounceMillis;
volatile unsigned long lastMillis;

void vreconnect() {
  //
  // Loop until we're reconnected
  //
  while (!client.connected()) {
    yield();
    if (fDebug) {
      Serial.print("INFO: Attempting MQTT connection...");
    }
    //
    // Attempt to connect
    //
    if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      if (fDebug) {
        Serial.println("INFO: connected");
      }
      //
      // Once connected, publish an announcement...
      //
      vPublishRelayState();
      //
      // ... and resubscribe
      //
      client.subscribe(MQTT_COMMAND_TOPIC);
    } else {
      if (fDebug) {
        Serial.print("ERROR: failed, rc=");
        Serial.print(client.state());
        Serial.println("DEBUG: try again in 5 seconds");
      }
      //
      // Wait 5 seconds before retrying  --> TODO without Wait!!!!
      //
       for(int i = 0; i<500; i++) {
        vRelayHandler();
        delay(10);
      }
    }
  }
}

void buttonInterrupt() {
  //TODO : bounce, bounce, debounce
  if ( (millis() - lastDebounceMillis) > DEBOUNCETIME)
  {
    lastDebounceMillis = millis();
    stateOfRelay = !stateOfRelay;
    if (fDebug) {
      Serial.print("INFO: ISR From Button...");
    }
  }
}

// function called when a MQTT message arrived
void callback(char* p_topic, byte* p_payload, unsigned int p_length) {
  // concat the payload into a string
  if (fDebug){
    Serial.println("Callback...");
  }
  String payload;
  for (uint8_t i = 0; i < p_length; i++) {
    payload.concat((char)p_payload[i]);
  }
  if (fDebug){
    Serial.print("Payload: ");
    Serial.println(payload);
  }
  // handle message topic
  if (String(MQTT_COMMAND_TOPIC).equals(p_topic)) {
    // test if the payload is equal to "ON" or "OFF"
    if (payload.equals(String(STATE_ON))) {
      if (stateOfRelay != true) {
        stateOfRelay = true;
        vRelayHandler();
      }
    } else if (payload.equals(String(STATE_OFF))) {
        if (fDebug){
          Serial.println("Light Off");
          Serial.println("Light turned off");
        }
        stateOfRelay = false;
        vRelayHandler();
    }
  }
}

void vRelayHandler() {
  //
  // In case of changes in the relay state
  //
  if (stateOfRelay != lastStateOfRelay) {
    lastStateOfRelay = stateOfRelay;
    digitalWrite(RELAY, stateOfRelay);
    digitalWrite(WIFI_LED, !stateOfRelay);
    if (EEPROM_STORE) {
      //
      // Write state to EEPROM
      //      
      EEPROM.write(0, stateOfRelay);    
      EEPROM.commit();
    }
    vPublishRelayState();
  }
  else {
    //
    // No state change part ->> Main Loop Handles the 5 [sec] updates
    //
  }

  yield();
}

void vPublishRelayState() {
  if(client.connected()) {
    lastMillis = millis();
    client.publish(MQTT_STATE_TOPIC, stateOfRelay ? STATE_ON : STATE_OFF , true);
  }
}


void setup() {
  if (fDebug) {
    Serial.begin(115200);
  }
  delay(500);
  //
  // EEPROM to store on/off state -> Safety
  //
  EEPROM.begin(512);
  //
  // Set the IO Pins
  //
  pinMode(WIFI_LED, OUTPUT); // status LED
  pinMode(RELAY, OUTPUT);    // Actuator Relay
  pinMode(BUTTON_GPIO_1, INPUT);

  attachInterrupt(BUTTON_GPIO_1, buttonInterrupt, FALLING);
  yield();
  //
  // Setting the internal variables
  //
  lastMillis         = millis();
  lastDebounceMillis = lastMillis;
  stateOfRelay       = LOW;
  lastStateOfRelay   = LOW;
  //
  // Differene logic in the beggining in case of your setup
  // TODO :  make this feature changeable from MQTT
  //
  if (EEPROM_STORE) {
    stateOfRelay = EEPROM.read(0);
  } else {
    stateOfRelay = LOW;
  }

  digitalWrite(WIFI_LED, !stateOfRelay);
  lastStateOfRelay = stateOfRelay;
  digitalWrite(RELAY, stateOfRelay);

  WiFiManager wifiManager;
  wifi_station_set_hostname("sonoffSwitch_1");

  if(fDebug) {
    wifiManager.setDebugOutput(true);
  }
 

 if(!wifiManager.autoConnect("SONOFF_1_ADMIN")) {
  delay(3000);
  ESP.reset();
 }else{

  
  client.setServer(MQTT_SERVER_IP , MQTT_SERVER_PORT);
  client.setCallback(callback);
  delay(1000);
 }
}

void loop() {
  // put your main code here, to run repeatedly:
  vRelayHandler();
  if (!client.connected()) {
    if (fDebug) {
      Serial.println("Reconnect");
    }
    vreconnect();
  } else {
    
    if ((millis() - lastMillis) > 60000) {
      vPublishRelayState();
    }
    delay(64);
    client.loop();
  }
}
