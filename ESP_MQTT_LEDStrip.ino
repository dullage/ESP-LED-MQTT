#include <Arduino.h>
#include <ESP8266WiFi.h>
//#include <ESP8266WiFiMulti.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <HttpClient.h>

//ESP8266WiFiMulti wifiMulti;

// Neopixel
#define PIN            3
#define NUMPIXELS      30
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
int ledArray[NUMPIXELS][3];

// Wifi
WiFiClient espClient;
char* SSID = "<ssid>";
char* WiFiPassword = "<password>";

// MQTT
const char* mqtt_server = "<ip_address>";
int mqtt_port = 1883;
PubSubClient client(espClient);
const char* deviceTopic = "switch/ledstrip";  // ESP Specific Topic

// Gloabl Animation Variables
int currentMode; //0
const int numModes = 8; // This is the total number of modes -1 (0 start)
//const int lastAnimationVariable = 11; // This is the total number of animation variables -1 (0 start)

int rgbValueOne[3]; // 1
int rgbValueTwo[3]; // 2

unsigned long colourDelay; // 3
int colourJump; // 4

unsigned long pixelDelay; // 5
int pixelJump; // 6

boolean randomise; // 7

unsigned long loopDelay; // 8
float highPixelDelay; // 9
float multiplier; // 10
int chance; // 11

unsigned long previousMillis;
boolean flipFlop;
int currentStep;
int currentPixel;

unsigned long colourDelaySeed;
int colourJumpSeed;
unsigned long pixelDelaySeed;
unsigned long loopDelaySeed;
float highPixelDelaySeed;
float multiplierSeed;


void resetGlobalAnimationVariables() {
  currentMode = 0;

  for (int colour = 0;colour < 3;colour ++) {
    rgbValueOne[colour] = 0;
    rgbValueTwo[colour] = 255;
  }

  colourDelay = 0;
  colourJump = 1;

  pixelDelay = 0;
  pixelJump = 1;

  randomise = false;

  loopDelay = 0;
  highPixelDelay = 0;
  multiplier = 1;
  
  previousMillis = 0;
  flipFlop = 0;
  currentStep = 0;
  currentPixel = 0;

  colourDelaySeed = 0;
  colourJumpSeed = 1;
  pixelDelaySeed = 0;
  loopDelaySeed = 0;
  highPixelDelaySeed = 0;
  multiplierSeed = 0;

  chance = 100;
}

void setAnimationDefaults(int mode) {
	// All Off
		// No additional defaults

	// All On
		// No additional defaults

	// Fade To Colour
	if (mode == 2) {
		rgbValueTwo[0] = 0;
		rgbValueTwo[1] = 0;
		rgbValueTwo[2] = 255;
		colourDelay = 20;
	}

	// Strobe
	else if (mode == 3) {
		colourDelay = 40;
	}

	// Fire
	else if (mode == 4) {
		colourDelay = 20;
	}

	// Colour Phase
	else if (mode == 5) {
		colourDelay = 30;
	}

	// Sparkle
	else if (mode == 6) {
		rgbValueTwo[0] = 150;
		rgbValueTwo[1] = 150;
		rgbValueTwo[2] = 150;
		colourJump = 5;
	}

	// Shoot
	else if (mode == 7) {
		rgbValueTwo[0] = 50;
		rgbValueTwo[1] = 50;
		rgbValueTwo[2] = 255;
		colourJump = 20;
		loopDelay = 10000;
		highPixelDelay = 5;
		multiplier = 1.25;
		randomise = true;
	}

	// Mode 8
	else if (mode == 8) {
		rgbValueTwo[0] = 100;
		rgbValueTwo[1] = 100;
		rgbValueTwo[2] = 255;
		colourDelay = 4;
		chance = 15000;
	}
}

