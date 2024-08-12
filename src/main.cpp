#include <Arduino.h>
// #include <IRremoteESP8266.h>
// #include <IRsend.h>
#include <ir_Sharp.h>

const uint16_t kIrLed = 14;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRSharpAc ac(kIrLed);  // Set the GPIO to be used for sending messages.

#include "DHT.h"

#define DHTPIN 12
#define DHTTYPE DHT22 
DHT dht(DHTPIN, DHTTYPE);


#define BOARD_BUTTON_PIN 4
#define BOARD_BUTTON_ACTIVE_LOW true
#define BOARD_LED_PIN 2
#define BOARD_LED_INVERSE true
#define BOARD_LED_BRIGHTNESS 255

#define BLYNK_TEMPLATE_ID "TMPL6Y5x4UMja"
#define BLYNK_TEMPLATE_NAME "Room Controller"

#define BLYNK_FIRMWARE_VERSION                                                                                                                                                                                                                                                                                                                                                                                                "0.1.0"

#define BLYNK_PRINT Serial

#define APP_DEBUG

#define USE_WEMOS_D1_MINI
#include "BlynkEdgent.h"

BlynkTimer timer;

#include <TimeLib.h>
time_t updated_local_time;

void printState() {
  // Display the settings.
  Serial.println("Sharp A/C remote is in the following state:");
  Serial.printf("  %s\n", ac.toString().c_str());
  // Display the encoded IR sequence.
  unsigned char* ir_code = ac.getRaw();
  Serial.print("IR Code: 0x");
  for (uint8_t i = 0; i < kSharpAcStateLength; i++)
    Serial.printf("%02X", ir_code[i]);
  Serial.println();
}

void initAC() {
  ac.begin();

  // kGreeAuto, kGreeDry, kGreeCool, kGreeFan, kGreeHeat
  ac.setMode(kSharpAcCool);
  ac.setFan(kSharpAcFanAuto);
  ac.setTemp(26);  // 16-30C
  ac.setSwingV(kSharpAcSwingVCoanda, false);
  ac.setIon(true);
  ac.setEconoToggle(true);

  Serial.println("Default state of the remote.");
  printState();
  Serial.println("Setting desired state for A/C.");
}

void updateSensorData() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  else {
    Serial.print(F("Humidity: "));
    Serial.print(h);
    Serial.print(F("%  Temperature: "));
    Serial.print(t);
    Serial.println();

    Blynk.virtualWrite(V0, t);
    Blynk.virtualWrite(V1, h);
  }
}

void dailyACturnOff() {
  if ( (hour(updated_local_time) == 4) && (minute(updated_local_time) == 30) && ac.getPower()) {
    Blynk.virtualWrite(V2, 0);
    ac.off();
    ac.send(); 
  }
}

void timerFunction() {
  Blynk.sendInternal("rtc", "sync"); //request current local time for device
  Serial.println();
  Serial.printf("Local time: %02d:%02d:%02d\n", hour(updated_local_time), minute(updated_local_time),second(updated_local_time));
  updateSensorData();
  dailyACturnOff();
}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V2);
  Blynk.sendInternal("rtc", "sync"); //request current local time for device
  dailyACturnOff();
}

BLYNK_WRITE(InternalPinRTC) {   //check the value of InternalPinRTC  
  unsigned long t = param.asLong();      //store time in t variable
  updated_local_time = t;
}

BLYNK_WRITE(V2)
{   
  if(param.asInt())
  {
    ac.on();
    Serial.println("Turning ON AC");
  }
  else
  {
    ac.off();
    Serial.println("Turning OFF AC");
  }
  
  ac.send(); 
}

BLYNK_WRITE(V3)
{ 
  int setTemperature = param.asInt();
  ac.setTemp(setTemperature);
  Serial.println("Temperature set to: " + String(setTemperature) + "C");
  
  if (ac.getPower())  
  {
    ac.send();
  }
}


BLYNK_WRITE(V4)
{   
  if(param.asInt())  
  {
    ac.setMode(kSharpAcDry);
    Serial.println("set to Dry mode");
  }
  else  
  {
    ac.setMode(kSharpAcCool);
    Serial.println("set to Cool mode");
  }

  if (ac.getPower())  
  {
    ac.send();
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  initAC();
  dht.begin();

  BlynkEdgent.begin();

  timer.setInterval(10000L, timerFunction);
}

void loop() {
  BlynkEdgent.run();
  timer.run(); 
}