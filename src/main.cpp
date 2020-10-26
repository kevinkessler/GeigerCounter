/**
 *  @filename   :   main.cpp
 *  @brief      :   MQTT Geiger Counter
 *  @author     :   Kevin Kessler
 *
 * Copyright (C) 2020 Kevin Kessler
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>          
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <WiFiManager.h>
#include <SPI.h>
#include "Ticker.h"
#include "geiger.h"

volatile uint16_t counters[6];
volatile uint8_t cntIdx=0;
uint16_t maxCount=0;
uint16_t minCount=0xffff;
bool fullMin;
volatile bool chirp=false;

const char *hostname = "geigermqtt";
char mqttServer[MQTT_SERVER_LENGTH];
char mqttTopic[MQTT_TOPIC_LENGTH];
uint16_t mqttPort;
bool configMode = false;
uint8_t flash_toggle=0;
volatile bool buttonLongPress = false; 
volatile uint32_t lastPressTime;

Ticker processer(processCounts, 10000);
Ticker chirpTicker(chirpOff,100);
Ticker mqttConfSender(configPub,3600000);

struct mqttConfig {
  uint32_t valid;
  char server[MQTT_SERVER_LENGTH];
  uint16_t port;
  char topic[MQTT_TOPIC_LENGTH]; 
};


void configModeCallback(WiFiManager *wfm) {
  Serial.println(F("Config Mode"));
  conf_display();
  Serial.println(WiFi.softAPIP());
  configMode = true;
  Serial.println(wfm->getConfigPortalSSID());
}



void printMQTT() {
  Serial.print(F("Server "));
  Serial.println(mqttServer);

  Serial.print(F("Port "));
  Serial.println(mqttPort,DEC);

  Serial.print(F("Topic "));
  Serial.println(mqttTopic);
}

void readEEPROM() {
  mqttConfig conf;
  EEPROM.get(0,conf);
  
  if (conf.valid ==0xDEADBEEF) {
    strncpy(mqttServer, conf.server, MQTT_SERVER_LENGTH);
    strncpy(mqttTopic, conf.topic, MQTT_TOPIC_LENGTH);
    mqttPort=conf.port;
  }
  else {
    WiFiManager wifiManager;

    strncpy(mqttServer,"",MQTT_SERVER_LENGTH);
    strncpy(mqttTopic,"",MQTT_TOPIC_LENGTH);
    mqttPort = 1883;

    Serial.println(F("Setup WIFI Manager"));
    printMQTT();
  
    callWFM(false);
  }
}

void writeEEPROM() {
  Serial.println(F("Writing MQTT Config"));

  mqttConfig conf;

  conf.valid = 0xDEADBEEF;
  strncpy(conf.server, mqttServer, MQTT_SERVER_LENGTH);
  strncpy(conf.topic, mqttTopic, MQTT_TOPIC_LENGTH);
  conf.port = mqttPort;

  EEPROM.put(0,conf);
  EEPROM.commit();
}

void callWFM(bool connect) {
  WiFiManager wfm;

  wfm.setAPCallback(configModeCallback);

  WiFiManagerParameter mqtt_server("server", "MQTT Server", mqttServer, MQTT_SERVER_LENGTH);
  char port_string[6];
  itoa(mqttPort, port_string,10);
  WiFiManagerParameter mqtt_port("port", "MQTT port", port_string, 6);
  WiFiManagerParameter mqtt_topic("topic", "MQTT Topic", mqttTopic,MQTT_TOPIC_LENGTH);

  wfm.addParameter(&mqtt_server);
  wfm.addParameter(&mqtt_port);
  wfm.addParameter(&mqtt_topic);

  if(connect) {
    if(!wfm.autoConnect()) {
      error_display(F("Failed to connect and hit timeout"));
      ESP.restart();
      delay(5000);
    }
  } else {
      if(!wfm.startConfigPortal()) {
        error_display(F("Portal Error"));
        ESP.restart();
        delay(5000);
    }

  }

  strncpy(mqttServer, mqtt_server.getValue(), MQTT_SERVER_LENGTH);
  strncpy(mqttTopic, mqtt_topic.getValue(), MQTT_TOPIC_LENGTH);
  mqttPort = atoi(mqtt_port.getValue());

  if(configMode) 
    writeEEPROM();
  
  clear_display();
}

void otaSetup() {

  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";
 
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    //Serial.printf("Progress: %u%%\n", (progress / (total / 100)));
    //Slow down the flashing of "FLASH" by a factor of 3, so it looks better
    if(++flash_toggle == 6)
      flash_toggle=0;

    Serial.printf("%u %u \n",flash_toggle, flash_toggle / 3);
    if(flash_toggle / 3) {
      flash_display();
    } else {
      clear_display();
    }
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      error_display(F("OTA Auth Failed"));
    } else if (error == OTA_BEGIN_ERROR) {
      error_display(F("OTA Begin Failed"));
    } else if (error == OTA_CONNECT_ERROR) {
      error_display(F("Connect Failed"));
    } else if (error == OTA_RECEIVE_ERROR) {
      error_display(F("OTA Receive Failed"));
    } else if (error == OTA_END_ERROR) {
      error_display(F("OTA End Failed"));
    }
  });

  ArduinoOTA.begin();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");
  EEPROM.begin(128);

  initDisplay();

  readEEPROM();

  printMQTT();

  callWFM(true);

  otaSetup();

  initMQTT();

  pinMode(GEIGER_PIN,INPUT);
  attachInterrupt(digitalPinToInterrupt(GEIGER_PIN), geigerCount,FALLING);

  pinMode(CONFIG_BUTTON,INPUT);
  attachInterrupt(digitalPinToInterrupt(CONFIG_BUTTON), longPress, CHANGE);

  processer.start();
  mqttConfSender.start();
}

void loop() {
  if(buttonLongPress) {
    callWFM(false);
    buttonLongPress = false;
  }

  processer.update();
  chirpTicker.update();
  mqttConfSender.update();

  ArduinoOTA.handle();

  if (chirp)
  {
    chirp = false;
    chirp_on();
    chirpTicker.start();
  }

}

void chirpOff() {
  chirpTicker.stop();
  chirp_off();
}

void processCounts() {
  uint8_t prevIdx=cntIdx;
  ++cntIdx;
  if(cntIdx==6)
  {
    cntIdx=0;
    fullMin=true;
  }
  
  uint16_t totCnt=0;
  for(uint8_t n=0;n<6;n++) {
    totCnt+=counters[n];
  }

  if(fullMin && totCnt<minCount) {
    minCount=totCnt;
  }

  if (totCnt>maxCount) {
    maxCount=totCnt;
  }
  counters[cntIdx] = 0;

  displayCounts(totCnt,minCount,maxCount);
  publishCount(counters[prevIdx]);

  Serial.printf("Count %d, Idx %d, Max %d, Min %d, Cur %d\n", totCnt,prevIdx,maxCount, minCount,counters[prevIdx]);

}

void configPub() {
  publishConfig();
}

ICACHE_RAM_ATTR void geigerCount() {
  counters[cntIdx]++;
  chirp=true;
}

ICACHE_RAM_ATTR void longPress() {
  uint8_t curState = digitalRead(CONFIG_BUTTON);
  
  if (curState) {
  // Button Released
    if((millis() - lastPressTime) > 1000)
    {
      buttonLongPress = true;
    }

  } else {
  // Button Pressed
    lastPressTime = millis();
    buttonLongPress = false;
  }


}