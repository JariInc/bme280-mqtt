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

// PubSubClient https://github.com/knolleary/pubsubclient
#include <PubSubClient.h>

// ArduinoJson https://arduinojson.org
#include <ArduinoJson.h>

// BME280 by Tyler Glenn https://github.com/finitespace/BME280
#include <BME280I2C.h>

// Ticker
#include <Ticker.h>

// Configuration
#define SDA_PIN D3
#define SCL_PIN D4
#define PUBLISH_INTERVAL 60

const char* ssid = "";
const char* password = "";
const char* mqtt_server = "";
char client_id[32];
char state_topic[64];

BME280I2C::Settings settings(
   BME280::OSR_X16,
   BME280::OSR_X16,
   BME280::OSR_X16,
   BME280::Mode_Forced,
   BME280::StandbyTime_1000ms,
   BME280::Filter_16,
   BME280::SpiEnable_False
);
BME280I2C bme(settings);
WiFiClient espClient;
PubSubClient client(espClient);
Ticker ticker;

void setup_wifi() {
  delay(10);

  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password); 

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void reconnect() {  
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Attempt to connect
    if (client.connect(client_id)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup_bme() {
  Wire.begin(SDA_PIN, SCL_PIN);
  
  while(!bme.begin()) {
    Serial.println("Could not find BME280 sensor!");
    delay(1000);
  }
  
  switch(bme.chipModel()) {
     case BME280::ChipModel_BME280:
       Serial.println("Found BME280 sensor! Success.");
       break;
     case BME280::ChipModel_BMP280:
       Serial.println("Found BMP280 sensor! No Humidity available.");
       break;
     default:
       Serial.println("Found UNKNOWN sensor! Error!");
  }
}

void setup() {
  // serial
  Serial.begin(115200);
  
  // bme
  setup_bme();
  
  // wifi
  setup_wifi();

  // build client id
  sprintf(client_id, "bme280-%X", ESP.getChipId());
  Serial.print("MQTT client id: ");
  Serial.println(client_id);

  // build state_topic
  sprintf(state_topic, "bme280/%X", ESP.getChipId());
  Serial.print("MQTT publish topic: ");
  Serial.println(state_topic);
      
  // mqtt
  client.setServer(mqtt_server, 1883);

  // ticker
  ticker.attach(PUBLISH_INTERVAL, publish_measurement);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
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
  
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print("C ");

  Serial.print("Humidity: ");
  Serial.print(hum);
  Serial.print("% RH ");
  
  Serial.print("Pressure: ");
  Serial.print(pres);
  Serial.println("Pa");
  
  char output[200];
  root.printTo(output);

  Serial.print("Published: ");
  Serial.println(output);

  client.publish(state_topic, output);
}