void reconnect() {
  // Reconnect to Wifi
  //if (wifiMulti.run() != WL_CONNECTED) {
	if (WiFi.status() != WL_CONNECTED) {
		Serial.println("");
		Serial.print("Waiting for WiFi connection...");
      
		//while(wifiMulti.run() != WL_CONNECTED) {
		while (WiFi.status() != WL_CONNECTED) {
			Serial.print(".");
			delay(500);
		}
      
		Serial.println("");
		Serial.println("WiFi connected!");
		Serial.print("SSID: ");
		Serial.print(WiFi.SSID());
		Serial.println("");
		Serial.print("IP address: ");
		Serial.print(WiFi.localIP());
		Serial.println("");
  }
  
  // Reconnect to MQTT Broker
  if (!client.connected()) {
	  WiFiClient wclient;
	  char* host = "192.168.0.32";
	  char* url = "/switch/unknown";

	  if (!wclient.connect(host, 80)) {
		  Serial.println("connection failed");
		  return;
	  }

	  Serial.print(String("GET ") + url + " HTTP/1.1\r\n" +
		  "Host: " + host + "\r\n" +
		  "Connection: close\r\n\r\n");

	  wclient.print(String("GET ") + url + " HTTP/1.1\r\n" +
		  "Host: " + host + "\r\n" +
		  "Connection: close\r\n\r\n");
	  
	  unsigned long timeout = millis();
	  while (wclient.available() == 0) {
		  if (millis() - timeout > 5000) {
			  Serial.println(">>> Client Timeout !");
			  wclient.stop();
			  return;
		  }
	  }

	  while (wclient.available()) {
		  String line = wclient.readStringUntil('\r');
		  Serial.print(line);
	  }

    Serial.println("");
    Serial.println("Waiting for MQTT connection...");

	// Create a random client ID
	String clientId = "ESP8266Client-";
	clientId += String(random(0xffff), HEX);

	int tryCount = 0;
    while(!client.connect(clientId.c_str())) {
        Serial.print(".");
		//Serial.println(client.state());
		
		//tryCount++;
		//if (tryCount > 8) {
		//	WiFi.disconnect;
		//}

        delay(500);
        //if (wifiMulti.run() != WL_CONNECTED) { // Check that there's still Wifi (and probably try to reconect once), return if not forcing back into the reconnection loop
		if (WiFi.status() != WL_CONNECTED) { // Check that there's still Wifi (and probably try to reconect once), return if not forcing back into the reconnection loop
          return;
        }
    }

    Serial.println("Connected!");
	Serial.println("");
    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off
    client.subscribe(deviceTopic);
    
    // Update the broker with current state
    if (currentMode == 0) { 
      client.publish(deviceTopic, "4");
    }
    else { 
      client.publish(deviceTopic, "5");
    }
  }
}

int* randomColour() {
	static int aRandomColour[3];

	aRandomColour[0] = random(255);
	aRandomColour[1] = random(255);
	aRandomColour[2] = random(255);

	return aRandomColour;
}

void allRgbValueOne() {
  if (flipFlop == 0) {
    updateLedArray_singleColour(rgbValueOne);
    updateStripFromLedArray();
  }
}

void allRgbValueTwo() {
  if (flipFlop == 0) {
    updateLedArray_singleColour(rgbValueTwo);
    updateStripFromLedArray();
  }
}

void fadeToColour() {
  if (flipFlop == 0) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= colourDelay) {
      int count = 0; // Reset the count
      for (int pixel = 0;pixel < NUMPIXELS;pixel ++){               // For every pixel
        if (ledArray[pixel][0] != rgbValueTwo[0] | ledArray[pixel][1] != rgbValueTwo[1] | ledArray[pixel][2] != rgbValueTwo[2]) {
          for (int colour = 0;colour < 3;colour++) {                    // For each colour
            if (ledArray[pixel][colour] < rgbValueTwo[colour]) {          // If the colour value is less that it should be
              ledArray[pixel][colour] += 1;                                 // Add 1
            }
            else if (ledArray[pixel][colour] > rgbValueTwo[colour]) {     // If the colour value is more that it should be
              ledArray[pixel][colour] -= 1;                                 // Subtract 1
            }
          }
          pixels.setPixelColor(pixel, pixels.Color(ledArray[pixel][0],ledArray[pixel][1],ledArray[pixel][2]));
        }
        else {
          count ++;
        }
      }
      pixels.show();
  
      if (count == NUMPIXELS) {
        flipFlop = 1;
      }
      
      previousMillis = currentMillis;
    }
  }
}

