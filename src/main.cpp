#include <Arduino.h>

#include <Wire.h>
#include <Adafruit_AHTX0.h>

#define IR_LED_PIN  13

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

IRSharpAc ac(IR_LED_PIN);  // Set the GPIO to be used for sending messages.
Adafruit_AHTX0 aht;

int btnState = HIGH;

void checkPhysicalButton()
{
  if (digitalRead(BOARD_BUTTON_PIN) == LOW) {
    // btnState is used to avoid sequential toggles
    if (btnState != LOW) {

      // Toggle AC power State
      bool power_state = ac.getPower();

      if(power_state) ac.off();
      else ac.on();

      // Update Button Widget
      Blynk.virtualWrite(V2, power_state);
    }
    btnState = LOW;
  } 
  else {
    btnState = HIGH;
  }
}

void initAC() {
  ac.begin();
}

void updateSensorData() {
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);

  float t = temp.temperature;
  float h = humidity.relative_humidity;

  if (isnan(h) || isnan(t)) {
    return;
  }
  else {
    Blynk.virtualWrite(V0, t);
    Blynk.virtualWrite(V1, h);
  }
}

void timerFunction() {
  Blynk.sendInternal("rtc", "sync"); //request current local time for device
  updateSensorData();
}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V2);
  Blynk.sendInternal("rtc", "sync"); //request current local time for device
}

BLYNK_WRITE(V2)
{  
  if(param.asInt() == 1)  ac.on();
  else  ac.off();
  ac.send();
}

void setup() {
  initAC();
  aht.begin();

  BlynkEdgent.begin();

  sensor_timer.setInterval(10000L, timerFunction);
  button_timer.setInterval(100L, checkPhysicalButton);
}

void loop() {
  BlynkEdgent.run();
  sensor_timer.run();
  button_timer.run();
}