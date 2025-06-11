#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>

#define IR_LED_PIN 13

#define BLYNK_TEMPLATE_ID "TMPL6Y5x4UMja"
#define BLYNK_TEMPLATE_NAME "SharpAC Switch"
#define BLYNK_FIRMWARE_VERSION "1.0.0"

#define USE_WEMOS_D1_MINI
#define BLYNK_PRINT Serial
#define APP_DEBUG

#include "BlynkEdgent.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Sharp.h>

BlynkTimer sensor_timer;
BlynkTimer button_timer;

IRSharpAc ac(IR_LED_PIN);
Adafruit_AHTX0 aht;

int btnState = HIGH;

void checkPhysicalButton() {
  if (digitalRead(BOARD_BUTTON_PIN) == LOW) {
    if (btnState != LOW) {
      bool power_state = ac.getPower();
      power_state ? ac.off() : ac.on();
      ac.send();
      Blynk.virtualWrite(V2, !power_state);
    }
    btnState = LOW;
  } else {
    btnState = HIGH;
  }
}

void updateSensorData() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);

  if (!isnan(temp.temperature) && !isnan(humidity.relative_humidity)) {
    Blynk.virtualWrite(V0, temp.temperature);
    Blynk.virtualWrite(V1, humidity.relative_humidity);
  }
}

void timerFunction() {
  updateSensorData();
}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V2);
}

BLYNK_WRITE(V2) {
  if (param.asInt() == 1) ac.on();
  else ac.off();
  
  ac.send();
}

void setup() {
  BlynkEdgent.begin();

  if (!aht.begin()) {
    Serial.println("AHT10 sensor not found!");
    while (1); // halt system or handle gracefully
  }

  ac.begin();

  sensor_timer.setInterval(10000L, timerFunction);
  button_timer.setInterval(100L, checkPhysicalButton);
}

void loop() {
  BlynkEdgent.run();
  sensor_timer.run();
  button_timer.run();
}
