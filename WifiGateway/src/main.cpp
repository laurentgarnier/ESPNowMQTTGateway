#include <Arduino.h>
#include "credentials.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
// #include <EEPROM.h>
#include <ArduinoJson.h>

//#define DEBUG_FLAG 

#ifdef DEBUG_FLAG
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debug(x)
#define debugln(x)
#endif

/* --------------------------- Wifi/MQTT ---------------------------------------------- */

WiFiClient    wifiClient;
#define       MQTT_MSG_SIZE    200
char          mqttTopic[MQTT_MSG_SIZE];
#define       MSG_BUFFER_SIZE  (50)
PubSubClient  mqttClient(wifiClient);
String        thingName;
const char*   rootTopic         = "Maison/";
const char*   willTopic         = "LWT";
const char*   willMessage       = "offline";
boolean       willRetain        = false;
byte          willQoS           = 0;

/* ------------------------ Messages --------------------------------------- */

#define MESH_ID               6734922
#define GROUP_SWITCH          1
#define GROUP_HT              2
#define GROUP_MOTION          3

typedef struct struct_message {
  int     mesh_id;
  uint8_t sensor_id[6];
  byte    category;
  bool    status ;
  float   temperature;
  float   humidity;
  float   battery;
} struct_message;


struct_message  msg;
uint8_t         incomingData[sizeof(struct struct_message)];
size_t          received_msg_length;

/* ############################ Setup ############################################ */
void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  while (WiFi.status() != WL_CONNECTED) {
    debug(".");
    delay(1000);
  }
  debug("IP address:\t");
  debugln(WiFi.localIP());

  mqttClient.setServer( mqtt_server , 1883);
  String MAC = WiFi.macAddress();
  MAC.replace(":", "");
  thingName = "ESPNowHub";// + MAC;
}

/* ################################# MQTT ########################################### */
void mqttReconnect() {

  if ( mqttClient.connected() ) return;

  String mainTopic = String(rootTopic) + String(thingName) + String("/");

  if (mqttClient.connect( thingName.c_str(), mqtt_username, mqtt_password, (mainTopic +  String(willTopic)).c_str(), willQoS, willRetain, willMessage)) {
    mqttClient.publish((mainTopic +  String("status")).c_str(), "online");
    mqttClient.publish((mainTopic +  String(willTopic)).c_str(), "online");
    mqttClient.publish((mainTopic +  "IP").c_str(), wifiClient.localIP().toString().c_str());
    debugln(F(mainTopic + "/status ... online "));
  }
}

void mqttPublish(char macAdress[], byte deviceCategory, String payload,  size_t len ) {

  strcpy (mqttTopic, (String(rootTopic) + String(thingName) + String("/")).c_str());
  String category;
  if(deviceCategory == GROUP_SWITCH)
    category = "Switch";
  else if(deviceCategory == GROUP_MOTION)
    category = "Motion";
  else if(deviceCategory == GROUP_HT)
    category = "Temperature";
  strcat (mqttTopic, category.c_str());
  strcat (mqttTopic, "/status");
  debug(mqttTopic);
  debug(' ');
  debugln(payload);
  mqttClient.publish(mqttTopic, payload.c_str() , len);
}

/* ############################ Sensors ############################################# */

void sendSensorDataToMQTT() {

  String sensor  = (char *)msg.sensor_id;

  // Payload format : 
  // [{"status":1, "temperature":21.7899999, "humidity":50.503; "battery":3.3},{"deviceId":"00000000"}]
  // Array with 2 objects for influxdbStorage, the second one is a tag in influxDb
  StaticJsonDocument<256> data;
  
  JsonObject measures = data.createNestedObject();
  measures["status"] = (int)msg.status;
  measures["temperature"] = msg.temperature;
  measures["humidity"] = msg.humidity;
  measures["battery"] = msg.battery;

  JsonObject deviceInformations = data.createNestedObject();
  deviceInformations["deviceId"] = sensor.substring(0,6);

  char payload[256];
  size_t n = serializeJson(data, payload);
  mqttPublish((char *)sensor.c_str(), msg.category, payload, n );
}

/* ############################ Loop ############################################# */
void loop() {
  mqttReconnect();
  mqttClient.loop();

  if (Serial.available()) {
    received_msg_length = Serial.readBytesUntil('\n', incomingData, sizeof(incomingData));
    if (received_msg_length == sizeof(incomingData)) {  // got a msg from a sensor
      memcpy(&msg, incomingData, sizeof(msg));
      if ( msg.mesh_id == MESH_ID ) sendSensorDataToMQTT(); // the message come from a known sensor
    }
  }
}