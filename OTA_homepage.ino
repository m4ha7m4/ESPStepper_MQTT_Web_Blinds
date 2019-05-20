//https://lastminuteengineers.com/creating-esp8266-web-server-arduino-ide/
String SendHTML(){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
//  ptr +="<meta http-equiv=\"refresh\" content=\"60\" />\n";
  ptr +="<title>Blind Contoller 152-1</title>\n";
  ptr +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;} h3 {color: #444444;margin-bottom: 50px;}\n";
  ptr +=".button {display: block;width: 200px;background-color: #1abc9c;border: none;color: white;padding: 7px 15px;text-decoration: none;font-size: 15px;margin: 0px auto 17px;cursor: pointer;border-radius: 2px;}\n";
  ptr +=".button-refresh {background-color: #1abc9c;}\n";
  ptr +=".button-refresh:active {background-color: #16a085;}\n";
  ptr +=".button-reset {background-color: #1abc9c;}\n";
  ptr +=".button-reset:active {background-color: #16a085;}\n";
  ptr +=".slider { width: 300px;}\n";
  ptr +="p {font-size: 14px;color: #800;margin-bottom: 10px;}\n"; //maroon #800, grey #888, teal #088, red #F00
  ptr +="h5 {color: #F00;}\n";
  ptr +="h6 {color: #00F;}\n";
  ptr +="</style>\n";
  ptr +="<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js\"></script>\n";
  ptr +="</head>\n";
  ptr +="<body>\n";
  ptr +="<h5>Blind Controller 152-1</h5>\n";
  ptr +="<h6>Device ID: " + String(SENSOR_ID)+" @ " + String(IpAddress2String(WiFi.localIP())) + "<br>\n";
  ptr +="URL for position control: http://" + String(IpAddress2String(WiFi.localIP())) + "/input?position=VALUE <br>\n";
  ptr +="Software Ver: " + String(Ver_ID)+ "<br>\n";
  ptr +="SSID: " + String(USER_SSID)+ "<br>\n";
  ptr +="MQTT server: " + String(USER_MQTT_SERVER) + " Conn State : " + String(client.connected()) + "</br>\n";

  
  ptr +="<p>Select Bind Position (Closed Up = 0,Open = " + String(STEPS_TO_OPEN) + ", Closed Down = " + String(STEPS_TO_CLOSE) + ") Current Position: <span id=\"sliderPos\"></span></p>\n";    
  ptr +="<input type=\"range\" min=\"0\" max=\"" + String(STEPS_TO_CLOSE) + "\" class=\"slider\" id=\"thisSlider\" onchange=\"thisSlider(this.value)\" value=\""+String(currentPosition)+"\"/>\n";          
  ptr +="<script>\nvar slider = document.getElementById(\"thisSlider\");\n"; 
  ptr +="var sliderP = document.getElementById(\"sliderPos\");\n sliderP.innerHTML = slider.value;\n"; 
  ptr +="slider.oninput = function() {\n slider.value = this.value;\n sliderP.innerHTML = this.value;\n }\n"; 
  ptr +="$.ajaxSetup({timeout:1000});\n function thisSlider(pos) { \n"; 
  ptr +="  $.get(\"/?sliderValue=\" + pos + \"&\");}\n</script>\n";
  
  ptr +="<form action=\"http://" + String(IpAddress2String(WiFi.localIP())) + "/input\" target=\"_self\" method=\"get\">  Position: <input type=\"text\" name=\"position\"> <input type=\"submit\" value=\"Submit\"> </form>";
  ptr +="<p><a class=\"button button-refresh\" href=\"/refresh\">Refresh</a>\n";

// v2.0 2019.05.19 - Added buttons for open/close/closedown
  ptr +="<p><a class=\"button button-open\" href=\"/open\">Open</a>\n";
  ptr +="<p><a class=\"button button-close\" href=\"/close\">Close Up (default)</a>\n";  
  ptr +="<p><a class=\"button button-closedown\" href=\"/closedown\">Close Down</a>\n"; 
  ptr +="<p><a class=\"button button-mqqtStat\" href=\"/mqttstat\">Mqtt Pub</a>\n";

  ptr +="<p><a class=\"button button-firmware\" href=\"/firmware\">Upd Firmware</a>\n";

  ptr +="<p><a class=\"button button-reset\" href=\"/resetesp\">Reset (not reliable)</a>\n";
 
  if(webPageMsg != ""){
    Serial.println("Mesage -: " + webPageMsg);
    ptr +="<p> Message: " + webPageMsg + "</p>\n";
  } 
  
  ptr +="<p>timer status: chekIn on? " + yesNo[timer.isEnabled(timerId_checkIn)] + " </p>\n";
  ptr +="</body>\n"; 
  ptr +="</html>\n";
  //webPageMsg = "";
  //Serial.println(ptr);
  return ptr;
}
void handle_status() {
  Serial.println("header message args: " + String(OTAserver.args()));
  String message, argNm, argVal;
  int newSliderValue= -1;
  for (uint8_t i = 0; i < OTAserver.args(); i++) {
     argNm = OTAserver.argName(i);
     argVal =  OTAserver.arg(i);
     if (argNm == "sliderValue"){ newSliderValue = argVal.toInt();}
     message += " " + argNm + ": " + argVal + "\n";
  }
  Serial.println("header message: " + message);
  if(newSliderValue != -1 ){handle_sliderChange(newSliderValue);}
  OTAserver.send(200, "text/html", SendHTML());
}
void handle_sliderChange(int newValue){
  if(newValue != sliderValue){
    sliderValue = newValue;
    String temp_str = String(sliderValue);
    temp_str.toCharArray(positionPublish, temp_str.length() + 1);
    client.publish(USER_MQTT_CMND "/set_position", positionPublish, true);
    webPageMsg = " New Blind Position: " + temp_str + " and Published cmnd/" + String(SENSOR_ID) + "/set_position " + temp_str;
    Serial.println("sliderStatus message: " + webPageMsg);
    }
}
void handle_resetesp(){
  Serial.println("Performing Soft Restart after 2 sec..");
   delay(2000);
  Serial.println("resetting");
  WiFi.disconnect();
  ESP.restart();
}
void handle_NotFound (){
  String msg = "Response not available from" + String(USER_MQTT_SERVER); // does not work still hows 'not found'
  OTAserver.sendHeader("Location", "/",true);  
  OTAserver.send(404, "text/plain", msg);
}
void handle_webMqttPub(){
  String temp_str = String(currentPosition);
  temp_str.toCharArray(positionPublish, temp_str.length() + 1);
  client.publish(USER_MQTT_CMND "/set_position", positionPublish, true);
  webPageMsg = " Published cmnd/" + String(SENSOR_ID) + "/set_position " + temp_str + " to MQTT server at " + String(USER_MQTT_SERVER) + " Conn State : " + String(client.connected());
  OTAserver.sendHeader("Location", "/",true);
  OTAserver.send (302, "text/plain", "");
}

// 2019.05.19 - CP Added HTML handle/commands
void handle_webOpen(){
  handle_sliderChange(STEPS_TO_OPEN);
  OTAserver.sendHeader("Location", "/",true);
  OTAserver.send (302, "text/plain", "");
}

void handle_webClose(){
  handle_sliderChange(0);
  OTAserver.sendHeader("Location", "/",true);
  OTAserver.send (302, "text/plain", "");
}
void handle_webCloseDown(){
  handle_sliderChange(STEPS_TO_CLOSE);
  OTAserver.sendHeader("Location", "/",true);
  OTAserver.send (302, "text/plain", "");
}

// Handle for parsing position from http://IPaddress/input?position=INTeger 
// Strings are converted to 0 (closed) so ignored for now
void handle_position(){  
String urlString = OTAserver.arg("position");
  int urlPosition = urlString.toInt();
  if(urlPosition >= 0 && urlPosition <= STEPS_TO_CLOSE){  //prevents input outside of defined step range
    handle_sliderChange(urlPosition);
  }
  OTAserver.sendHeader("Location", "/",true);
  OTAserver.send (302, "text/plain", "");

}
