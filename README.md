Modified code to function on my ESP32 DOIT Devkit V1 clone and added a webserver with HTTP url based commands.


Also uploaded my STL files for 4mm sqaure adapter (with option of 45 offset on square side), 6mm Hex adapter (original hex was asymmetrical) and motor mount to fit my blinds with an offset rod from Home Depot



### m4ha7m4 and father implementation of  Blind Controller based  on thehookup's ESP8266 project!
     thehookup - Motorize and Automate your Blinds for $10! (WiFi)
     https://www.youtube.com/watch?v=1O_1gUFumQM  
      https://github.com/thehookup/Motorized_MQTT_Blinds
    V 1.0
        - added analog light sensor and tilt based on light level (assumed max of 1024)   
        -  REMOVED IN V2.0
    v 1.1
       - Added Webserver and web 
    v 1.2
      - added solar postion based on 
        2017 Ken Willmott https://github.com/KenWillmott/SolarPosition
        Arduino library based on the program "Arduino Uno and Solar Position Calculations"
       - REMOVED IN v2.0
    v 1.3 
      - added position slider on web page https://randomnerdtutorials.com/esp32-servo-motor-web-server-arduino-ide/
      - added button and mqtt for turning amb light sensor control on/off. Useful for if room empty etc. - REMOVED
    v 1.5
     - adpated for ESP32 from NODEMCU
     - replaced webserver library
    v 2.0 
      - Modified for CP to allow for full range of motion of blinds
      - Defined "STEPS_TO_OPEN" for open position and to use STEPS_TO_CLOSE for FULL range of blinds motion and updated all code
      - Added web handles so "http://YOUR_IP/close" sets  position to 0, "/open" sets to STEPS_TO_OPEN and "/closedown" sets to * 
        STEPS_TO_CLOSE
      - Cleaned up for GITHUB
      - Added url for specific position: http://YOUR_IP/input?position=VALUE  
      - Removed dad's sun position and ambient light sensor code (sorry!)
   
### To Do: 
     - Prevent strings causing input of 0 and closing blinds when using URL and webpage form position
     - Refresh slider with button and form control. Does not work because webpage refreshes before movement begins.  Would need to enable ajax refresh of slider only or delay refresh for 3-5 seconds to allow motor to complete movement
     - Change driver library to play with microstepping
     - Add SmartThings/Hubduino integration via ST_Anything or SmartLife
  

 
ORIGINAL PROJECT BELOW:
https://github.com/thehookup/Motorized_MQTT_Blinds
# Motorized_MQTT_Blinds


This repository is to accompany my Motorized_MQTT_Blinds video:

[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/1O_1gUFumQM/0.jpg)](https://www.youtube.com/watch?v=1O_1gUFumQM)

## Parts List
Stepper Motors: https://amzn.to/2D5rVsF

Stepper Drivers: https://amzn.to/2OZqW1W

NodeMCU: https://amzn.to/2I89xDF

12V Power Supply: https://amzn.to/2G2ZJrf

Buck Converter: https://amzn.to/2UsQ7jA

