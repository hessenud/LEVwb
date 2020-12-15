LEVwb
------
LEVwb is a alternative firmware for a Sonoff POW implementing a Charger (Wallbox) for Light Electric Vehicles (LEV).
The LEVwb was born when i tried to connect my Twizy to a SMA HomeManager.
For simplicity reasons it started as an Arduino Sketch and resulted in a [SEMP implementation for Arduino uSEMP](https://github.com/hessenud/uSEMP)  

Arduino ist _not_ suited for developing something much bigger as "Blink.ino" but... 
...still it's easy for anybody to customize the code, what's really necessary as there is still no run-time-configuration implemented.
So right now, there's only a statically compiled config thats done using Arduino (or alike) in the "_LEVwb.ino_"  file and then downloaded to the target.

Currently this Firmware is for Rev1 of Sonoff POW. Recently I got a Rev2 with a different power meter chip (CSE7766). As soon as the cleanup-operation on this source is sufficiently advanced, i will integrate this...


Building and installation
==========================
Download or clone LEVwb. Download  the Libraries. Th easiest way is to download everything as **\*.zip**.  Unzip LEVwb and use the Arduino Library Manager to add uHelper and uSEMP as a **\*.zip library**. 
Calibration of the power meter is in EEPROM but all the other config and the gui is stored in LittleFS, so select a partition scheme with space for a filesystem. 

###required Libraries
* HLW8012                --  Power Meter of Sonoff Pow rev1 
* [uSEMP](https://github.com/hessenud/uSEMP)                    --  the SEMP protocol to connect to SMA energy manager
* [uHelper](https://github.com/hessenud/uHelper)                --    several small helper bits
* ESP8266 Board support --  SSDP / WebServer / WiFi / mDNS .... all the good stuff theat comes with the core
* EEPROM                   --  will be replaced by littleFS as configuration will become larger
* ArduinoJSON			--
* WiFiManager			--  Captive Portal WIFI Configuration
* ArduinoOTA            --  SW Update Over-The-Air as the Hardware should be inside a wheater-proof housing

####optional
* RemoteDebug        	-- if REMOTE_DEBUG   works only with patches when used with WifiManager I will integrate a variant in uHelper
* ArduinoMqttClient 		-- if USE_MQTT   
* Adafruit_SSD1306     	-- if USE_OLED
*...

### Upload Image and additional files
The files in "data/" have to be uploaded to the LittleFS filesystem. You can either do it using the LittlFS uploading tool from within Arduino IDE or you can use the upload feature of the integrated filesytem-browser. I've "assimilated" the FSBrowser example of LittleFS, giving you a way to upload and edit individual files.
Edit data/config.json as needed.
There is still some configuration done at compile time. Look at the defines in LEVwb.ino and edit whatever suits your needs. 



WebGUI
====================
Now the device serves a simple GUI. I'm no GUI designer, so this is more of the least possibl effort ;-)
Just open the GUI with http://ipaddrOrHostname/  and the Rest should be very self explanatory.
It's still not fully working, but at least the basics are implemented.

The webpages and css are stored in LittleFS filesystem.
    
HTTP requests 
====================
	replace TPOW-1.local with your selected Hostname
	
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
    
    http://TPOW-1.local/ctl?on 		-- switch ON
    						 ?off        -- switch oFF
    						 ?                                                                                                                                       
    
    http://TPOW-1.local/pwr			-- actual values as XML file
    http://TPOW-1.local/stat			-- actual values as JSON
    
    http://TPOW-1.local/setProfile		-- set profile 
     	-- profile = profile idx  0:Std  1:Qucik  2:Prog2     
        	-- requested = requested energy in Wh (3kWh)
        	-- optional  = optional energy           (6kWh)    
        	-- startTime = earliest start as TimeSTring "hh:mm"
        	-- endTime   = latest end  as TimeSTring "hh:mm"
        	-- ToD		= true means start an end times are "TimeOfDay" absolute otherwise relative	
    	
    http://TPOW-1.local/getProfiles		-- get profiles as JSON

    http://TPOW-1.local/restart		--  reset and restart
    
	http://TPOW-1.local/reload			-- reload  calibration and parameters                             


You have to calibrate the measurement with a known load or with another measurement. If you use a known load, make sure to use a resitive load like a light bulb.
    
    http://TPOW-1.local/calibrate?power=2000&current=8.8&voltage=228            
            -- calibrate measurement
            -- power    = active power (2000)[W]
            -- current    = active current (8.8)[A]
            -- voltage    = active voltage (228)[V]
    


ToDo
============
* configuration... WebGUI for configuration
* Sonoff Pow Rev2 uses different power meter (CSE7766) -> config option
* use OVMS to reduce max charging power according to power recommendations
* use actual PV production for local planning ( Modbus support )
* secure auto wifi configuration-- everybody can hijack the wallbox when configured Wifi is down!!!
