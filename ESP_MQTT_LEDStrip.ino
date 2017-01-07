#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

// Neopixel
#define PIN            3
#define NUMPIXELS      27 //112
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
int ledArray[NUMPIXELS][3]; // Custom array to allow us to buffer changes before sending them

// Wifi
WiFiClient espClient;
char* SSID = "<REDACTED>";
char* WiFiPassword = "<REDACTED>";

// MQTT
const char* mqtt_server = "<REDACTED>";
int mqtt_port = 1883;
const char* mqttUser = "<REDACTED>";
const char* mqttPass = "<REDACTED>";
PubSubClient client(espClient);

const uint8_t bufferSize = 20;
char buffer[bufferSize];

// ESP Specific Topics
const char* deviceStateTopic = "switch/stairlamp/state";
const char* deviceControlTopic = "switch/stairlamp";   
const char* rgbStateTopic = "switch/stairlamp/rgb/state"; 
const char* rgbControlTopic = "switch/stairlamp/rgb"; 
//const char* brightnessStateTopic = "switch/stairlamp/brightness/state"; - Not yet implemented
//const char* brightnessControlTopic = "switch/stairlamp/brightness"; - Not yet implemented

// Sun Position
const char* sunPositionTopic = "sunPosition"; // This will be sent as a retained message so will be updated upon boot
int sunPosition = 0;

// Global Animation Variables
#pragma region Global Animation Variables
int currentMode; // 0
const int numModes = 9; // This is the total number of modes

int brightness = 255;

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

int trailLength; // 12

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
#pragma endregion

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

  trailLength = 0;

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
	if (mode == 1) {
		rgbValueTwo[0] = 255;
		rgbValueTwo[1] = 147;
		rgbValueTwo[2] = 41;
		colourDelay = 2;
	}

	// Fade To Colour
	else if (mode == 2) {
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
		multiplier = 1.15;
		randomise = true;
		trailLength = 7;
	}

	// Mode 8
	else if (mode == 8) {
		rgbValueTwo[0] = 100;
		rgbValueTwo[1] = 100;
		rgbValueTwo[2] = 255;
		colourDelay = 4;
		chance = 15000;
	}

	// Mode 9
	if (mode == 9) {
		rgbValueTwo[0] = 255;
		rgbValueTwo[1] = 147;
		rgbValueTwo[2] = 41;
	}
}

void reconnect() {
  // Reconnect to Wifi
	if (WiFi.status() != WL_CONNECTED) {
		Serial.println("");
		Serial.print("Waiting for WiFi connection...");
      
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
    Serial.println("");
    Serial.println("Waiting for MQTT connection...");

	// Create a random client ID
	String clientId = "ESP8266Client-";
	clientId += String(random(0xffff), HEX);

	int tryCount = 0;
    while(!client.connect(clientId.c_str(), mqttUser, mqttPass)) {
        Serial.print(".");

		if (WiFi.status() != WL_CONNECTED) { // Check that there's still Wifi, return if not forcing back into the reconnection loop
          return;
        }

		tryCount++;
		if (tryCount > 8) {
			ESP.restart(); // Restart the ESP upon failing 8 times
		}

		delay(500);
    }

    Serial.println("Connected!");
	Serial.println("");

    digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off

	// MQTT Subscriptions
    client.subscribe(deviceControlTopic);
	client.subscribe(rgbControlTopic);
	//client.subscribe(brightnessControlTopic); - Not yet implemented
	client.subscribe(sunPositionTopic);
  }
}

#pragma region MiscFunctions
int* randomColour() {
	static int aRandomColour[3];

	aRandomColour[0] = random(255);
	aRandomColour[1] = random(255);
	aRandomColour[2] = random(255);

	return aRandomColour;
}

void updateLedArray_singleColour(int targetColour[3]) {
	for (int i = 0; i < NUMPIXELS; i++) {
		ledArray[i][0] = targetColour[0];
		ledArray[i][1] = targetColour[1];
		ledArray[i][2] = targetColour[2];
	}
}

void updateStripFromLedArray() {
	for (int i = 0; i < NUMPIXELS; i++) {
		pixels.setPixelColor(i, pixels.Color(ledArray[i][0], ledArray[i][1], ledArray[i][2]));
	}
	pixels.show();
}

