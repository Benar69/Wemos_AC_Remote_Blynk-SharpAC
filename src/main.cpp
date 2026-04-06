#include <Arduino.h>
#include <Wire.h>
#include <AHTxx.h>

#define IR_LED_PIN 13
#define PHYS_BUTTON_PIN 12

#define TEMP_OFFSET_C 8
#define BLYNK_TEMPLATE_ID "TMPL6Y5x4UMja"
#define BLYNK_TEMPLATE_NAME "SharpAC Switch"
#define BLYNK_FIRMWARE_VERSION "2.0.4"

#define USE_WEMOS_D1_MINI
#define BLYNK_PRINT Serial
#define APP_DEBUG

#include "BlynkEdgent.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_Sharp.h>

#define AC_AUTO 0
#define AC_COOL 1
#define AC_DRY 2

// Virtual Pin Map:
//   V0 → Sensor string "🌡 XX.X°C  💧 XX.X%"  (Label / Value Display widget)
//   V1 → (free)
//   V2 → Power switch  (Button widget, Switch mode)
//   V3 → Set temperature  (Slider / Numeric Input widget)
//   V4 → AC mode  (Segmented Switch widget: 0=Auto, 1=Cool, 2=Dry)

BlynkTimer sensor_timer;
BlynkTimer button_timer;

IRSharpAc ac(IR_LED_PIN);

AHTxx aht10(AHTXX_ADDRESS_X38, AHT1x_SENSOR);

int btnState = LOW;
int current_mode = AC_AUTO;
const uint16_t phys_button_debounce_time_ms = 500;
unsigned long last_phys_button_triggered_time = 0;
const char *mode_string[] = {"Auto", "Cool", "Dry"};

float tempC = 0;
float humidity = 0;

bool on_sync = false;

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
      else {
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

void sendSensorStr() {
  char sensorStr[64];
  snprintf(sensorStr, sizeof(sensorStr), "Mode: %s   | Temp: %.1fC   Hum: %.1f%%", mode_string[current_mode], tempC, humidity);
  Blynk.virtualWrite(V0, sensorStr);
}

void updateSensorData() {
  tempC   = aht10.readTemperature() - TEMP_OFFSET_C;
  humidity = aht10.readHumidity(AHTXX_USE_READ_DATA);

  bool tempOk     = (tempC    != AHTXX_ERROR);
  bool humidityOk = (humidity != AHTXX_ERROR);

  // --- Serial feedback ---
  Serial.print(F("Temperature...: "));
  if (tempOk) { Serial.print(tempC);    Serial.println(F(" +-0.3C")); }
  else        { printStatus(); if (aht10.softReset()) Serial.println(F("reset success")); else Serial.println(F("reset failed")); }

  Serial.print(F("Humidity......: "));
  if (humidityOk) { Serial.print(humidity); Serial.println(F(" +-2%")); }
  else            { printStatus(); if (aht10.softReset()) Serial.println(F("reset success")); else Serial.println(F("reset failed")); }

  // --- Pack both values into a single string and send on V0 ---
  if (tempOk && humidityOk) {
    sendSensorStr();
  }
}

BLYNK_CONNECTED() {
  on_sync = true;

  Blynk.syncVirtual(V2);  // Sync the power mode 
  Blynk.syncVirtual(V3);  // Set Temperature
  Blynk.syncVirtual(V4);  // Set Mode

  ac.send();
  
  on_sync = false;
}

// Power switch
BLYNK_WRITE(V2) {
  if (param.asInt() == 1) {
    ac.on();
    Serial.println("AC turned on.");
  }
  else {
    ac.off();
    Serial.println("AC turned off.");
  }
  if (!on_sync) ac.send();
}

// Set temperature
BLYNK_WRITE(V3) {
  uint8_t setTempC = param.asInt();
  ac.setTemp(setTempC);
  Serial.printf("AC Temp set to: %dC\n", setTempC);
  if (ac.getPower() && !on_sync) ac.send();
}

// AC mode: 0 = Auto, 1 = Cool, 2 = Dry
BLYNK_WRITE(V4) {
  int mode = param.asInt();
  switch (mode) {
    case 0:
      ac.setMode(kSharpAcAuto);
      current_mode = AC_AUTO;
      Serial.println("AC Mode: Auto");
      break;
    case 1:
      ac.setMode(kSharpAcCool);
      current_mode = AC_COOL;
      Serial.println("AC Mode: Cool");
      break;
    case 2:
      ac.setMode(kSharpAcDry);
      current_mode = AC_DRY;
      Serial.println("AC Mode: Dry");
      break;
    default:
      Serial.printf("AC Mode: unknown value %d\n", mode);
      break;
  }
  sendSensorStr();
  if (ac.getPower() && !on_sync) ac.send();
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
    Serial.println(F("AHT1x not connected or fail to load calibration coefficient"));
  }

  ac.begin();
  ac.setTemp(25);
  ac.setMode(kSharpAcAuto);
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