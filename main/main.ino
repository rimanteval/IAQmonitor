/******************************************************************************
 * Copyright 2018 Google
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/
#include "esp32-mqtt.h"
#include <Wire.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Adafruit_BMP280.h>
#include <ArduinoJson.h>
#include "SparkFun_SCD30_Arduino_Library.h" 


//SCD30
#define SDA_1 17
#define SCL_1 16

//BMP280
#define SDA_2 18
#define SCL_2 19

TwoWire I2Cone = TwoWire(0);
TwoWire I2Ctwo = TwoWire(1);

SCD30 airSensor;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

Adafruit_BMP280 bmp(&I2Ctwo);

void setup() {
  Serial.begin(115200);
  I2Cone.begin(SDA_1, SCL_1);
  I2Ctwo.begin(SDA_2, SCL_2);
  airSensor.begin(I2Cone); //SCD30 begin
  timeClient.begin();
  bool status;    
  status = bmp.begin(0x76);  //I2C address is set to 0x76
  if (!status) {
      Serial.println("Could not find a valid BMP280 sensor, check wiring!");
  }
  setupCloudIoT();
  timeClient.setTimeOffset(10800);
}

String formattedDate;
String dayStamp;
unsigned long lastMillis = 0;
void loop() {
  char buffer[512];
  StaticJsonDocument<422> doc;
  mqtt->loop();
  delay(10);
  if (!mqttClient->connected()) {
    connect();
  }
  // publish a message every 5 minutes.
  if (millis() - lastMillis > 300000) {
    lastMillis = millis();
    while(!timeClient.update()) {
      timeClient.forceUpdate();
    }
    Serial.println("MQTT.....");
    formattedDate = timeClient.getFormattedDate();
    int splitT = formattedDate.indexOf("T");
    dayStamp = formattedDate.substring(0, splitT);
    doc["device_id"] = "esp32";
    doc["date"] = dayStamp;
    doc["time"] = timeClient.getFormattedTime();
   
    if (airSensor.dataAvailable())
    {
      doc["temperature_1"] = airSensor.getTemperature();
      doc["co2"] = airSensor.getCO2();
      doc["humidity"] = airSensor.getHumidity();
    }else {
      doc["temperature_1"] = 0;
      doc["co2"] = 0;
      doc["humidity"] = 0;
    }
    doc["temperature_2"] = bmp.readTemperature();
    doc["pressure"] = (bmp.readPressure() / 100.0F);      
    serializeJson(doc, buffer);
    publishTelemetry(buffer);
  }
}
