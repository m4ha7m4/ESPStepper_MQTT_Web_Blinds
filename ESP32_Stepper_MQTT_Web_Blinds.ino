
/* m4ha7m4 and father implementation of  Blind Controller based  on
 *  thehookup - Motorize and Automate your Blinds for $10! (WiFi)
 *  https://www.youtube.com/watch?v=1O_1gUFumQM  
 *  https://github.com/thehookup/Motorized_MQTT_Blinds
 *  V 1.0
 *    - added analog light sensor and tilt based on light level (assumed max of 1024)
 *  v 1.1
 *    - Added Webserver and web 
 *  v 1.2
 *    - added solar postion based on 
 *        2017 Ken Willmott https://github.com/KenWillmott/SolarPosition
 *        Arduino library based on the program "Arduino Uno and Solar Position Calculations"
 *  v 1.3
 *  
 *    - added position slider on web page https://randomnerdtutorials.com/esp32-servo-motor-web-server-arduino-ide/
 *    - added button and mqtt for turning amb light sensor control on/off. Useful for if room empty etc.
 *  v 1.5
 *    - adpated for ESP32 from NODEMCU
 *    - replaced webserver library
 *  v 2.0 
 *    - Modified for CP to allow for full range of motion
 *    - Defined "STEPS_TO_OPEN" for open position and to use STEPS_TO_CLOSE for FULL range of blinds motion and updated all code
 *    - Added web handles so "http://YOUR_IP/close" sets  position to 0, "/open" sets to STEPS_TO_OPEN and "/closedown" sets to STEPS_TO_CLOSE
 *    - Cleaned up for GITHUB
 *    - Added url for specific position: http://YOUR_IP/input?position=VALUE  
 *    - Removed dad's sun position and ambient light sensor code (sorry!)
 *   
 *   To Do: 
 *    - Prevent strings causing input of 0 and closing blinds when using URL and webpage form position
 *    - Refresh slider with button and form control. Does not work because webpage refreshes before movement begins.
 *      Would need to enable ajax refresh of slider only or delay refresh for 3-5 seconds to allow motor to complete movement
 *    - Change driver library
 *    - Add SmartThings/Hubduino integration via ST_Anything or SmartLife
 *  
 */
#include <SimpleTimer.h>    //https://github.com/marcelloromani/Arduino-SimpleTimer/tree/master/SimpleTimer
#include <WiFi.h>
#include <ESPmDNS.h>
#include <PubSubClient.h>   //https://github.com/knolleary/pubsubclient
#include <AH_EasyDriver.h>  //http://www.alhin.de/arduino/downloads/AH_EasyDriver_20120512.zip
#include <WiFiClient.h>
#include <WebServer.h>
#include <Update.h>

#define Ver_ID "V-0518-19-2.0"


/*****************  START USER CONFIG SECTION *********************************/
/*****************  START USER CONFIG SECTION *********************************/
/*****************  START USER CONFIG SECTION *********************************/
/*****************  START USER CONFIG SECTION *********************************/          

#define USER_SSID                 "YOUR_SSID"
#define USER_PASSWORD             "YOUR_WIFI_PASSWORD"
#define USER_MQTT_SERVER          "YOUR_MQTT_SERVER_ADDRESS"
#define USER_MQTT_PORT            1883
#define USER_MQTT_USERNAME        "YOUR_MQTT_USER_NAME"
#define USER_MQTT_PASSWORD        "YOUR_MQTT_PASSWORD"


#define SENSOR_ID                 "BlindsMCU"         // Used to define MQTT topics, MQTT Client ID, and ArduinoOTA

#define USER_MQTT_CMND            "cmnd/" SENSOR_ID   //Prefix for MQTT commands (/set_position, /OpnClsStp), comment out if not needed
#define USER_MQTT_STAT            "stat/" SENSOR_ID   //Prefix for MQTT/checkIn, comment out if not needed

#define STEPPER_SPEED             35                  //Defines the speed in RPM for your stepper motor
#define STEPPER_STEPS_PER_REV     1028                //Defines the number of pulses that is required for the stepper to rotate 360 degrees
#define STEPPER_MICROSTEPPING     0                   //Defines microstepping 0 = no microstepping, 1 = 1/2 stepping, 2 = 1/4 stepping 
#define DRIVER_INVERTED_SLEEP     1                   //Defines sleep while pin high.  If your motor will not rotate freely when on boot, comment this line out.

#define STEPS_TO_CLOSE            20                  //Defines the number of steps needed to move ENTIRE range of blind (close up THROUGH close down)
#define STEPS_TO_OPEN             10                  // Steps to open position from 0 (horizontal)

