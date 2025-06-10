#include <Arduino.h>
#include <ir_Sharp.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>

#define IR_LED_PIN  3

#define BOARD_BUTTON_PIN 4
#define BOARD_BUTTON_ACTIVE_LOW true
#define BOARD_LED_PIN 2
#define BOARD_LED_INVERSE true
#define BOARD_LED_BRIGHTNESS 255

#define BLYNK_TEMPLATE_ID "TMPL6Y5x4UMja"
#define BLYNK_TEMPLATE_NAME "SharpAC Switch"

#define BLYNK_FIRMWARE_VERSION                                                                                                                                                                                                                                                                                                                                                                                                "0.1.0"

#define BLYNK_PRINT Serial

#define APP_DEBUG

#define USE_WEMOS_D1_MINI
#include "BlynkEdgent.h"

BlynkTimer timer;

IRSharpAc ac(IR_LED_PIN);  // Set the GPIO to be used for sending messages.
Adafruit_AHTX0 aht;

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

BLYNK_WRITE(InternalPinRTC) {   //check the value of InternalPinRTC  
  unsigned long t = param.asLong();      //store time in t variable
}

BLYNK_WRITE(V2)
{  
  if(param.asInt() == 1)  ac.on();
  else  ac.off();
  ac.send();
}

void setup() {
  initAC();
  Wire.begin(2,0);
  aht.begin();

  BlynkEdgent.begin();

  timer.setInterval(10000L, timerFunction);
}

void loop() {
  BlynkEdgent.run();
  timer.run(); 
}