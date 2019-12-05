// WiFi
#define WIFI_SSID "***"
#define WIFI_PASSWORD "***"
#define WIFI_HOSTNAME "RemoteControl"

// MQTT
#define MQTT_HOST "1.2.3.4"
#define MQTT_PORT 1883
#define MQTT_CLIENT_ID "RemoteControl"
#define MQTT_USERNAME "***"
#define MQTT_PASSWORD "***"
#define MQTT_CONNECTED_TOPIC "MyHome/Indoor/MyRoom/RemoteControl/Connected"
#define MQTT_CONNECTED_MESSAGE "true"
#define MQTT_LAST_WILL_TOPIC "MyHome/Indoor/MyRoom/RemoteControl/Connected"
#define MQTT_LAST_WILL_MESSAGE "false"
#define MQTT_STATUS_REQUEST_TOPIC "MyHome/Indoor/MyRoom/RemoteControl/TechnicalStatusRequest"
#define MQTT_STATUS_RESPONSE_TOPIC "MyHome/Indoor/MyRoom/RemoteControl/TechnicalStatus"

typedef struct {
  int pin;
  String topic;
  String message;
  bool lastState;
  int lastDebounceTime;
  bool state;
} button;

button buttons[2] = {
  {
    D6,
    "MyHome/Scene",
    "cook-dinner"
  },
  {
    D7,
    "MyHome/Scene",
    "dinner"
  }
};

// Debug
#define SERIAL_BAUDRATE 115200
#define DEBUG_PRINT_SERIAL true
