/**
 *  @filename   :   main.h
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

#ifndef INCLUDE_GEIGER_H_
#define INCLUDE_GEIGER_H_

#define GEIGER_PIN D1
#define PEIZO_PIN D0
#define CONFIG_BUTTON D3

//Display Pins
#define DC D2
#define CS D8
#define RST D4

#define DOWN_ARROW 24
#define UP_ARROW 25

#define MQTT_SERVER_LENGTH 30
#define MQTT_TOPIC_LENGTH 40

void callWFM(bool);
void processCounts(void);
void chirpOff(void);
void initDisplay(void);
void initMQTT(void);
void clear_display(void);
void conf_display(void);
void error_display(const __FlashStringHelper *);
void flash_display(void);
void chirp_on(void);
void chirp_off(void);
void configPub(void);
void displayCounts(uint16_t cur, uint16_t min, uint16_t max);
boolean publishConfig (void);
boolean publishCount(uint16_t count);

ICACHE_RAM_ATTR void geigerCount(void);
ICACHE_RAM_ATTR void longPress(void);

#endif /* INCLUDE_GEIGER_H_ */
