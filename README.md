## Overview
Allows the WiFi control of a WS8212 LED strip using an ESP8266 controller and an MQTT broker. Designed to work well with Home Assistant https://home-assistant.io/.

To use this code simply configure the PIN and NUMPIXELS variables in the NeoPixel section. You will also need to configure the credentials for your WiFi and MQTT broker, see <REDACTED> code. Lastly you can tailor the MQTT topics as requried.

## Features
* Works well with Home Assistant.
* Includes 7 animations.
* Basic control using simple MQTT commands (e.g. "1" to turn on, "0" to turn off).
* Advanced control using JSON MQTT commands.
* Tweak animation variables without needing to alter code.
* Utilizes a non-blocking reconnect function allowing animations to continue if MQTT or WiFi connection is lost.
* Automatic Recovery - All varables are published as a retained message to a recovery topic. If the ESP is restarted this message is used to return to the previous state.
* Retained Statuses - The state is published as a retained message allowing your hub (e.g. Home Assistant) to grab the current state if it is restarted.

## Dependencies
The following libraries are required and so must be present when compiling.

* Arduino
* ESP8266WiFi
* PubSubClient
* Adafruit_NeoPixel
* ArduinoJson

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
| 1 | rgbValueOne |
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
| 9 | Stairs On | rgbValueOne, rgbValueTwo |
| 10 | Stair Shutdown | |
| 11 | Stair Startup | rgbValueTwo |

## Home Assistant Examples
**light:**
Basic control of light.
```
- platform: mqtt
  name: Lamp
  state_topic: "switch/lamp/state"
  command_topic: "switch/lamp"
  rgb_state_topic: "switch/lamp/rgb/state"
  rgb_command_topic: "switch/lamp/rgb"
  payload_off: "0"
  payload_on: "1"
  retain: false
```

**input_select:**
UI to allow user to trigger an animation.
```
lamp:
    name: Lamp
    options:
      - "Select..."
      - "Warm Light"
      - "Strobe"
      - "Fire"
      - "Colour Phase"
      - "Sparkle"
      - "Shoot / Drip"
      - "Rain"
      - "Stepped"
      - "Startup"
      - "Shutdown"
    initial: "Select..."
```

**automation:**
Sends the appropriate MQTT command when a mode is selected and then returns the input select back to 'Select...'
```
- alias: "Lamp Mode"
  trigger:
    platform: state
    entity_id: input_select.lamp       
  action:
    - service: mqtt.publish
      data_template:
        topic: "switch/stairs"
        payload: >
          {% if trigger.to_state.state == 'Warm Light' %}
            {0:2,2:[255,147,41]}
          {% elif trigger.to_state.state == 'Strobe' %}
            {0:3}
          {% elif trigger.to_state.state == 'Fire' %}
            {0:4}
          {% elif trigger.to_state.state == 'Colour Phase' %}
            {0:5,3:15}
          {% elif trigger.to_state.state == 'Sparkle' %}
            {0:6,2:[50,50,50],3:0,4:5,5:0}
          {% elif trigger.to_state.state == 'Shoot / Drip' %}
            {0:7,2:[255,147,41],8:0,10:1,9:30, 5:0 ,4:255,3:0,12:5,7:0}
          {% elif trigger.to_state.state == 'Rain' %}
            {0:8,11:1100}
          {% elif trigger.to_state.state == 'Stepped' %}
            {0:9}
          {% elif trigger.to_state.state == 'Startup' %}
            {0:11,3:1,4:4}
          {% elif trigger.to_state.state == 'Shutdown' %}
            {0:10,3:1}
          {% else %}
            {}
          {% endif %}
        retain: false
    - service: input_select.select_option
      data:
        entity_id: input_select.stairlamp_mode
        option: 'Select...'
```