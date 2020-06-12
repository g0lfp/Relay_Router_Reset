

// ESP Relay board router autoresetter.
// by G0LFP
// Has no actual functionality, but enables a wifi update client.
// Other than checking for internet connectivity. If it can't see out, it power cycles the router
// http://192.168.x.y/?relay=1 to turn the relay on
// http://192.168.x.y/?relay=0 to turn the relay off again

// The opto isolated input part isn't coded into this version.

// Load this into a new ESP Relay PCB so that all you have to do is apply power.
// Connect your phone to select WiFi access point.
// Then you can upload your program 'OTA'

// http://192.168.x.y/update to send a .bin of your new program to the ESP OTA.

//#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266httpUpdate.h>
#include <ESP8266HTTPClient.h>

#include "WiFiManager.h"          //https://github.com/tzapu/WiFiManager

#include <EEPROM.h>

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

const char* host = "esp8266-http-Update";
#define LEDPIN 2    // some versions differ
#define RELAYPIN 4
#define BUTTON 0
#define REDLED 2

bool relayStatus;
int counterVal = 3600; //Set to one hour until changeable. using - EEPROM.read(0);
//int checkSum = EEPROM.get(4);
int counter;

/*
 * To do list...
 * Check the that the checksum is what we expect it to be.
 * If it isn't then set the counter a short value, say 10.
 * Write this value as the default to EEPROM
 * That'll stop it from just hanging up for ages.
 * 
 * Write a page to take the delay argument from the browser. Check its validity
 * then write it to EEPROM
 * 
 * 
 * */

void setup() 
{
  // Wait at least 10 seconds until trying to connect to the wifi. 
  // The ESP reboots and starts a lot faster than my router, so fails to connect to the WiFi

  delay(10000);
  
  
  // Setup code goes here, runs once
  Serial.begin(115200);
  pinMode(LEDPIN,OUTPUT);
  pinMode(REDLED, OUTPUT);
  pinMode (RELAYPIN,OUTPUT);
  pinMode(BUTTON,INPUT);
  counterVal = counterVal * 2; // 2hz clock ;)

   //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing - maybe addd a config pin tto.
  //wifiManager.resetSettings();    Uncommenting this line forces a reset to wifi settings.
                                    // This means you will have to connect and config wifi every time you boot the device.

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if(!wifiManager.autoConnect()) 
  {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    //delay(1000);
  } 

  // Once you get to this point, you have connected successfully to your wifi
  Serial.println("Connected!");
  long ver = ESP.getChipId();
  Serial.print("Conn:ESP");
  Serial.println(ver,DEC);
  Serial.print("Local IP address:");
  Serial.println(WiFi.localIP());
  

  
  // This chunk enables the update bin server.
    MDNS.begin(host);
    httpUpdater.setup(&httpServer);
    httpServer.begin();
    MDNS.addService("http", "tcp", 80);
    //Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", host);

    httpServer.on ( "/", handleRoot );  //see the function at the end of the code. 


    relayStatus = 0;
}

void loop(void)
{

  httpServer.handleClient();
    
  HTTPClient http;
  //==============================================================

  // If this is a remote monitor, it will probbly want to call a server page and pass data.
  // 
  
  String pageToLoad = "http://www.5and9.co.uk/test.php?ip="; // This chunk of code enables me to see it call my server
    pageToLoad += WiFi.localIP().toString();                 //  and post its ID and IP address.
    pageToLoad += "&id=";
    pageToLoad += ESP.getChipId();
    Serial.println(pageToLoad);
  http.begin(pageToLoad); 
  int httpCode = http.GET();
  delay(500);
  if(httpCode > 0) 
    {    
    // Page load success
      String payload = http.getString();
      Serial.print(" pass: Returned:");
      Serial.println(httpCode);
    }
    else
    {

      Serial.print("Fail: Returned:");
      Serial.println(httpCode);
    // handle page load failure 
    // wait here for 1 minute for the router to connect and then toggle the relay. 
    Serial.println("Unable to connect, waiting a couple of minutes");
    digitalWrite(LEDPIN,HIGH);
    for (int i = 0; i< 600; i++)      
      {
      //digitalWrite(LEDPIN, HIGH);
      //digitalWrite(REDLED, HIGH);
        
      delay(975);  
      httpServer.handleClient();
      digitalWrite(REDLED, LOW);    
      httpServer.handleClient();
      delay(25); 
      counter--;
      digitalWrite(REDLED, HIGH);
      }
      // 10 minutes later, we'll check again and if nothing we toggle to reset.

      String pageToLoad = "http://www.5and9.co.uk/failover.php?ip="; // This chunk of code enables me to see it call my server
    pageToLoad += WiFi.localIP().toString();                 //  and post its ID and IP address.
    pageToLoad += "&id=";
    pageToLoad += ESP.getChipId();
    Serial.println(pageToLoad);
  http.begin(pageToLoad); 
  int httpCode = http.GET();
  delay(500);
  if(httpCode > 0) 
    {    
      // If we saw the server do nothing.... everything is OK afterall.
      // httpCode should be a negative number.. >0 means that no connecyion could be made.
    }
    else
    {
      // If we still can't see the server, either the WAN is down or the router is not responding
      // in a timely fashion. In either case, a cold boot may help.
      digitalWrite(RELAYPIN,HIGH);
        Serial.println("Relay = on");
        relayStatus = 1;
        delay(2000);
      digitalWrite(RELAYPIN,LOW);
        Serial.println("Relay = Off");
        relayStatus = 0;
        delay(10000); // to allow router to come back up before proceeding.. This could probably be a lot shorter.
    }     
    }

    // The line below tells the ESPduino where to go looking for an update. 
    // it's probably not a good idea to have it constantly hitting a server.
    // The line is left here as an example for you to edit and use at your discretion.
    //t_httpUpdate_return ret = ESPhttpUpdate.update("http://www.yourdomain/ESPprogram.bin");

    // Heartbeat flashing LED, this also acts as a yield to the uP such that the update can take place.
 for (int i = 0; i< 3598; i++)      //Number of seconds to wait.
      {
      //digitalWrite(LEDPIN, HIGH);
      //digitalWrite(REDLED, HIGH);
        
      delay(975);  
      httpServer.handleClient();
      digitalWrite(LEDPIN, HIGH);    
      httpServer.handleClient();
      delay(25); 
      counter--;
      digitalWrite(LEDPIN, LOW);    
      
      if (counter == 0)
        {
        digitalWrite(RELAYPIN,LOW);
        Serial.println("Relay = off");
        relayStatus = 0;
        }

       // READ THE BUTTON AND TOGGLE THE RELAY
       // if BUTTON && relaystatus
       // turn OFF
      
      if (!digitalRead(BUTTON) && relayStatus)
      {
        digitalWrite(RELAYPIN,LOW);
        Serial.println("Relay = off");
        relayStatus = 0;
        while(!digitalRead(BUTTON)) {}
      
      }
           

       // if BUTTON && !relaystatus
       // turn ON
      if (!digitalRead(BUTTON) && !relayStatus)
      {
        digitalWrite(RELAYPIN,HIGH);
        Serial.println("Relay = on");
        relayStatus = 1;
        counter = counterVal;
        while(!digitalRead(BUTTON)) {} // now let go!
        
      
      }


      digitalWrite(LEDPIN,!relayStatus);
       
      }
    
    http.end();
   
   //delay(36000); // Don't be tempted to do this..  you cannot talk to the ESP in this state.
  
  
}

