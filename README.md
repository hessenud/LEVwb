LEVwb
------
LEVwb is a alternative firmware for a Sonoff POW implementing a Charger (Wallbox) for Light Electric Vehicles (LEV).
The LEVwb was born when i tried to connect my Twizy to a SMA HomeManager.
For simplicity reasons it started as an Arduino Sketch and resulted in a [SEMP implementation for Arduino uSEMP](https://github.com/hessenud/uSEMP)  

Arduino ist _not_ suited for developing something much bigger as "Blink.ino" but... 
...still it's easy for anybody to customize the code, what's really necessary as there is still no run-time-configuration implemented.
So right now, there's only a statically compiled config thats done using Arduino (or alike) in the "_LEVwb.ino_"  file and then downloaded to the target.


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



HTTP requests 
====================
    http://TPOW-1.local/on    -- switch ON
    
    http://TPOW-1.local/off    -- switch OFF
    
    http://TPOW-1.local/pwr    -- return actual measurements as XML
    
    http://TPOW-1.local/energy?plan=0&requested=3000&optional=6000&start=0&end=10800     
        -- plan = number of energy request (0)
            / if plan param is missing a new request is allocated     
        -- requested = requested energy in Wh (3kWh)
        -- optional  = optional energy           (6kWh)    
        -- start     = earliest start in seconds from NOW  (now)
        -- end         = latest end     in seconds from NOW      (3h from now)
                                                                                
    http://TPOW-1.local/energy?plan=0&requested=1000&optional=6000&startTime=08:00&endTime=16:00    
        -- see above.. 
        -- startTime = earliest start as Time of Day
        -- endTime     = latest end     as Time of Day    
                                                                                
                                                                                        
    http://TPOW-1.local/energy?plan=0                    -- delete plan 0                                                                                                                                                    
                                                                                  


You have to calibrate the measurement with a known load or with another measurement. If you use a known load, make sure to use a resitive load like a light bulb.
    
    http://TPOW-1.local/calibrate?power=2000&current=8.8&voltage=228            
            -- calibrate measurement
            -- power    = active power (2000)[W]
            -- current    = active current (8.8)[A]
            -- voltage    = active voltage (228)[V]
    


ToDo
============
* configuration... WebGUI for configuration
* LittleFS vs EEPROM
* captive portal
* Sonoff Pow Rev2 uses different power meter (CSE7766) -> config option
* use OVMS to reduce max charging power according to power recommendations
* use actual PV production for local planning ( Modbus support )
* ...
