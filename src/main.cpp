#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>

#define IR_LED_PIN 13
#define PHYS_BUTTON_PIN 12

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

int btnState = LOW;
const uint16_t phys_button_debounce_time_ms = 500;
unsigned long last_phys_button_triggered_time = 0;

void checkPhysicalButton() {
  if (digitalRead(PHYS_BUTTON_PIN) == HIGH) {
    if (btnState != HIGH && millis() - last_phys_button_triggered_time >= phys_button_debounce_time_ms) {
      bool power_state = ac.getPower();

      if (power_state) {
        Serial.println("AC turned off by button.");
        ac.off();
      }
      else{
        Serial.println("AC turned on by button.");
        ac.on();
      }

      ac.send();
      Blynk.virtualWrite(V2, !power_state);
      last_phys_button_triggered_time = millis();
    }
    btnState = HIGH;
  } else {
    btnState = LOW;
  }
}

void updateSensorData() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);

  if (!isnan(temp.temperature) && !isnan(humidity.relative_humidity)) {
    Serial.printf("Temperature: %.1fC | Humidity: %.1f%%", temp.temperature, humidity.relative_humidity);
    Blynk.virtualWrite(V1, humidity.relative_humidity);
    Blynk.virtualWrite(V0, temp.temperature);
  }
}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V2);
}

BLYNK_WRITE(V2) {
  if (param.asInt() == 1) {
    ac.on();
    Serial.println("AC turned on.");
  }
  else{
    ac.off();
    Serial.println("AC turned off.");
  } 
  
  ac.send();
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(PHYS_BUTTON_PIN, INPUT);

  BlynkEdgent.begin();
  
  if (!aht.begin()) {
    Serial.println("AHT10 sensor not found!");
    while (1); // halt system or handle gracefully
  }

  ac.begin();
  ac.setTemp(25);
  ac.setFan(kSharpAcFanAuto);
  ac.setSwingV(kSharpAcSwingVMid);
  ac.setIon(false);
  ac.setTurbo(false);

  sensor_timer.setInterval(10000L, updateSensorData);
  button_timer.setInterval(50L, checkPhysicalButton);
}

void loop() {
  BlynkEdgent.run();
  sensor_timer.run();
  button_timer.run();
}