void strobe() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= colourDelay) {
    if (flipFlop == 1) {
      updateLedArray_singleColour(rgbValueTwo);
      updateStripFromLedArray();
      flipFlop = !flipFlop;
    }
    else if (flipFlop == 0) {
      updateLedArray_singleColour(rgbValueOne);
      updateStripFromLedArray();
      flipFlop = !flipFlop;
    }
    previousMillis = currentMillis;
  }
}

void fire(int Cooling, int Sparking, int SpeedDelay) { // Need to tailor variables and update LED array
  static byte heat[NUMPIXELS];
  int cooldown;
  
  // Step 1.  Cool down every cell a little
  for( int i = 0; i < NUMPIXELS; i++) {
    cooldown = random(0, ((Cooling * 10) / NUMPIXELS) + 2);
    
    if(cooldown>heat[i]) {
      heat[i]=0;
    } else {
      heat[i]=heat[i]-cooldown;
    }
  }
  
  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for( int k= NUMPIXELS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }
    
  // Step 3.  Randomly ignite new 'sparks' near the bottom
  if( random(255) < Sparking ) {
    int y = random(7);
    heat[y] = heat[y] + random(160,255);
    //heat[y] = random(160,255);
  }

  // Step 4.  Convert heat to LED colors
  for( int j = 0; j < NUMPIXELS; j++) {
    setPixelHeatColor(j, heat[j] );
  }

  pixels.show();
  delay(SpeedDelay);
}
void setPixelHeatColor (int Pixel, byte temperature) {
  // Scale 'heat' down from 0-255 to 0-191
  byte t192 = round((temperature/255.0)*191);
 
  // calculate ramp up from
  byte heatramp = t192 & 0x3F; // 0..63
  heatramp <<= 2; // scale up to 0..252
 
  // figure out which third of the spectrum we're in:
  if( t192 > 0x80) {                     // hottest
    pixels.setPixelColor(Pixel, 255, 255, heatramp);
  } else if( t192 > 0x40 ) {             // middle
    pixels.setPixelColor(Pixel, 255, heatramp, 0);
  } else {                               // coolest
    pixels.setPixelColor(Pixel, heatramp, 0, 0);
  }
}

void colourPhase() {
  if (flipFlop == 1) { // fadeToColour will set flipFlop to 1 when complete so we use this to update the current step then reset the flipFlop
    currentStep++;
    flipFlop = 0;
    if (currentStep > 5) { // If the current step is greater than the number of steps we reset
      currentStep = 0;
    }
  }
  
  if (currentStep == 0) {
    //Serial.println("0");
    rgbValueTwo[0] = 255;
    rgbValueTwo[1] = 0;
    rgbValueTwo[2] = 0;
    fadeToColour();
  }
  else if (currentStep == 1) {
    //Serial.println("1");
    rgbValueTwo[0] = 255;
    rgbValueTwo[1] = 255;
    rgbValueTwo[2] = 0;
    fadeToColour();
  }
  else if (currentStep == 2) {
    //Serial.println("2");
    rgbValueTwo[0] = 0;
    rgbValueTwo[1] = 255;
    rgbValueTwo[2] = 0;
    fadeToColour();
  }
  else if (currentStep == 3) {
    rgbValueTwo[0] = 0;
    rgbValueTwo[1] = 255;
    rgbValueTwo[2] = 255;
    fadeToColour();
  }
  else if (currentStep == 4) {
    rgbValueTwo[0] = 0;
    rgbValueTwo[1] = 0;
    rgbValueTwo[2] = 255;
    fadeToColour();
  }
  else if (currentStep == 5) {
    rgbValueTwo[0] = 255;
    rgbValueTwo[1] = 255;
    rgbValueTwo[2] = 255;
    fadeToColour();
  }
}

