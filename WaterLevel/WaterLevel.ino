#include <NewPing.h>
#include "ConnectionManager.h"

#define loopInterval 2000
#define waterReserveTopic "WaterReserve"

#define RESET_PIN 0
#define TRIG_PIN 12
#define ECHO_PIN 12

const float BARREL_HEIGHT = 85+10+20;
const float BARREL_DIAMETER = 57;
const float BARREL_RADIUS = BARREL_DIAMETER / 2;
const float BARREL_AREA = BARREL_RADIUS * BARREL_RADIUS * PI;



unsigned long lastLoop = 0;

NewPing sonar(TRIG_PIN, ECHO_PIN);
ConnectionManager connection(RESET_PIN);

void messageReceived(String &topic, String &payload) {
  Serial.println("Incoming: " + topic + " - " + payload);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Setup...");

  connection.setMessageCallback(messageReceived);
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
  unsigned int uS = sonar.ping(); // Send ping, get ping time in microseconds (uS).
  
  float distance = (float)uS / US_ROUNDTRIP_CM;
  float waterLevel = BARREL_HEIGHT - distance;
  float volumeCM = waterLevel * BARREL_AREA;
  float liters = volumeCM / 1000;

  Serial.print("Ping: ");
  Serial.print(liters);
  Serial.println("cm");
  connection.publish(waterReserveTopic, String(volumeCM).c_str());
}

bool wifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}
