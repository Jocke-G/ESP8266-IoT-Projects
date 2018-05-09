#include <NewPingESP8266.h> //https://github.com/jshaw/NewPingESP8266
#include "ConnectionManager.h"

#define loopInterval 10000
#define waterReserveTopic "WaterReserve"

const float BARREL_HEIGHT = 85+10+20;
const float BARREL_DIAMETER = 57;
const float BARREL_RADIUS = BARREL_DIAMETER / 2;
const float BARREL_AREA = BARREL_RADIUS * BARREL_RADIUS * PI;

#define RESET_PIN 0
#define TRIG_PIN 12
#define ECHO_PIN 16

unsigned long lastLoop = 0;

NewPingESP8266 sonar(TRIG_PIN, ECHO_PIN);
ConnectionManager connection(RESET_PIN);

void messageReceived(String &topic, String &payload) {
  Serial.println("Incoming: " + topic + " - " + payload);
}

void saveConfigCallback () {
  Serial.println("Should save config");
  connection.shouldSaveConfig = true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Setup...");

  connection.setMessageCallback(messageReceived);
  connection.setSaveCallback(saveConfigCallback);
  connection.setup();
  Serial.println("Setup Done");
}

void loop() {
  connection.loop();

  unsigned long currentMillis = millis();
  if (currentMillis - lastLoop < loopInterval) {
    return;
  }
  lastLoop = currentMillis;
  
  readSonarSensor();
}

void readSonarSensor(){
  Serial.print("Ping: ");
  float distance = sonar.ping_cm();
  Serial.print(sonar.ping_cm());
  Serial.print(" cm, ");

  float waterLevel = BARREL_HEIGHT - distance;
  float volumeCM = waterLevel * BARREL_AREA;
  float liters = volumeCM / 1000;
  
  Serial.print(liters);
  Serial.println(" liters");
  connection.publish(waterReserveTopic, String(liters).c_str());
}