void sparkle() { 
  unsigned long currentMillis = millis();
  
  // Initiator
  if (currentStep == 0) { 
    allRgbValueOne(); // Sets all LEDs to rgbValueOne
    colourDelaySeed = colourDelay;
    colourJumpSeed = colourJump;
    pixelDelaySeed = pixelDelay;
    currentStep ++;
  }

  // Pixel Delay
  if (currentStep == 1) {
    if (currentMillis - previousMillis >= pixelDelay) {
      if (randomise == true) {
        pixelDelay = random(pixelDelaySeed);
        colourDelay = random(colourDelaySeed); // Only change between pixels so the light doesn't waver
        colourJump = random(1,colourJumpSeed); // Min of 1 otherwise there's a chance the loop will stop
      }
      previousMillis = currentMillis;
      currentStep ++;
    }
  }

  // Roll Up
  if (currentStep == 2 && (currentMillis - previousMillis >= colourDelay )) {
    if (ledArray[currentPixel][0] != rgbValueTwo[0] | ledArray[currentPixel][1] != rgbValueTwo[1] | ledArray[currentPixel][2] != rgbValueTwo[2]) {   // If the pixel hasn't reached peak
      for (int i = 1;!(i > colourJump);i++) {
        for (int col = 0;col < 3;col++) {                                                                         // For each colour
          if (ledArray[currentPixel][col] < rgbValueTwo[col]) {                                                                      // If the colour value is less that it should be
            ledArray[currentPixel][col] += 1;                                                                                  // Add 1
          }
          else if (ledArray[currentPixel][col] > rgbValueTwo[col]) {                                                               // If the colour value is more that it should be
            ledArray[currentPixel][col] -= 1;                                                                                // Subtract 1
          }
        }
      }
      updateStripFromLedArray();
    }
    else {
      currentStep ++;
    }
  previousMillis = currentMillis;
  }

  // Roll Down
  if (currentStep == 3 && (currentMillis - previousMillis >= colourDelay )) {
    if (ledArray[currentPixel][0] != rgbValueOne[0] | ledArray[currentPixel][1] != rgbValueOne[1] | ledArray[currentPixel][2] != rgbValueOne[2]) {   // If the pixel hasn't reached base
      for (int i = 1;!(i > colourJump);i++) {
        for (int col = 0;col < 3;col++) {                                                                         // For each colour
          if (ledArray[currentPixel][col] < rgbValueOne[col]) {                                                                      // If the colour value is less that it should be
            ledArray[currentPixel][col] += 1;                                                                                  // Add 1
          }
          else if (ledArray[currentPixel][col] > rgbValueOne[col]) {                                                               // If the colour value is more that it should be
            ledArray[currentPixel][col] -= 1;                                                                                // Subtract 1
          }
        }
      }
      updateStripFromLedArray();
    }
    else {
      currentStep = 1; // Go back to the mid pixel wait
      currentPixel = random(NUMPIXELS);
    }
  previousMillis = currentMillis;
  }
}

void rain() {
  unsigned long currentMillis = millis();

  if (random(chance) == 1) {
    int randomPixel = random(NUMPIXELS);
    ledArray[randomPixel][0] = rgbValueTwo[0];
    ledArray[randomPixel][1] = rgbValueTwo[1];
    ledArray[randomPixel][2] = rgbValueTwo[2];
    updateStripFromLedArray();
  }

  if (currentMillis - previousMillis >= colourDelay) {
    for (int pixel = 0;pixel < NUMPIXELS;pixel ++){               // For every pixel
      for (int i = 1;!(i > colourJump);i++) {
        if (ledArray[pixel][0] != rgbValueOne[0] | ledArray[pixel][1] != rgbValueOne[1] | ledArray[pixel][2] != rgbValueOne[2]) {
          for (int colour = 0;colour < 3;colour++) {                    // For each colour
            if (ledArray[pixel][colour] < rgbValueOne[colour]) {          // If the colour value is less that it should be
              ledArray[pixel][colour] += 1;                                 // Add 1
            }
            else if (ledArray[pixel][colour] > rgbValueOne[colour]) {     // If the colour value is more that it should be
              ledArray[pixel][colour] -= 1;                                 // Subtract 1
            }
          }
          pixels.setPixelColor(pixel, pixels.Color(ledArray[pixel][0],ledArray[pixel][1],ledArray[pixel][2]));
        }
      }
    }
    updateStripFromLedArray();
    previousMillis = currentMillis;
  }
}