void publishState() {
	if (currentMode == 0) {
		client.publish(deviceControlTopic, "4");
		client.publish(deviceStateTopic, "0");
	}
	else {
		client.publish(deviceControlTopic, "5");
		client.publish(deviceStateTopic, "1");
	}
}

void publishColour() {
	snprintf(buffer, bufferSize, "%d,%d,%d", rgbValueTwo[0], rgbValueTwo[1], rgbValueTwo[2]);
	client.publish(rgbStateTopic, buffer);
}

//void publishBrightness() { - Not yet implemented
//	char brightnessChar[3];
//	String(brightness).toCharArray(brightnessChar, 3);
//	client.publish(brightnessStateTopic, brightnessChar);
//}
#pragma endregion

#pragma region Animations
void allRgbValueOne() {
  if (flipFlop == 0) { // This isn't used
    updateLedArray_singleColour(rgbValueOne);
    updateStripFromLedArray();
  }
}

void allRgbValueTwo() {
  if (flipFlop == 0) { // This isn't used
    updateLedArray_singleColour(rgbValueTwo);
    updateStripFromLedArray();
  }
}

void stairsOn() {
	if (flipFlop == 0) { // This isn't used
		updateLedArray_singleColour(rgbValueOne);

		for (int i = 0; i < 3; i++) {
			ledArray[3][i] = rgbValueTwo[i];
			ledArray[12][i] = rgbValueTwo[i];
			ledArray[21][i] = rgbValueTwo[i];
			ledArray[30][i] = rgbValueTwo[i];
			ledArray[40][i] = rgbValueTwo[i];
			ledArray[49][i] = rgbValueTwo[i];
			ledArray[58][i] = rgbValueTwo[i];
			ledArray[68][i] = rgbValueTwo[i];
			ledArray[77][i] = rgbValueTwo[i];
			ledArray[86][i] = rgbValueTwo[i];
			ledArray[96][i] = rgbValueTwo[i];
			ledArray[105][i] = rgbValueTwo[i];
			ledArray[111][i] = rgbValueTwo[i];
		}
		updateStripFromLedArray();
	}
}

void stairsOff() {
	if (flipFlop == 0) { // This isn't used
		updateLedArray_singleColour(rgbValueOne);

		if (sunPosition == 0) {
			rgbValueTwo[0] = 2;
			rgbValueTwo[1] = 1;
			rgbValueTwo[2] = 0;
		}
		else if (sunPosition == 1) {
			rgbValueTwo[0] = 0;
			rgbValueTwo[1] = 0;
			rgbValueTwo[2] = 0;
		}

		for (int i = 0; i < 3; i++) {
			ledArray[0][i] = rgbValueTwo[i];
			ledArray[9][i] = rgbValueTwo[i];
			ledArray[18][i] = rgbValueTwo[i];
			ledArray[27][i] = rgbValueTwo[i];
			ledArray[37][i] = rgbValueTwo[i];
			ledArray[46][i] = rgbValueTwo[i];
			ledArray[55][i] = rgbValueTwo[i];
			ledArray[65][i] = rgbValueTwo[i];
			ledArray[74][i] = rgbValueTwo[i];
			ledArray[83][i] = rgbValueTwo[i];
			ledArray[93][i] = rgbValueTwo[i];
			ledArray[102][i] = rgbValueTwo[i];
			ledArray[111][i] = rgbValueTwo[i];
		}
		updateStripFromLedArray();
	}
}