#define STEPPER_SLEEP_PIN         32 
#define STEPPER_MICROSTEP_1_PIN   33
#define STEPPER_MICROSTEP_2_PIN   25
//#define STEPPER_MICROSTEP_3_PIN 26                  NOT used by this driver library but present on DRV8825 board 
#define STEPPER_STEP_PIN          27 
#define STEPPER_DIR_PIN           14 

#define  OTAupdate_path "/firmware"
#define  OTAupdate_username "YOUR_UPDATEUSERNAME"
#define  OTAupdate_password "YOUR_UPDATEPASSWORD"

/*****************  END USER CONFIG SECTION *********************************/
/*****************  END USER CONFIG SECTION *********************************/
/*****************  END USER CONFIG SECTION *********************************/

WiFiClient espClient;
PubSubClient client(espClient);
SimpleTimer timer;
AH_EasyDriver shadeStepper(STEPPER_STEPS_PER_REV, STEPPER_DIR_PIN ,STEPPER_STEP_PIN,STEPPER_MICROSTEP_1_PIN,STEPPER_MICROSTEP_2_PIN,STEPPER_SLEEP_PIN);
WebServer OTAserver(80);

//Global Variables
int timerId_checkIn, timerId_stepperMot;
bool boot = true;
int currentPosition = 0;
int newPosition = 0;
char positionPublish[50];
bool moving = false;
char charPayload[50];

const char* ssid = USER_SSID ; 
const char* password = USER_PASSWORD ;
const char* mqtt_server = USER_MQTT_SERVER ;
const int mqtt_port = USER_MQTT_PORT ;
const char *mqtt_user = USER_MQTT_USERNAME ;
const char *mqtt_pass = USER_MQTT_PASSWORD ;
const char *mqtt_client_name = SENSOR_ID ; 

