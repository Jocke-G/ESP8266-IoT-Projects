#include "Arduino.h"
#include "ConnectionManager.h"

ConnectionManager::ConnectionManager(int pin) {
  _pin = pin;
}

void ConnectionManager::saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void ConnectionManager::setMessageCallback(MQTTClientCallbackSimple cb) {
  mqtt.onMessage(cb);
}

void ConnectionManager::setup() {
  pinMode(_pin, INPUT);

  mqtt.begin(mqttServer, net);

  mountFileSystem();

  long initialWifiMillis = millis();
  Serial.print("Initial load WiFi");
  while (!wifiConnected()) {
    if (millis() > initialWifiMillis + 5000) {
      break;
    }
    delay(500);
    Serial.print(".");
  }

  if (wifiConnected()) {
    printWifiConnected();
  } else {
    Serial.println("Initial WiFi failed, showing portal.");
    showConfigPortal();
  }

  setupOta();
}

void ConnectionManager::loop() {
  if (!wifiConnected()) {
    Serial.println("WiFi is not connected, showing portal.");
    showConfigPortal();
  }

  if (digitalRead(_pin) == LOW ) {
    showConfigPortal();
  }

  ArduinoOTA.handle();

  if (!mqtt.connected()) {
    reconnectMqtt();
  }

  mqtt.loop();
}

void ConnectionManager::publish(String topicPart, String message) {
  String topic = createTopic(topicPart, false);
  Serial.print("Publish Topic: ");
  Serial.println(topic);
  mqtt.publish(topic, message);
}

void ConnectionManager::mountFileSystem() {
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          strcpy(mqttServer, json["mqttServer"]);
          strcpy(mqttPort, json["mqttPort"]);
          strcpy(mqttUser, json["mqttUser"]);
          strcpy(mqttPass, json["mqttPass"]);

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
}

void ConnectionManager::setupOta() {
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(hostname);

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

void ConnectionManager::printWifiConnected() {
  Serial.println("WiFi Connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

bool ConnectionManager::wifiConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void ConnectionManager::reconnectMqtt() {
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
    publish("state", "Connected");
    //mqtt.subscribe(createTopic(targetTemperatureTopic, true));
  } else {
    Serial.println("MQTT failed");
  }
}

String ConnectionManager::createTopic(String topicPart, bool set) {
  if (set) {
    return String(basetopic) + topicPart + "/Set";
  } else {
    return String(basetopic) + topicPart;
  }
}

void ConnectionManager::showConfigPortal() {
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqttServer, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqttPort, 6);
  WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqttUser, 20);
  WiFiManagerParameter custom_mqtt_pass("pass", "mqtt pass", mqttPass, 20);

  WiFiManager wifiManager;


  //wifiManager.setSaveConfigCallback(saveConfigCallback);


  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_pass);

  //reset settings - for testing
  //wifiManager.resetSettings();

  wifiManager.setTimeout(120);

  if (!wifiManager.startConfigPortal(hostname)) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    //ESP.reset();
    ESP.restart();
    delay(5000);
  }

  Serial.println("WiFi Connected");

  strcpy(mqttServer, custom_mqtt_server.getValue());
  strcpy(mqttPort, custom_mqtt_port.getValue());
  strcpy(mqttUser, custom_mqtt_user.getValue());
  strcpy(mqttPass, custom_mqtt_pass.getValue());

  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqttServer"] = mqttServer;
    json["mqttPort"] = mqttPort;
    json["mqttUser"] = mqttUser;
    json["mqttPass"] = mqttPass;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }
}
