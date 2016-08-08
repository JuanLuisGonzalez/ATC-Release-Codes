/*Arduino Total Control for Newbies 
 REAL basic functions for controlling stuff via ATC app and ethernet shield.
 Please use the "example" Codes to get all the awesome features the app has!!!
 If using this sketch, DO NOT ENABLE accelerometer data in the app or seek bars or funny stuff will happen :) 
 
 * Ethernet shield Module attached
 - For UNO pins 10, 11, 12 and 13 are dedicated
 - For MEGA pins 10, 50, 51 and 52 are dedicated
 
 * Controls 5 relays (digital outputs) with the first 5 buttons in the app
 
 Author: Juan Luis Gonzalez Bello 
 Date: Feb 2016   
 Get the app: https://play.google.com/store/apps/details?id=com.apps.emim.btrelaycontrol 
 */

#include <SPI.h> 
#include <Ethernet.h> 

// Special commands
#define CMD_SPECIAL '<'
#define CMD_ALIVE   '['

// Enter a MAC address and IP address for your controller below. 
// The IP address will be dependent on your local network: 
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; 
IPAddress ip(192,168,1,60); 

// Initialize the Ethernet server library 
// with the IP address and port you want to use 
// (port 80 is default for HTTP): 
EthernetServer server(80); 
EthernetClient client; 

void setup() {  
  // start the Ethernet connection and the server: 
  Ethernet.begin(mac, ip); 
  server.begin();

  // Initialize Output PORTS 
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);

  server.println("Thanks for your support");
} 

void loop() { 
  int appData = -1; 

  // =========================================================== 
  // This is the point were you get data from the App 
  client = server.available(); 
  if (client){ 
    appData = client.read(); 
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
    server.println("Hello ATC!"); 
    break; 
  } 
  // ========================================================== 
} 
