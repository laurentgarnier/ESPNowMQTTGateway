#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

//      WARNING IMPORTANT NOTE  //
// No usage of Serial for debugging because it is used for 
// communication with the second micro controller

uint8_t  staticMACAddress[]      = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};

// When ESPNow message is received, it is sent as it is on Serial
void OnDataRecv(u8 *mac, u8 *incomingData, u8 len)
{
  Serial.write(incomingData, len);
  Serial.write("\n");
}

void setup()
{
  Serial.begin(115200);
  delay(500);

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  wifi_set_macaddr(STATION_IF, &staticMACAddress[0]);
 
  if (esp_now_init() != 0)
  {
    delay(1000);
    ESP.restart();
  }

  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  // Define receive function
  esp_now_register_recv_cb(OnDataRecv);
}

void loop()
{
}