void shoot() { 
  unsigned long currentMillis = millis();
  
  // Initiator
  if (currentStep == 0) { 
    allRgbValueOne(); // Sets all LEDs to rgbValueOne
    loopDelaySeed = loopDelay;
    colourJumpSeed = colourJump;
    multiplierSeed = multiplier;
    highPixelDelaySeed = highPixelDelay; // Seeded because it's multiplied not because it's randomised
    currentStep ++;
  }

  // Loop Delay
  if (currentStep == 1) {
    if (currentMillis - previousMillis >= loopDelay) {
      if (randomise == true) {
        loopDelay = random(loopDelaySeed);
        colourJump = random(1,colourJumpSeed);
        float multiplierSeedX = (multiplier * 100.00);
        multiplier = (random(multiplierSeedX * 0.95,multiplierSeedX * 1.05)) / 100.00; // +-5% - Best at 1.25
      }
      highPixelDelay = highPixelDelaySeed;
      previousMillis = currentMillis;
      currentStep ++;
    }
  }

  // Pixel Delay
  if (currentStep == 2) {
    if (currentMillis - previousMillis >= pixelDelay) {
      previousMillis = currentMillis;
      currentStep ++;
    }
  }

  // Roll Up
  if (currentStep == 3 && (currentMillis - previousMillis >= colourDelay )) {
    if (ledArray[currentPixel][0] != rgbValueTwo[0] | ledArray[currentPixel][1] != rgbValueTwo[1] | ledArray[currentPixel][2] != rgbValueTwo[2]) {   // If the pixel hasn't reached peak
      for (int i = 1;!(i > colourJump);i++) {
        for (int col = 0;col < 3;col++) {                                                                         // For each colour
          if (ledArray[currentPixel][col] < rgbValueTwo[col]) {                                                                      // If the colour value is less that it should be
            ledArray[currentPixel][col] += 1;                                                                                  // Add 1
          }
          else if (ledArray[currentPixel][col] > rgbValueTwo[col]) {                                                               // If the colour value is more that it should be
            ledArray[currentPixel][col] -= 1;                                                                                // Subtract 1
          }
        }
      }
      updateStripFromLedArray();
    }
    else {
      currentStep ++;
    }
  previousMillis = currentMillis;
  }

  // High Pixel Delay
  if (currentStep == 4) {
    if (currentMillis - previousMillis >= highPixelDelay) {
      previousMillis = currentMillis;
      highPixelDelay = (highPixelDelay * multiplier);
      currentStep ++;
    }
  }

  // Roll Down
  if (currentStep == 5 && (currentMillis - previousMillis >= colourDelay )) {
    if (ledArray[currentPixel][0] != rgbValueOne[0] | ledArray[currentPixel][1] != rgbValueOne[1] | ledArray[currentPixel][2] != rgbValueOne[2]) {   // If the pixel hasn't reached base
      for (int i = 1;!(i > colourJump);i++) {
        for (int col = 0;col < 3;col++) {                                                                         // For each colour
          if (ledArray[currentPixel][col] < rgbValueOne[col]) {                                                                      // If the colour value is less that it should be
            ledArray[currentPixel][col] += 1;                                                                                  // Add 1
          }
          else if (ledArray[currentPixel][col] > rgbValueOne[col]) {                                                               // If the colour value is more that it should be
            ledArray[currentPixel][col] -= 1;                                                                                // Subtract 1
          }
        }
      }
      updateStripFromLedArray();
    }
    else {
      if (currentPixel == NUMPIXELS - 1) {
        currentPixel = 0;
        currentStep = 1;
      }
      else {
        currentPixel ++;
        currentStep = 2;
      }
    }
  previousMillis = currentMillis;
  }
}

void updateLedArray_singleColour(int targetColour[3]) {
  for (int i = 0;i < NUMPIXELS;i ++) {
    ledArray[i][0] = targetColour[0];
    ledArray[i][1] = targetColour[1];
    ledArray[i][2] = targetColour[2];
  }
}

