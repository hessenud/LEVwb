LEVwb
------
LEVwb is a alternative firmware for a Sonoff POW implementing a Charger (Wallbox) for Light Electric Vehicles (LEV).
The LEVwb was born when i tried to connect my Twizy to a SMA HomeManager.
For simplicity reasons it started as an Arduino Sketch and resulted in a [SEMP implementation for Arduino uSEMP](https://github.com/hessenud/uSEMP)  

Arduino ist _not_ suited for developing something much bigger as "Blink.ino" but... 
...still it's easy for anybody to customize the code, what's really necessary as there is still no run-time-configuration implemented.
So right now, there's only a statically compiled config thats done using Arduino (or alike) in the "_LEVwb.ino_"  file and then downloaded to the target.

Currently this Firmware works with Sonoff POW Revision 1 _and_ Revision 2.There is also support for other Appliances like Dishwasher, Dryer, Pumps, Heating....
... but the GUI doesn't support this (yet)  the Alternative GUI is the integrated FileBrowser, that let's you edit the JSON config files directly on the device.

Features:
===========
* works on Sonoff POW R1 + R2   CSE7766 and HLW8012 
* speaks SMA's SEMP protocol and integrates nicely in SMA EnergyManager installations
* minimum WebGUI for Chargers (LEVwb - Wallbox Light Electric Vehicles)   
* support all SEMP defined DeviceTypes
* Timer ( work-in-progress )
* autodetection for energy requests, with programmable thresholds (Power/Time)
* ...
* incomplete documentation... yes it's a feature and no BUG ;-)

Actually everything is work-in-progress ;-) yet... and will be for a loong loooong time


Building and installation
==========================
Download or clone LEVwb. Download  the Libraries. Th easiest way is to download everything as **\*.zip**.  Unzip LEVwb and use the Arduino Library Manager to add uHelper and uSEMP as a **\*.zip library**. 
Calibration of the power meter is in EEPROM but all the other config and the gui is stored in LittleFS, so select a partition scheme with space for a filesystem. 

###required Libraries
* HLW8012                --  Power Meter of Sonoff Pow rev1 
* [uSEMP](https://github.com/hessenud/uSEMP)                    --  the SEMP protocol to connect to SMA energy manager
* [uHelper](https://github.com/hessenud/uHelper)                --    several small helper bits
* ESP8266 Board support --  SSDP / WebServer / WiFi / mDNS .... all the good stuff theat comes with the core
* EEPROM                --  Calibration 
* LittleFS				--  Configuration / Webserver 
* ArduinoJSON			--
* WiFiManager			--  Captive Portal WIFI Configuration
* WebSocketsServer		--
* ArduinoOTA            --  SW Update Over-The-Air as the Hardware should be inside a wheater-proof housing
* TimeLib

####optional
* RemoteDebug        	-- if REMOTE_DEBUG works only with patches: delete the Websockets-implementation to prevent linker trouble
* ArduinoMqttClient 		-- if USE_MQTT   
* Adafruit_SSD1306     	-- if USE_OLED
*...

### Upload Image and additional files
The files in "data/" have to be uploaded to the LittleFS filesystem. You can do it using the LittlFS uploading tool from within Arduino IDE. I've "assimilated" the FSBrowser example of LittleFS, for uploading and editing individual files.
Edit data/_*.json files as needed.
There is still some configuration done at compile time. Look at the defines in LEVwb.ino and edit whatever suits your needs. 

#### Configuration
There is a config section in the WebGUI for the devices configuration  _http://TPOW-1.local/cfg.html_
But the WebGUI is not really maintained or particularly well and beautifully designed...
So the "official" config-GUI is via the online editor of the integrated FSbrowser: _http://TPOW-1.local/edit_.
Every config option is available and tweakable by editing the 4 config files:

##### _device.json	- device description
-  modelVariant: (0 - Simualtion, 1 - Sonoff Pow rev1, 2 - rev2)   
-  maxPwr: 			maximum power consumption for SEMP Device description
-  hostname: 		MDNS hostname
-  device_name:  	name of SEMP device
-  model_name: 	 	your choice for a (SSDP) model name... 

##### _preferences	- device parameters
-  assumed_power: 	1000,
-  mqtt_broker:    	hostname or IP address of MQTT broker
-  mqtt_broker_port: port number
-  mqtt_user:		user name for MQTT broker ( optional )		 
-  mqtt_password: 	( optional )
-  updateTime: 		( future use )
-  devType: 			SEMP device number  12: other  5:EVCharger
-  intr: 			interruptible
-  defCharge: 		energy for an auto-detected energy request
-  autoDetect: 		true/false   enable autodetect feature
-  ad_on_threshold: 	power threshold for auto detecting a request
-  ad_on_time: 		seconds for power threshold until auto detect is valid
-  ad_off_threshold:	lower power threshold for auto detecting end of 
 					active power request
-   ad_off_time: 	seconds for lower power threshold until end of 
  					active request is valid
- ad_prolong_inc: 	if an uninterruptible device has not finished the 
  					active request at the end of the
  					planned timeframe ( e.g. laundr-O-mat needs another round )
  				    prolong the active frame by nn seconds



#### Calibration
Like suggested in the Sonoff POW Users Manual. Take a voltmeter, a known load or a calibrated amperemeter and enter the Values in the calibration section on the config page: _http://TPOW-1.local/cfg.html_
* exp. Power		-  known power consumptin of a well defined load like a light bulb
* exp. Voltage	-  the actual measured voltage at point of load
* exp. Current	-  either the measured current or Power/Current ( beware- use load without apparent power component)

and press _CALIBRATE!_ button

   
	



WebGUI
====================
Now the device serves a simple GUI. I'm no GUI designer, so this is more of the least possible effort ;-)
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
* variant for household appiances like dryer, washing machine, dishwasher. They have different needs and use-patterns
* use OVMS to reduce max charging power according to power recommendations
* use actual PV production for local planning ( Modbus support )
* secure auto wifi configuration-- everybody can hijack the wallbox when configured Wifi is down!!!
