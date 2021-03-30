#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <ServoInput.h>
#include <Servo.h>

/* Put your SSID & Password */
const char* ssid = "RC_WiFi";  // Enter SSID here
const char* password = "W1r3L3ss";  //Enter Password here <- easy code for testing, Updated in LIVE env

/* Put IP Address details */
IPAddress local_ip(192,168,2,1);
IPAddress gateway(192,168,2,1);
IPAddress subnet(255,255,255,0);

const boolean serialLogging = true;
const float steeringDeadzone = 0.25;  // 0.15 = 15%
const float throttleDeadzone = 0.60;

ServoInputPin<D1> ch1; // steering
ServoInputPin<D2> ch2; // throttle

Servo steering;  // create servo object to control a servo
Servo throttle;

// failsafe setting
boolean straight = false; // do we need to set the servos to 90/centre

//Transmitter inverts
boolean throttleInvert = true;
boolean steeringInvert = true;
//WebControl inverts
boolean webthrottleInvert = true;
boolean websteeringInvert = false;

ESP8266WebServer server (80);

//This function takes the parameters passed in the URL(the x and y coordinates of the joystick)
//and sets the motor speed based on those parameters. 
void handleJSData(){
  //1stick
  if ((server.hasArg("lri") == false) && (server.hasArg("fbi") == false)) {
    Serial.println("1 stick controller");
    int x = server.arg(0).toInt() * 10;
    if (websteeringInvert) {
      steering.write (map (x, -1000, 1000, 0, 180));
    } else {
      steering.write (map (x, -1000, 1000, 180, 0));  
    }
    
    int y = server.arg(1).toInt() * 10 / 4;
    if (webthrottleInvert) {
      int mappedy = (map (y, -1000, 1000, 8, 172));
      throttle.write (mappedy);
    } else {
      throttle.write (map (y, -1000, 1000, 180, 0));  
    }
  }
  
  //2sticks
  //Left/Right
  if (server.hasArg("lri") == true) {
    Serial.println("2 stick  LR controller");
    int x = server.arg(1).toInt() * 10;
    if (websteeringInvert) {
      steering.write (map (x, -1000, 1000, 20, 160));
    } else {
      steering.write (map (x, -1000, 1000, 160, 20));  
    }  
  }

  //Front/Back
  if (server.hasArg("fbi") == true) {
    Serial.println("2 stick FB controller");
    int y = server.arg(2).toInt() * 10 / 4;  //         --------------------- LIMITER -------
      if (serialLogging == true) {
        Serial.print("Y from Web: ");
        Serial.println(y);
      }
    // normalizing for servo values
    int mapy = y / 10 ;
    //mapDeadzone
    if( 3 >= mapy && mapy >= -3) {
      throttle.write(93); //                            -------- THUMB UP HOLD set to 93 as 90 seems to throttle backwards
      if (serialLogging == true) {
        Serial.println("Deadzone! Throttle");
      }
    }
    //Foward
    if(mapy > 3) {
      int mappedy = (map (y, -1000, 1000, 8, 172));
      throttle.write (mappedy);
      if (serialLogging == true) {
        Serial.print("Throttle: ");
        Serial.print(mapy);
        Serial.println("%");
        Serial.print("Throttle Y  to  Car: ");
        Serial.println(mappedy);
      }
    }
    //Brake/Backward
    else if(mapy < -3){
      int mappedy = (map (y, -1000, 1000, 8, 172));
      throttle.write (mappedy);
      if (serialLogging == true) {
        Serial.print("Brake: ");
        Serial.print(abs(mapy));
        Serial.println("%");
        Serial.print("Brake Y  to  Car: ");
        Serial.println(mappedy);
      }
    }
    else {
    } 
  }  
//  if (webthrottleInvert) {                 ------------  commented out to be able to build the deadzone
//    int mappedy = (map (y, -1000, 1000, 8, 172));
//    throttle.write (mappedy);
//  } else {
//    throttle.write (map (y, -1000, 1000, 180, 0));  
//  }
  
  //return an HTTP 200
  server.send(200, "text/plain", "");   
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found, what are you trying to do here?");
}


void client_status() {
Serial.println("-------------Connected Client-----------------------");
Serial.print(wifi_softap_get_station_num()); Serial.println(" client.");

struct station_info *station_list = wifi_softap_get_station_info();
}


// Starting Setup
void setup()
{
  Serial.begin(115200);
  Serial.flush();
  Serial.println(" ");
  Serial.println(" ");
  Serial.println(" ");
  Serial.println("Initializing . . .");

  // attaches the servo
  steering.attach(D3);   
  throttle.attach(D4); 
  
  // setting servos to centre for start
  steering.write(90);
  throttle.write(90);
  Serial.print("Servo driver started");
  
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100); //giving some time to boot the server before continuing
  Serial.println("");
  Serial.print("WiFi started, SSID : ");
  Serial.print(ssid);
  Serial.println(" ");
  
  //initialize SPIFFS to be able to serve up the static HTML files. 
  if (!SPIFFS.begin()){
    Serial.println("SPIFFS Mount failed");
  } 
  else {
     Serial.println("SPIFFS Mount succesfull");
  }
  
  //set the static pages on SPIFFS for the html and js
  server.serveStatic("/", SPIFFS, "/2sticks.html");
  server.serveStatic("/virtualjoystick.js", SPIFFS, "/virtualjoystick.js");
  server.serveStatic("/favicon.ico", SPIFFS, "/favicon.ico");
  //call handleJSData function when this URL is accessed by the js in the html file
  server.on("/jsData.html", handleJSData);
  server.onNotFound(handle_NotFound);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop()
{
  // checking for Wifi Connected device
   uint8 WifiClient = wifi_softap_get_station_num();

  if (straight == false)
  {
    throttle.write(90);
    steering.write(90);
    
    throttle.write(90);
    delay(100); //giving some time 
    steering.write(90);
    delay(100); //giving some time 
    straight = true;
  }
   //if Wifi client is connected
   if (WifiClient) {
    client_status();
      while (wifi_softap_get_station_num() == 1) {
        // WebServer trigger
        server.handleClient();
         
        if (!WifiClient) {
          straight = false;
        }
      }
    if (!WifiClient) {
      straight = false;
    }
  }
  
  //if NO Wifi client is connected we listen to the RC transmitter
  if (!WifiClient) {
    
    // getting transmitter values
    //ch1 steering
   int steeringAngle = ch1.getAngle();
   //ch2 throttle
   int throttleAngle = ch2.getAngle();

    // Sent commands to servos
    if (steeringInvert) {
      steering.write (map (steeringAngle, 0, 180, 0, 180));
    } else {
      steering.write (map (steeringAngle, 180, 0, 180, 0));  
    }
    if (throttleInvert) {
      throttle.write (map (throttleAngle, 0, 180, 8, 172));
    } else {
      throttle.write (map (throttleAngle, 8, 172, 180, 0));  
    }
  }
  

}
