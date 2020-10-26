/**
 *  @filename   :   display.cpp
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
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include "geiger.h"

Adafruit_PCD8544 display=Adafruit_PCD8544(DC,CS,RST);


void initDisplay(void) {
  display.begin();
  display.setContrast(60);

  pinMode(PEIZO_PIN, OUTPUT);
  chirp_off();
}

void chirp_on() {
  digitalWrite(PEIZO_PIN, HIGH);
}

void chirp_off() {
  digitalWrite(PEIZO_PIN, LOW);
}

void clear_display() {
    display.clearDisplay();
    display.display();
}

void conf_display() {
  display.clearDisplay();

  display.setTextColor(BLACK,WHITE);
  display.setTextSize(2);
  display.setCursor(21,17);
  display.print(F("CONF"));
  display.display();
}

void error_display(const __FlashStringHelper *str) {
  Serial.println(str);
  display.clearDisplay();
  display.setTextColor(BLACK,WHITE);
  display.setTextSize(1);

  display.print(str);
  display.display();

}

void flash_display() {
  display.clearDisplay();

  display.setTextColor(BLACK,WHITE);
  display.setTextSize(2);
  display.setCursor(12,17);
  display.print(F("FLASH"));
  display.display();
}

void displayCounts(uint16_t cur, uint16_t min, uint16_t max) {
  display.clearDisplay();
  
  display.setTextColor(BLACK,WHITE);
  display.setTextSize(2);

  display.setCursor(9,0);
  display.print(F("Count:"));

  uint8_t xcord=18;
  if(cur<10) {
      xcord=36;
  } else if(cur<100) {
      xcord=30;
  } else if (cur < 1000) {
      xcord=24;
  }

  display.setCursor(xcord,16);
  char counter[6];
  itoa(cur,counter,10);
  display.print(counter);

  display.setTextSize(1);
  display.setCursor(0,40);
  display.write(UP_ARROW);
  display.print(F(":"));
  itoa(min,counter,10);
  display.setCursor(12,40);
  display.print(counter);

  display.setCursor(48,40);
  display.write(DOWN_ARROW);
  display.print(F(":"));
  display.setCursor(60,40);
  itoa(max,counter,10);
  display.print(counter);

  display.display();
}