void fadeToColour() {
  if (flipFlop == 0) { // This isn't used
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
  
      //if (count == NUMPIXELS) {
      //  flipFlop = 1;
      //}
      
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
		currentStep++;
	}

	// Loop Delay
	if (currentStep == 1) {
		if (currentMillis - previousMillis >= loopDelay) {
			if (randomise == true) {
				loopDelay = random(loopDelaySeed);
				colourJump = random(1, colourJumpSeed);
				float multiplierSeedX = (multiplier * 100.00);
				multiplier = (random(multiplierSeedX * 0.95, multiplierSeedX * 1.05)) / 100.00; // +-5% - Best at 1.25
			}
			highPixelDelay = highPixelDelaySeed;
			previousMillis = currentMillis;
			currentStep++;
		}
	}

	// Pixel Delay
	if (currentStep == 2) {
		if (currentMillis - previousMillis >= pixelDelay) {
			previousMillis = currentMillis;
			currentStep++;
		}
	}

	// Roll Up
	if (currentPixel > (NUMPIXELS - 1)) { // Skip the roll up if we're after the last pixel (trail)
		currentStep = 4;
	}

	if (currentStep == 3 && (currentMillis - previousMillis >= colourDelay)) {
		if (ledArray[currentPixel][0] != rgbValueTwo[0] | ledArray[currentPixel][1] != rgbValueTwo[1] | ledArray[currentPixel][2] != rgbValueTwo[2]) {   // If the pixel hasn't reached peak
			for (int i = 1; !(i > colourJump); i++) {
				for (int col = 0; col < 3; col++) {                                                                         // For each colour
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
			currentStep++;
		}
		previousMillis = currentMillis;
	}

	// High Pixel Delay
	if (currentStep == 4) {
		if (currentMillis - previousMillis >= highPixelDelay) {
			previousMillis = currentMillis;
			highPixelDelay = (highPixelDelay * multiplier);
			currentStep++;
		}
	}

	// Roll Down
	if (currentStep == 5 && (currentMillis - previousMillis >= colourDelay)) {
	  if (trailLength == 0) {
		if (ledArray[currentPixel][0] != rgbValueOne[0] | ledArray[currentPixel][1] != rgbValueOne[1] | ledArray[currentPixel][2] != rgbValueOne[2]) {   // If the pixel hasn't reached base
			for (int i = 1; !(i > colourJump); i++) {
				for (int col = 0; col < 3; col++) {                                                                         // For each colour
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
			if (currentPixel == (NUMPIXELS - 1)) {
				currentPixel = 0;
				currentStep = 1;
			}
			else {
				currentPixel++;
				currentStep = 2;
			}
		}
	}
	else {
		Serial.println("---");
		Serial.print("currentPixel = ");
		Serial.println(currentPixel);
		
		int currentTrailPixel = currentPixel; // Start with the current pixel as we're about to put the next one up

		while (currentTrailPixel >= (currentPixel - (trailLength + 1))) {

			if (currentTrailPixel >= 0 && currentTrailPixel < NUMPIXELS) {
				Serial.print("trailPixelToUpdate = ");
				Serial.println(currentTrailPixel);

				if (currentTrailPixel > (currentPixel - trailLength)) { // This is the trail
					int trailDifference = (currentPixel - currentTrailPixel);
					float trailPercentage = ((1.0 / trailLength) * (trailLength - trailDifference));
					Serial.print("trailPercentage = ");
					Serial.println(trailPercentage);
					
					ledArray[currentTrailPixel][0] = (rgbValueTwo[0] * trailPercentage);
					ledArray[currentTrailPixel][1] = (rgbValueTwo[1] * trailPercentage);
					ledArray[currentTrailPixel][2] = (rgbValueTwo[2] * trailPercentage);
				}
				else { // Clear the trail
					ledArray[currentTrailPixel][0] = rgbValueOne[0];
					ledArray[currentTrailPixel][1] = rgbValueOne[1];
					ledArray[currentTrailPixel][2] = rgbValueOne[2];
				}
			}

			currentTrailPixel = (currentTrailPixel - 1);
		}

		updateStripFromLedArray();
		
		if (currentPixel == ((NUMPIXELS - 1) + trailLength)) {
			currentPixel = 0;
			currentStep = 1;
		}
		else {
			currentPixel++;
			currentStep = 2;
		}
	}
  previousMillis = currentMillis;
  }
}
#pragma endregion

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on to indicate no connection
  
  Serial.begin(115200);

  pixels.begin(); // This initializes the NeoPixel library.
  pixels.setBrightness(brightness);
  //publishBrightness(); - Not yet implemented
  pixels.show();

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

  // Set all global animation variables to their defaults
  resetGlobalAnimationVariables();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("------------------- MQTT Recieved -------------------");

#pragma region deviceControlTopic
  if (String(deviceControlTopic).equals(topic)) {
	  if (length == 1) {
		  // Off
		  if ((char)payload[0] == '0') {
			  resetGlobalAnimationVariables();
			  currentMode = 0; // Not required, for clarity
			  setAnimationDefaults(currentMode);
			  publishState();
		  }
		  // On
		  else if ((char)payload[0] == '1') {
			  int previousMode = currentMode;
			  if (previousMode == 0) {
				  resetGlobalAnimationVariables();
			  }
			  
			  currentMode = 1;
			  
			  if (previousMode == 0) {
				  setAnimationDefaults(currentMode);
			  }

			  publishState();
		  }
		  // Toggle - Using a re-publish ensures any changes to the On / Off routines are used for the toggle as well
		  else if ((char)payload[0] == '2') {
			  if (currentMode == 0) {
				  client.publish(deviceControlTopic, "1");
			  }
			  else {
				  client.publish(deviceControlTopic, "0"); 
			  }
		  }
		  // Cycle Modes
		  else if ((char)payload[0] == '9') {
			  int lastMode = currentMode;

			  resetGlobalAnimationVariables();

			  currentMode = ++lastMode;

			  if (currentMode > numModes) {
				  currentMode = 0;
			  }

			  setAnimationDefaults(currentMode);
			  publishState();
		  }
	  }

	  else if ((char)payload[0] == '{') {     // It's a JSON
		  char message_buff[200];               // Max of 200 characters
		  for (int i = 0; i < length; i++) {
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

			  if (root.containsKey("0")) {
				  resetGlobalAnimationVariables(); // Only do this if the mode changes
				  currentMode = root["0"];
				  setAnimationDefaults(currentMode); // Only do this if the mode changes
				  publishState();
				  publishColour();
			  }
			  if (currentMode > numModes) { // If mode is greater than is possible, set to 0
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
				  publishColour();
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

			  if (root.containsKey("12")) {
				  trailLength = root["12"];
			  }

		  }
	  }
  }
#pragma endregion

#pragma region rgbControlTopic
	else if (String(rgbControlTopic).equals(topic)) {
		String payloadString;
		for (uint8_t i = 0; i < length; i++) {
			payloadString.concat((char)payload[i]);
		}

		uint8_t firstIndex = payloadString.indexOf(',');
		uint8_t lastIndex = payloadString.lastIndexOf(',');

		uint8_t rgb_red = payloadString.substring(0, firstIndex).toInt();
		if (rgb_red < 0 || rgb_red > 255) {
			return;
		}
		else {
			rgbValueTwo[0] = rgb_red;
		}

		uint8_t rgb_green = payloadString.substring(firstIndex + 1, lastIndex).toInt();
		if (rgb_green < 0 || rgb_green > 255) {
			return;
		}
		else {
			rgbValueTwo[1] = rgb_green;
		}

		uint8_t rgb_blue = payloadString.substring(lastIndex + 1).toInt();
		if (rgb_blue < 0 || rgb_blue > 255) {
			return;
		}
		else {
			rgbValueTwo[2] = rgb_blue;
		}

		publishColour();
	}
#pragma endregion

#pragma region brightnessControlTopic
	//else if (String(brightnessControlTopic).equals(topic)) {
	//	char message_buff[3];
	//	int i = 0;
	//	for (i; i < length; i++) {
	//		message_buff[i] = payload[i];
	//	}
	//	message_buff[i] = '\0';

	//	if ((int)message_buff < 0 || (int)message_buff > 255) {
	//		return;
	//	}
	//	else {
	//		brightness = (int)message_buff;
	//		pixels.setBrightness(brightness);
	//		pixels.show();
	//		publishBrightness();
	//	}
	//}
#pragma endregion

#pragma region sunPositionTopic
	else if (String(sunPositionTopic).equals(topic)) {
		if ((char)payload[0] == '0') {
			sunPosition = 0;
		}
		else if ((char)payload[0] == '1') {
			sunPosition = 1;
		}
	}
#pragma endregion

}

void loop() {
  if (!client.connected()) {
    digitalWrite(LED_BUILTIN, LOW);  // Turn the LED on
    reconnect();
  }
  client.loop();

  if (currentMode == 0) {
	if (String(deviceControlTopic).equals("switch/stairs")) { // Special On/Off Modes for Stairs
		stairsOff();
	}
	else {
		allRgbValueOne(); // Normal
	}
  }
  else if (currentMode == 1) {
	fadeToColour();
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
  else if (currentMode == 9) {
	stairsOn();
  }
}
