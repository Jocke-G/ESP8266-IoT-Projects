#include <DHT.h>
#include <DHT_U.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <MQTTClient.h>

#define mqttServer "192.168.1.x"
#define mqttPort "1883"
#define mqttUser "user"
#define mqttPass "password"
#define basetopic "MyHouse/Indoor/MyRoom/"
#define hostname          "MyRoom"
#define loopInterval      2000
#define targetTemperatureTopic "TargetTemperature"    
#define temperatureTopic       "Temperature"    
#define humidityTopic          "Humidity"    
#define heaterTopic            "Heater"
#define temperatureTreshold 0.5

#define RESET_PIN 0
#define DHT_PIN 2
#define RELAY_PIN 3
#define DHTTYPE DHT22

float targetTemperature = 12;
unsigned long lastLoop = 0;

DHT dht(DHT_PIN, DHTTYPE);
WiFiClient net;

MQTTClient mqtt;

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
  if(topic == createTopic(targetTemperatureTopic, true)){
    targetTemperature = payload.toFloat();
    Serial.print("Target temperature changd to:");
    Serial.println(targetTemperature);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Setup...");

  pinMode(RESET_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  mqtt.begin(mqttServer, net);
  mqtt.onMessage(messageReceived);

  long initialWifiMillis = millis();
  Serial.print("Initial load WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() > initialWifiMillis + 5000) {
      break;
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println("Setup Done");
}

void loop() {
  if (!wifiConnected()) {
    Serial.println("WiFi is not connected, showing portal.");
    showConfigPortal();
  }
  if (digitalRead(RESET_PIN) == LOW ) {
    showConfigPortal();
  }

  mqtt.loop();

  unsigned long currentMillis = millis();
  if (currentMillis - lastLoop < loopInterval) {
    return;
  }
  lastLoop = currentMillis;

  if (!mqtt.connected()) {
    reconnectMqtt();
  }
  readSensor();
}

void reconnectMqtt() {
  if (mqtt.connected()) {
    return;
  }

  Serial.println("MQTT is not connected");
  if (!wifiConnected()) {
    Serial.println("WiFi not connected, aborting MQTT Connect.");
    return;
  }

  Serial.println("Connecting MQTT...");
  if (mqtt.connect(hostname, mqttUser, mqttPass)) {
    Serial.println("MQTT Connected");
    publishMqtt("state", "Connected");
    mqtt.subscribe(createTopic(targetTemperatureTopic, true));
  } else {
    Serial.println("MQTT failed");
  }
}

String createTopic(String topicPart, bool set){
  if(set){
    return String(basetopic) + topicPart + "/Set";
  }else{
    return String(basetopic) + topicPart;
  }
}

bool wifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void readSensor() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" *C ");

  evaluateTemperature(temperature);

  if (wifiConnected()) {
    publishMqtt(humidityTopic, String(humidity).c_str());
    publishMqtt(temperatureTopic, String(temperature).c_str());
  }
}

void evaluateTemperature(float temperature) {
  bool currentState = digitalRead(RELAY_PIN);
  Serial.print("Current heater state: ");
  Serial.println(currentState);
  float lowerTemperature = targetTemperature - temperatureTreshold;
  Serial.print("Lower temperature limit: ");
  Serial.println(lowerTemperature);
  if ((temperature < lowerTemperature) && currentState) {
    Serial.println("Temperature is low, turning on heater");
    publishMqtt(heaterTopic, "on");
    digitalWrite(RELAY_PIN, LOW);
  } else if ((temperature > targetTemperature + temperatureTreshold) && !currentState) {
    Serial.println("Temperature is high, turning off heater");
    publishMqtt(heaterTopic, "off");
    digitalWrite(RELAY_PIN, HIGH);
  }
}

void publishMqtt(String topicPart, String message) {
  String topic = createTopic(topicPart, false);

  Serial.print("Publish Topic: ");
  Serial.println(topic.c_str());

  mqtt.publish(topic, message);
}

void showConfigPortal() {
  WiFiManager wifiManager;

  //reset settings - for testing
  //wifiManager.resetSettings();

  wifiManager.setTimeout(120);

  if (!wifiManager.startConfigPortal(hostname)) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  Serial.println("WiFi Connected");
}
