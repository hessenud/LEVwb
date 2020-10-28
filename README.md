LEVwb
------
LEVwb is a alternative firmware for a Sonoff POW implementing a Charger (Wallbox) for Light Electric Vehicles (LEV).
The LEVwb was born when i tried to connect my Twizy to a SMA HomeManager.
This resulted in a [SEMP implementation for Arduino uSEMP](https://github.com/hessenud/uSEMP)  


required Libraries
======================
* HLW8012                --  Power Meter of Sonoff Pow rev1 
* [uSEMP](https://github.com/hessenud/uSEMP)                    --  the SEMP protocol to connect to SMA energy manager
* [uHelper](https://github.com/hessenud/uHelper)                --    several small helper bits
* ESP8266 Board support --  SSDP / WebServer / WiFi / mDNS .... all the good stuff theat comes with the core
* EEPROM                   --  will be replaced by littleFS as configuration will become larger
* ArduinoOTA            --  SW Update Over-The-Air as the Hardware should be inside a wheater-proof housing

optional
================
* RemoteDebug        -- if REMOTE_DEBUG   
* ArduinoMqttClient -- if USE_MQTT   
* Adafruit_SSD1306     -- if USE_OLED
*




ToDo
============
* configuration... WebGUI for configuration
* LittleFS vs EEPROM
* captive portal
* Sonoff Pow Rev2 uses different power meter (CSE7766) -> config option
* use OVMS to reduce max charging power according to power recommendations
* use actual PV production for local planning ( Modbus support )