String webPageMsg = "";
int sliderValue = -1;
String yesNo[2] = {"No","Yes"};
const char* host = SENSOR_ID ;


  
//Functions
void setup_wifi() {
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() 
{
  int retries = 0;
  while (!client.connected()) {
    if(retries < 150)
    {
      Serial.print("Attempting MQTT connection...");
      if (client.connect(mqtt_client_name, mqtt_user, mqtt_pass)) 
      {
        Serial.println("connected");
        if(boot == false)
        {
          client.publish(USER_MQTT_STAT "/checkIn","Reconnected"); 
        }
        if(boot == true)
        {
          client.publish(USER_MQTT_STAT "/checkIn","Rebooted");
        }
        // ... and resubscribe
        client.subscribe(USER_MQTT_CMND "/OpnClsStp");
        client.subscribe(USER_MQTT_CMND "/set_position");
        Serial.println(" MQTT subscribed: " USER_MQTT_CMND "/OpnClsStp");
        Serial.println(" MQTT subscribed: " USER_MQTT_CMND "/set_position");
      } 
      else 
      {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        retries++;
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
    // this MCU only publishes MQTT, if no connection reboot, no use of loop() w/o mqtt conn
    //       if controlling lights etc, comment out this IF, as loop() should coninue. Also change delay(5000) wait!
    if(retries > 149) 
    {
    ESP.restart();
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  String newTopic = topic;
  Serial.print(topic);
  Serial.print("] ");
  payload[length] = '\0';
  String newPayload = String((char *)payload);
  int intPayload = newPayload.toInt();
  Serial.println(newPayload);
  Serial.println();
  newPayload.toCharArray(charPayload, newPayload.length() + 1);
    //BP: for OpnClsStp, dont's retain. Every time mqtt reconnects, it  will subscribe to CLOSE/OPEN. i.e. server will send
    // retained poistion of OpnClsStop which will be CLOSE/OPEN , and callback will process those and set_position to 0 or 12
    // The  postion before mqtt conn will be lost on reconnect! And position will be set to full OPEN/ full CLOSE blind for no reason! 
    // (till updated by checkIn() which will be after 90 secs
    // retain only set_position when NOT called by OPEN or CLOSE
    //CP:  Modified this to allow Close DOWN = 0, open = STEPS_TO_OPEN, close up = 20 -- NEED TO TEST DIRECTION
  if (newTopic == USER_MQTT_CMND "/OpnClsStp") 
  {
    if (newPayload == "CLOSED")
    {
      client.publish(USER_MQTT_CMND "/set_position", "0", false);
    }
    else if (newPayload == "OPEN")
    {   
      int stepsToOpen = STEPS_TO_OPEN;
      String temp_str = String(stepsToOpen);
      temp_str.toCharArray(charPayload, temp_str.length() + 1);
      client.publish(USER_MQTT_CMND "/set_position", charPayload, false);
    }
    else if (newPayload == "CLOSEDOWN")
    {   
      int stepsToClose = STEPS_TO_CLOSE;
      String temp_str = String(stepsToClose );
      temp_str.toCharArray(charPayload, temp_str.length() + 1);
      client.publish(USER_MQTT_CMND "/set_position", charPayload, false);
    }
    else if (newPayload == "STOP")
    {
      String temp_str = String(currentPosition);
      temp_str.toCharArray(positionPublish, temp_str.length() + 1);
      client.publish(USER_MQTT_CMND "/set_position", positionPublish, true);  //ok to retain for STOP
    }
  }
  
  if (newTopic == USER_MQTT_CMND "/set_position")
  {
    if(boot == true)
    {
      newPosition = intPayload;
      currentPosition = intPayload;
      boot = false;
    }
    if(boot == false)
    {
      newPosition = intPayload;
    }
  }
  
}

void processStepper()
{
  if (newPosition > currentPosition)
  {
    #if DRIVER_INVERTED_SLEEP == 1
    shadeStepper.sleepON();
    #endif
    #if DRIVER_INVERTED_SLEEP == 0
    shadeStepper.sleepOFF();
    #endif
    shadeStepper.move(80, FORWARD);
    currentPosition++;
    moving = true;
  }
  if (newPosition < currentPosition)
  {
    #if DRIVER_INVERTED_SLEEP == 1
    shadeStepper.sleepON();
    #endif
    #if DRIVER_INVERTED_SLEEP == 0
    shadeStepper.sleepOFF();
    #endif
    shadeStepper.move(80, BACKWARD);
    currentPosition--;
    moving = true;
  }
  if (newPosition == currentPosition && moving == true)
  {
    #if DRIVER_INVERTED_SLEEP == 1
    shadeStepper.sleepOFF();
    #endif
    #if DRIVER_INVERTED_SLEEP == 0
    shadeStepper.sleepON();
    #endif
    String temp_str = String(currentPosition);
    temp_str.toCharArray(positionPublish, temp_str.length() + 1);
    client.publish(USER_MQTT_STAT  "/position", positionPublish); 
    moving = false;
    
    Serial.println("Current Position " + String(currentPosition) + " State: " + blindState(currentPosition));
    Serial.println("New     Position " + String(newPosition) + " State: " + blindState(newPosition));
  }

}

String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ; 
}

String blindState(int pos){
  // Calculate state based on position in relation to 0, STEPS_TO_OPEN, STEPS_TO_CLOSE

  if (pos <= STEPS_TO_CLOSE && pos >= (STEPS_TO_CLOSE - 1)) {return "Closed Down";}
  else if (pos >= 2 && pos <  (STEPS_TO_OPEN -1)) {return "Partially Closed Up";}  // Changed from slightly open 
  else if (pos > (STEPS_TO_OPEN + 1) && pos < (STEPS_TO_CLOSE - 1)) {return "Partially Open Down";} //changed from partially open
  else if (pos < 2) {return "Closed Up";} 
  else if (pos < 0 || pos > STEPS_TO_CLOSE) {return "Invalid Position" ;}
  else {return "Fully Open";}
}


void checkIn()
{
  client.publish(USER_MQTT_STAT  "/checkIn","OK");

}

//Run once setup
void setup() {
  Serial.begin(115200);
  shadeStepper.setMicrostepping(STEPPER_MICROSTEPPING);            // 0 -> Full Step                                
  shadeStepper.setSpeedRPM(STEPPER_SPEED);     // set speed in RPM, rotations per minute
  #if DRIVER_INVERTED_SLEEP == 1
  shadeStepper.sleepOFF();
  #endif
  #if DRIVER_INVERTED_SLEEP == 0
  shadeStepper.sleepON();
  #endif
  WiFi.mode(WIFI_STA);
  client.setCallback(callback);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  delay(10);
  timerId_stepperMot = timer.setInterval(((1 << STEPPER_MICROSTEPPING)*5800)/STEPPER_SPEED, processStepper);   
  timerId_checkIn = timer.setInterval(90000, checkIn);  //check every 90000msec = 90 sec

  if (MDNS.begin(host)){Serial.println("MDNS responder started");}
  MDNS.addService("http", "tcp", 80);
  Serial.printf("HTTPUpdateServer ready! Open http://%s.local%s in your browser and login with username '%s' and password '%s'\n", host, OTAupdate_path, OTAupdate_username, OTAupdate_password);
  String localIP =  IpAddress2String(WiFi.localIP());
  char str_array[localIP.length()];
  localIP.toCharArray(str_array, localIP.length()+1);
  Serial.println(" localIP char " + String(str_array));
//  Serial.printf("If port forwarding is set in Router, Open http://%s:8266%s in your browser and login with username '%s' and password '%s'\n", str_array, update_path, update_username, update_password);
                // http://webupdate-nodemcu-garage.local/firmware
// web page status client
  OTAsetup();
  OTAserver.begin();

  checkIn();
}
void loop() 
{
  timer.run();
  if (!client.connected()){reconnect();}
  client.loop();
  OTAserver.handleClient();  // OTA via web chek
}
