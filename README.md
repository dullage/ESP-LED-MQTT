## Overview
Allows the WiFi control of a WS8212 LED strip using an ESP8266 controller and an MQTT broker.

## Transmission Codes
| Code | Direction | Message |
|---|---|---|
| 0 | To ESP | Turn off |
| 1 | To ESP | Turn on |
| 2 | To ESP | Toggle |   
| 3 | To ESP | Go publish current state request |
| 4 | From ESP | Current state broadcast Off |
| 5 | From ESP | Current state broadcast On |
| 6 | From ESP | Turn off | 
| 7 | From ESP | Turn on |
| 8 | From ESP | Toggle |
| 9 | From ESP | Special |
| { | To ESP | JSON |

#### JSON Example
`{0:1,2:[0,0,255]} = Turn all LEDs to Blue (Mode = 1, rgbColourTwo = Blue)`

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

## Modes
| Code | Mode | Useable Variables |
|---|---|
| 0 | All Off |
| 1 | All On | rgbValueTwo |
| 2 | Fade to Colour | rgbValueTwo, colourDelay |
| 3 | Strobe | rgbValueOne, rgbValueTwo, colourDelay |
| 4 | Fire | colourDelay |
| 5 | Colour Phase | colourDelay |
| 6 | Sparkle | rgbValueOne, rgbValueTwo, colourDelay(r), colourJump(r), pixelDelay(r) |
| 7 | Shoot | rgbValueOne, rgbValueTwo, colourDelay, colourJump(r), pixelDelay, loopDelay(r), highPixelDelay, multiplier |
| 8 | Rain | rgbValueOne, rgbValueTwo, colourDelay, colourJump, chance |