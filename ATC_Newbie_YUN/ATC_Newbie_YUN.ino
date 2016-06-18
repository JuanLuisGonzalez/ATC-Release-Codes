/*Arduino Total Control Firmware for Jun
 REMEMBER TO DISCONNECT BLUETOOT TX PIN WHEN UPLOADING SKETCH 
 Basic functions for controlling stuff via ATC app.
 Please use the "example" Codes to get all the awesome features the app has!!!
 This code works for arduino YUN or YUN shield only.
 If using this sketch, DO NOT ENABLE accelerometer data in the app or seek bars or funny stuff will happen :)
 
 Commnunication is made as the following:
  Arduino -> Bridge -> WiFi module on yun -> Your wireless router (WLAN) -> ATC App
 Port: 5555
 
 * Controls 5 relays (digital outputs) with the first 5 buttons in the app
 
 Author: Juan Luis Gonzalez Bello 
 Date: Feb 2016   
 Get the app: https://play.google.com/store/apps/details?id=com.apps.emim.btrelaycontrol 
 */

#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>

// Special commands
#define CMD_SPECIAL '<'
#define CMD_ALIVE   '['

// Listen on default port 5555, the webserver on the Yun
// ip addres is asigned by your router
YunServer server;
YunClient client;

void setup() {
  // Bridge startup, set up comm between arduino an wifi module
  Bridge.begin();
  delay(100);
  pinMode(0, INPUT_PULLUP);

  // Listen for incoming connections
  // Arduino acts as a TCP/IP server
  server.begin();

  // Initialize Output PORTS 
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);

  // Greet arduino total control on top of the app 
  server.println("Thanks for your support!"); 
}


void loop() {
  static int disconnectedTimer = 0;
  int appData = -1; 
  delay(1);
  
  // =========================================================== 
  // This is the point were you get data from the App 
  // Get clients coming from server
  if(client){
    disconnectedTimer = 0;
    appData = client.read();   // Get a byte from app, if available 
  }
  else{
    disconnectedTimer++; 
    client = server.accept();
  }
  
  if(disconnectedTimer > 5000){ // if 5 seconds have elapsed being disconnected, or without getting data from app
    disconnectedTimer = 0;
    Bridge.begin();
    server.begin(); // reset the server
  }
  
  switch(appData){ 
  case 'A': // Turn on pin 2
    digitalWrite(2, HIGH);
    break; 
  
  case 'a': // turn off pin 2
    digitalWrite(2, LOW);
    break; 
  
  case 'B': 
    digitalWrite(3, HIGH);
    break; 
  
  case 'b': 
    digitalWrite(3, LOW);
    break; 
  
  case 'C': 
    digitalWrite(4, HIGH);
    break; 
  
  case 'c': 
    digitalWrite(4, LOW);
    break; 
  
  case 'D': 
    digitalWrite(5, HIGH);
    break; 
  
  case 'd': 
    digitalWrite(5, LOW);
    break; 
  
  case 'E': 
    digitalWrite(6, HIGH);
    break; 
  
  case 'e': 
    digitalWrite(6, LOW);
    break; 
    
  case CMD_SPECIAL: 
    // I hope you did not enabled accelerometer or seek bars, so this will never be called
    break; 

  case CMD_ALIVE: 
    // Character '[' is received every 2.5s, use 
    // this event to tell the android we are still here
    server.println("Please move to example code"); 
    break; 
  }   
  // ========================================================== 
}
