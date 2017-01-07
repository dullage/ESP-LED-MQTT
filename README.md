## Overview
Allows the WiFi control of a WS8212 LED strip using an ESP8266 controller and an MQTT broker. Designed to work well with Home Assistant https://home-assistant.io/.

To use this code simply configure the PIN and NUMPIXELS variables in the NeoPixel section. You will also need to configure the credentials for your WiFi and MQTT broker, see <REDACTED> code. Lastly you can tailor the MQTT topics as requried.

## Transmission Codes
| Code | Message |
|---|---|
| 0 | Turn off |
| 1 | Turn on |
| 2 | Toggle |   
| 9 | Mode Cycle |
| { | JSON Control |

#### JSON Example
The message below would turn all LEDs to Blue (Mode = 1, rgbColourTwo = Blue). See tables below for message options.

`{0:1,2:[0,0,255]}`

## Variables
| Code | Variable |
|---|---|
| 0 | currentMode |
| 1 | rgbVaueOne |
| 2 | rgbValueTwo |
| 3 | colourDelay |
| 4 | colourJump |
| 5 | pixelDelay |
| 6 | pixelJump |
| 7 | randomise |
| 8 | loopDelay |
| 9 | highPixelDelay |
| 10 | multiplier |
| 11 | chance |
| 12 | trailLength |

## Modes
| Code | Mode | Useable Variables |
|---|---|---|
| 0 | All Off |
| 1 | All On | rgbValueTwo |
| 2 | Fade to Colour | rgbValueTwo, colourDelay |
| 3 | Strobe | rgbValueOne, rgbValueTwo, colourDelay |
| 4 | Fire | colourDelay |
| 5 | Colour Phase | colourDelay |
| 6 | Sparkle | rgbValueOne, rgbValueTwo, colourDelay(r), colourJump(r), pixelDelay(r) |
| 7 | Shoot | rgbValueOne, rgbValueTwo, colourDelay, colourJump(r), pixelDelay, loopDelay(r), highPixelDelay, multiplier |
| 8 | Rain | rgbValueOne, rgbValueTwo, colourDelay, colourJump, chance |