void updateStripFromLedArray() {
  for (int i = 0;i < NUMPIXELS;i ++){
    pixels.setPixelColor(i, pixels.Color(ledArray[i][0],ledArray[i][1],ledArray[i][2]));
  }
  pixels.show();
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on to indicate no connection
  
  Serial.begin(115200);

  pixels.begin(); // This initializes the NeoPixel library.
  pixels.setBrightness(255);
  pixels.show();

  //wifiMulti.addAP("SKYA6DDD_EXT", "TXDBATRT");
  //wifiMulti.addAP("SKYA6DDD", "TXDBATRT");
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, WiFiPassword);
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // Initialise the custom ledArray library
  for (int i = 0;i < NUMPIXELS;i ++) {
    ledArray[i][0] = 0;
    ledArray[i][1] = 0;
    ledArray[i][2] = 0;
  }

  resetGlobalAnimationVariables();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("------------------- MQTT Recieved -------------------");

  if (length == 1) {
    // Off
    if ((char)payload[0] == '0') {
      resetGlobalAnimationVariables();
    }
    // On
    else if ((char)payload[0] == '1') {
      resetGlobalAnimationVariables();
      currentMode = 1;
    }
    // Toggle
    else if ((char)payload[0] == '2') { 
      if (currentMode == 0) {
        resetGlobalAnimationVariables();
        currentMode = 1;
      }
      else {
        resetGlobalAnimationVariables();
      }
    }
    // State check
    else if ((char)payload[0] == '3') { 
      if (currentMode == 0) { 
        client.publish(deviceTopic, "4");
      }
      else { 
        client.publish(deviceTopic, "5");
      }
    }
  // Special
    else if ((char)payload[0] == '9') {
		int lastMode = currentMode;

		resetGlobalAnimationVariables();
		
		currentMode = ++lastMode;

		if (currentMode > numModes) {
			currentMode = 0;
		}

		setAnimationDefaults(currentMode);
    }
  }
  else if ((char)payload[0] == '{') {     // It's a JSON
    char message_buff[200];               // Max of 200 characters
    for(int i = 0; i < length; i++) {
      message_buff[i] = payload[i]; 
      if (i == (length - 1)) {            // If we're at the last character
        message_buff[i + 1] = '\0';       // Add a string terminator to the next character
      }
    }
    StaticJsonBuffer<300> jsonBuffer;     // Step 1: Reserve memory space (https://bblanchon.github.io/ArduinoJson/)
    JsonObject& root = jsonBuffer.parseObject(message_buff); // Step 2: Deserialize the JSON string
    
    if (!root.success()) {
      Serial.println("parseObject() failed");
    }
    else if (root.success()) {
      Serial.println("Root Success");

      resetGlobalAnimationVariables();

      if (root.containsKey("0")) {
        currentMode = root["0"];
      }
	  if (currentMode > numModes) { // If mode is unknown, set to 0
		  currentMode = 0;
	  }

      if (root.containsKey("1")) {
        rgbValueOne[0] = root["1"][0];
        rgbValueOne[1] = root["1"][1];
        rgbValueOne[2] = root["1"][2];
      }

      if (root.containsKey("2")) {
        rgbValueTwo[0] = root["2"][0];
        rgbValueTwo[1] = root["2"][1];
        rgbValueTwo[2] = root["2"][2];
      }

      if (root.containsKey("3")) {
        colourDelay = root["3"];
      }

      if (root.containsKey("4")) {
        colourJump = root["4"];
      }

      if (root.containsKey("5")) {
        pixelDelay = root["5"];
      }

      if (root.containsKey("6")) {
        pixelJump = root["6"];
      }

      if (root.containsKey("7")) {
        randomise = root["7"];
      }

      if (root.containsKey("8")) {
        loopDelay = root["8"];
      }

      if (root.containsKey("9")) {
        highPixelDelay = root["9"];
      }

      if (root.containsKey("10")) {
        multiplier = root["10"];
      }

      if (root.containsKey("11")) {
        chance = root["11"];
      }

	  setAnimationDefaults(currentMode);
      
    }
  }
}

void loop() {
  if (!client.connected()) {
    digitalWrite(LED_BUILTIN, LOW);  // Turn the LED on
    reconnect();
  }
  client.loop();

  if (currentMode == 0) {
    allRgbValueOne();
  }
  else if (currentMode == 1) {
    allRgbValueTwo();
  }
  else if (currentMode == 2) {
    fadeToColour();
  }
  else if (currentMode == 3) {
    strobe();
  }
  else if (currentMode == 4) {
    fire(55,120,colourDelay);
  }
  else if (currentMode == 5) {
    colourPhase();
  }
  else if (currentMode == 6) {
    sparkle();
  }
  else if (currentMode == 7) {
    shoot();
  }
  else if (currentMode == 8) {
    rain();
  }
}
