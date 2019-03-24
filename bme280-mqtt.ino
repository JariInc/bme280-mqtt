/*
 * Based on work of 
 *   https://github.com/knolleary/pubsubclient/blob/master/examples/mqtt_esp8266/mqtt_esp8266.ino
 *   https://arduinojson.org/v5/example/parser/
 *   https://github.com/finitespace/BME280/blob/master/examples/BME_280_Spi_Sw_Test/BME_280_Spi_Sw_Test.ino
 */

// I2C
#include <Wire.h>
 
// Bundled ESP8266WiFi headers
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

// PubSubClient https://github.com/knolleary/pubsubclient
#include <PubSubClient.h>

// ArduinoJson https://arduinojson.org
#include <ArduinoJson.h>

// BME280 by Tyler Glenn https://github.com/finitespace/BME280
#include <BME280I2C.h>

// Configuration
#include "config.h"

const char* ssid = WIFI_SSID;
const char* password = WIFI_PSK;
IPAddress IP_ADDR;
IPAddress GATEWAY;
IPAddress SUBNET; 
const char* mqtt_server = MQTT_SERVER_ADDR;
char client_id[32];
char state_topic[64];

BME280I2C::Settings settings(
   BME280::OSR_X8,
   BME280::OSR_X16,
   BME280::OSR_X2,
   BME280::Mode_Forced,
   BME280::StandbyTime_1000ms,
   BME280::Filter_16,
   BME280::SpiEnable_False
);
BME280I2C bme(settings);
WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);

  // Set static IP
  WiFi.mode(WIFI_STA);
  WiFi.config(ip, gateway, subnet);  
  WiFi.begin(ssid, password); 

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void reconnect() {  
  // Loop until we're reconnected
  while (!client.connected()) {
    // Attempt to connect
    if (!client.connect(client_id)) {
      // Wait 1 second before retrying
      delay(1000);
    }
  }
}

void setup_bme() {
  pinMode(BME_POWER_PIN, OUTPUT);
  digitalWrite(BME_POWER_PIN, HIGH);
  
  Wire.begin(SDA_PIN, SCL_PIN);
  
  while(!bme.begin()) {
    delay(1000);
  }

  bme.chipModel();
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("setup");
  
  // bme
  setup_bme();
  Serial.println("setup bme");
  
  // wifi
  setup_wifi();
  Serial.println("setup wifi");
  
  // build client id
  sprintf(client_id, "bme280-%X", ESP.getChipId());

  // build state_topic
  sprintf(state_topic, "bme280/%X", ESP.getChipId());
      
  // mqtt
  client.setServer(mqtt_server, 1883);
  Serial.println("setup mqtt");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
    
   publish_measurement();
}

void publish_measurement() {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  float temp(NAN), hum(NAN), pres(NAN);
  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_Pa);

  bme.read(pres, temp, hum, tempUnit, presUnit);
  
  root["temperature"] = temp;
  root["humidity"] = hum;
  root["pressure"] = pres;
  
  char output[200];
  root.printTo(output);

  client.publish(state_topic, output);  
  delay(100);

  digitalWrite(BME_POWER_PIN, LOW);

  Serial.println("sleep");
  ESP.deepSleep(PUBLISH_INTERVAL * 1000000);
}

