/*Arduino Total Control for Newbies 
 REMEMBER TO DISCONNECT BLUETOOT TX PIN WHEN UPLOADING SKETCH 
 REAL basic functions for controlling stuff via ATC app.
 Please use the "example" Codes to get all the awesome features the app has!!!
 If using this sketch, DO NOT ENABLE accelerometer data in the app or seek bars or funny stuff will happen :)
 
 * Bluetooth Module attached to Serial port with a baud rate of 9600
   * Controls 5 relays (digital outputs) with the first 5 buttons in the app
 
 Author: Juan Luis Gonzalez Bello 
 Date: Aug 2016   
 Get the app: https://play.google.com/store/apps/details?id=com.apps.emim.btrelaycontrol 
 */

// Special commands
#define CMD_SPECIAL '<'
#define CMD_ALIVE   '['

void setup() {  
  // initialize BT Serial port 
  Serial.begin(9600);  

  // Initialize Output PORTS 
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);

  // Greet arduino total control on top of the app 
  Serial.println("Thanks for your support!"); 
} 

void loop() { 
  int appData; 

  // =========================================================== 
  // This is the point were you get data from the App 
  appData = Serial.read();   // Get a byte from app, if available 
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
    Serial.println("Hello ATC!"); 
    break; 
  } 
  // ========================================================== 
} 
