#ifndef ConnectionManager_h
#define MConnectionManager_h

#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include "Arduino.h"
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <MQTTClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

class ConnectionManager {
  public:
    ConnectionManager(int pin);
    void setup();
    void setMessageCallback(MQTTClientCallbackSimple cb);
    void loop();
    void publish(String topicPart, String message);
    String createTopic(String topicPart, bool set);

    //ToDo: Remove save callback
    void setSaveCallback(void (*func)(void));
    bool shouldSaveConfig = false;


  private:
    //Settings made in WiFiManager
    char mqttServer[40];
    char mqttPort[6] = {'1', '8', '8', '3', '\0'};
    char mqttUser [20];
    char mqttPass [20];
    char baseTopic [50]= {'H', 'o', 'u', 's', 'e', '/', 'O', 'u', 't', 'd', 'o', 'o', 'r', '/', 'G', 'r', 'e', 'e', 'n', 'H', 'o', 'u', 's', 'e', '\0'};; //House/Outdoor/GreenHouse"
    char hostname [15] = {'G', 'r', 'e', 'e', 'n', 'H', 'o', 'u', 's', 'e', '\0'}; //"GreenHouse"

    int _pin;

    WiFiClient net;
    MQTTClient mqtt;
    void (*_savecallback)(void) = NULL;

    void mountFileSystem();
    void setupOta();
    void printWifiConnected();
    bool wifiConnected();
    void reconnectMqtt() ;
    void showConfigPortal();
};

#endif
