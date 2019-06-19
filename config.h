// Replace all the placeholders (starting with "<" and ending in ">") with the correct values

// Neopixel
#define PIN            <SET YOUR DATA PIN HERE>
#define NUMPIXELS      <SET THE NUMBER OF LIGHTS HERE>

// Wifi
char* SSID = "<SSID>";
char* WiFiPassword = "<SSID PASSWORD>";
char* WiFiHostname = "<HOSTNAME>";

// MQTT
const char* mqtt_server = "<MQTT SERVER IP ADDRESS>";
int mqtt_port = 1883;
const char* mqttUser = "<MQTT USER>";
const char* mqttPass = "<MQTT PASSWORD>";

// Device Specific Topics
const char* commandlTopic = "light/led_kitchen";
const char* stateTopic = "light/led_kitchen/state";
const char* rgbCommandTopic = "light/led_kitchen/rgb";
const char* rgbStateTopic = "light/led_kitchen/rgb/state";
const char* availabilityTopic = "light/led_kitchen/availability";
const char* recoveryTopic = "light/led_kitchen/recovery";
