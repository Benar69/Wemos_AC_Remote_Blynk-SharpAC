#include <Arduino.h>
#include <Wire.h>
#include <AHTxx.h>

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
float ahtValue;                               //to store T/RH result

AHTxx aht10(AHTXX_ADDRESS_X38, AHT1x_SENSOR);

int btnState = LOW;
const uint16_t phys_button_debounce_time_ms = 500;
unsigned long last_phys_button_triggered_time = 0;

void printStatus()
{
  switch (aht10.getStatus())
  {
    case AHTXX_NO_ERROR:
      Serial.println(F("no error"));
      break;

    case AHTXX_BUSY_ERROR:
      Serial.println(F("sensor busy, increase polling time"));
      break;

    case AHTXX_ACK_ERROR:
      Serial.println(F("sensor didn't return ACK, not connected, broken, long wires (reduce speed), bus locked by slave (increase stretch limit)"));
      break;

    case AHTXX_DATA_ERROR:
      Serial.println(F("received data smaller than expected, not connected, broken, long wires (reduce speed), bus locked by slave (increase stretch limit)"));
      break;

    case AHTXX_CRC8_ERROR:
      Serial.println(F("computed CRC8 not match received CRC8, this feature supported only by AHT2x sensors"));
      break;

    default:
      Serial.println(F("unknown status"));    
      break;
  }
}

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
  ahtValue = aht10.readTemperature(); //read 6-bytes via I2C, takes 80 milliseconds
  
  Serial.print(F("Temperature...: "));
  if (ahtValue != AHTXX_ERROR) //AHTXX_ERROR = 255, library returns 255 if error occurs
  {
    Blynk.virtualWrite(V0, ahtValue);
    Serial.print(ahtValue);
    Serial.println(F(" +-0.3C"));
  }
  else
  {
    printStatus();

    if   (aht10.softReset() == true) Serial.println(F("reset success")); //as the last chance to make it alive
    else                             Serial.println(F("reset failed"));
  }

  ahtValue = aht10.readHumidity(AHTXX_USE_READ_DATA); //use 6-bytes from temperature reading, takes zero milliseconds!!!

  Serial.print(F("Humidity......: "));
  
  if (ahtValue != AHTXX_ERROR) //AHTXX_ERROR = 255, library returns 255 if error occurs
  {
    Blynk.virtualWrite(V1, ahtValue);
    Serial.print(ahtValue);
    Serial.println(F(" +-2%"));
  }
  else
  {
    printStatus();
    
    if   (aht10.softReset() == true) Serial.println(F("reset success")); //as the last chance to make it alive
    else                             Serial.println(F("reset failed"));
  }

}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V2);
}

BLYNK_WRITE(V3) {
  uint8_t setTempC = param.asInt();
  ac.setTemp(setTempC);
  Serial.printf("AC Temp set to: %dC\n", setTempC);

  if(ac.getPower()) ac.send();
  
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
  
  if (aht10.begin() == true) {
    sensor_timer.setInterval(10000L, updateSensorData);
    
  }
  else {
    Serial.println(F("AHT1x not connected or fail to load calibration coefficient")); //(F()) save string to flash & keeps dynamic memory free
  }

  ac.begin();
  ac.setTemp(25);
  ac.setFan(kSharpAcFanAuto);
  ac.setSwingV(kSharpAcSwingVMid);
  ac.setIon(false);
  ac.setTurbo(false);

  
  button_timer.setInterval(50L, checkPhysicalButton);
}

void loop() {
  BlynkEdgent.run();
  sensor_timer.run();
  button_timer.run();
}