// The websserver code actually lives here, this reads the args and acts upon them. The relay is already here.
void handleRoot() {
   String message = "<HTML>";
   message += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
   message +="<H1>Relay Board </H1>  <br><br>";
   
   message += WiFi.localIP().toString();
   message += "<br>ESP";
   message += ESP.getChipId();
  message += "<br> URI: ";
  message += httpServer.uri();
  message += "<br>Method: ";
  message += ( httpServer.method() == HTTP_GET ) ? "GET" : "POST";
  //message += "<br>  Arguments: ";
  //message += httpServer.args();

  message += "<form action=\"/\"  method = \"get\">";
  
 

  for ( uint8_t i = 0; i < httpServer.args(); i++ ) 
  {
   // message += " " + httpServer.argName ( i ) + ": " + httpServer.arg ( i ) + "\n";
  }
  
  if (httpServer.arg("relay") == "") //default state 
    {
      // Don't turn anything on or off, just put the status on the screen, allow the user to choose!
      //digitalWrite(RELAYPIN,LOW); 
      //Serial.println("Relay = off");
      if (relayStatus == 0)
        {
          message += "<input type=\"Submit\" value=\"On\" name=\"relay\">"; //<input type="submit" value="On" name="relay">
        }
        else
        {
          message += "<input type=\"Submit\" value=\"Off\" name=\"relay\">"; //<input type="submit" value="On" name="relay">  
        }
     
    }
    if (httpServer.arg("relay") == "Off")
    {
      digitalWrite(RELAYPIN,LOW);
      digitalWrite(LEDPIN,LOW);
      Serial.println("Relay = off");
      relayStatus = 0;
      message += "<input type=\"Submit\" value=\"On\" name=\"relay\">";
      //message += "&nbsp<input id=\"checkbox1\" name=\"checkbox\" type=\"checkbox\" checked=\"unchecked\">";
    }
    
    if (httpServer.arg("relay") == "On")
    {
      digitalWrite(RELAYPIN,HIGH);
      digitalWrite(LEDPIN,LOW);
      Serial.println("Relay = on");
      relayStatus = 1;
      message += "<input type=\"Submit\" value=\"Off\" name=\"relay\">";
      //message += "&nbsp<input id=\"checkbox1\" name=\"checkbox\" type=\"checkbox\" checked=\"unchecked\">";
    }
   
    if (httpServer.arg("relay") == "1")
    {
      digitalWrite(RELAYPIN,HIGH);
      digitalWrite(LEDPIN,LOW);
      Serial.println("Relay = on");
      relayStatus = 1;
      message += "<input type=\"submit\" value=\"On\">";
     // message += "<br><input id=\"checkbox1\" name=\"checkbox\" type=\"checkbox\" checked=\"checked\">";
    }
  
  if (httpServer.arg("relay") == "0")
    {
      digitalWrite(RELAYPIN,LOW);
      digitalWrite(LEDPIN,LOW);
      Serial.println("Relay = off");
     relayStatus = 0;
       message += "<input type=\"submit\" value=\"On\">";
     // message += "<br><input id=\"checkbox1\" name=\"checkbox\" type=\"checkbox\" checked=\"unchecked\">";
    }

     message += "</form> ";

     if (relayStatus  == 0) 
    {
      message += "<H2>Relay is OFF </H2>  <br>";
    }
    else
    {
      message += "<H2>Relay is ON </H2>  <br>";
    }
  message += "</HTML>";
  httpServer.send ( 200, "html", message );
 
}
