/**
 *  @filename   :   mqtt.cpp
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

#include <ESP8266WiFi.h>
#include "PubSubClient.h"
#include "geiger.h"


WiFiClient espClient;
PubSubClient mqttClient(espClient);
char subName[25];

extern char mqttServer[MQTT_SERVER_LENGTH];
extern char mqttTopic[MQTT_TOPIC_LENGTH];
extern uint16_t mqttPort;

const char *configJson = "{\"name\": \"GeigerCounter\",\"state_topic\": \"%s/%s\", \"unit_of_measurement\": \"Count\", \"value_template\": \"{{ value_json.count}}\" }";
const char *stateJson = "{\"count\":%s}";

void mqttCallback(char *topic, byte *payload, uint16_t length) {
    Serial.printf("Message Received on topic %s\n", topic);
}

static void reconnect() {
    if(mqttClient.connect(subName)) {
        Serial.println("MQTT Connected");
        publishConfig();
    } else {
        char errorMes[50];
        sprintf(errorMes, "MQTT Connection failed, rc=%d",mqttClient.state());
        error_display((const __FlashStringHelper *)errorMes);
        Serial.println(errorMes);
    }

}

static boolean publishMes(char *topic, char *payload, boolean retained) {
    if(!mqttClient.connected())
        reconnect();
        if(!mqttClient.connected())
            return false;

    if(!mqttClient.publish(topic, payload, retained)) {
        char errorMes[50];
        sprintf(errorMes, "MQTT Publish failed, rc=%d",mqttClient.state());
        Serial.println(errorMes);
        error_display((const __FlashStringHelper *)errorMes);
        return false;
    }

    return true;
}

boolean publishConfig () {
    uint8_t configLen = strlen(configJson);
    char payload[configLen + MQTT_TOPIC_LENGTH + 10];
    sprintf(payload,configJson,mqttTopic,"state");

    char topic[MQTT_TOPIC_LENGTH+10];
    sprintf(topic,"%s/%s",mqttTopic,"config");

    return publishMes(topic,payload, true);
}

boolean publishCount(uint16_t count) {
    char payload[30];
    char num[10];
    itoa(count, num, 10);
    sprintf(payload,stateJson,num);

    char topic[MQTT_TOPIC_LENGTH + 10];
    sprintf(topic,"%s/%s",mqttTopic,"state");
    return publishMes(topic,payload,false);
}


void initMQTT() {
    mqttClient.setServer(mqttServer, mqttPort);
    mqttClient.setCallback(mqttCallback);
    
    sprintf(subName, "geigercounter-%s", &(WiFi.macAddress().c_str())[9]);
}
