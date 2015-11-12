/*Arduino Total Control for Advanced programers
 Basic functions to control and display information into the app.
 This code works for every arduino board.
 
 * Ethernet shield Module attached
 - For UNO pins 10, 11, 12 and 13 are dedicated
 - For MEGA pins 10, 50, 51 and 52 are dedicated
 * Controls 8 relays, appliances, circuits, etc. (for n relays check uno/mega code)
 * A button from GND to A5 explains how to manualy turn on/off relay 1
 * Relays connected to RelayPinss[MAX_RELAYS]
 * Take analog samples from sensors connected to A0 to A4 
 * Servos attached to pin 8 and 9, they will move with phone accelerometer
 * Relay states are remembered
 
 To send data to app use tags:
 For buttons: (<ButnXX:Y\n), XX 0 to 19 is the button number, Y 0 or 1 is the state
 Example: server.println("<Butn05:1"); will turn the app button 5 on
 
 For texts: <TextXX:YYYY\n, XX 0 to 19 is the text number, YYYY... is the string to be displayed
 Example: server.println("<Text01:A1: 253"); will display the text "A1: 253" at text 1
 
 For images: <ImgsXX:Y\n, XX 0 to 19 is the image number, Y is the image state(0, 1 or 2)to be displayed
 Example: server.println("<Imgs02:1"); will change image 2 to the pressed state
 
 For sound alarms: <Alrm00 will make the app beep.
 
 Make the app talk: Text to Speech tag <TtoS0X:YYYY\n, X is 0 for english and 1 for your default language, YYYY... is any string
 Example: Serial.println("<TtoS00:Hello world");

 Change the seek bar values in app: <SkbX:YYY\n, where X is the seek bar number from 0 to 7, and YYY is the seek bar value
 
 If a  no tag new line ending string is sent, it will be displayed at the top of the app
 Example: server.println("Hello Word"); "Hello Word" will be displayed at the top of the app
 
  Special information can be received from app, for example sensor data and seek bar info:
 * "<PadXxx:YYY\n, xx 00 to 24 is the touch pad number, YYY is the touch pad X axis data 0 to 255
 * "<PadYxx:YYY\n, xx 00 to 24 is the touch pad number, YYY is the touch pad Y axis data 0 to 255
 * "<SkbX:SYYYYY\n", X 0 to 7 is the seek bar number, YYYYY is the seek bar value from 0 to 255
 * "<AccX:SYYYYY\n", X can be "X,Y or Z" is the accelerometer axis, YYYYY is the accelerometer value 
    in m/s^2 multiplied by 100, example: 981 == 9.81m/s^2
 * "S" is the value sign (+ or -)
 
 Author: Juan Luis Gonzalez Bello 
 Date: March 2015   
 Get the app: https://play.google.com/store/apps/details?id=com.apps.emim.btrelaycontrol
 ** After copy-paste of this code, use Tools -> Atomatic Format 
 */

#include <EEPROM.h> 
#include <SPI.h> 
#include <Ethernet.h> 
#include <Servo.h>

// Special commands
#define CMD_SPECIAL '<'
#define CMD_ALIVE   '['

// Number of relays
#define MAX_RELAYS 8 
// Relay 1 is at pin 2, relay 2 is at pin 3 and so on.
int RelayPins[MAX_RELAYS]  = {
  2, 3, 4, 5, 6, 7, 10, 13};
// Relay 1 will report status to toggle button and image 1, relay 2 to button 2 and so on.
String RelayAppId[] = {
  "04", "05", "06", "07", "08", "09", "10", "11"};
// Command list (turn on - off for eachr relay)
const char CMD_ON[] = {
  'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L'};
const char CMD_OFF[] = {
  'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l'};

// Used to keep track of the relay status 
int RelayStatus = 0; 
int STATUS_EEADR = 20; 

// Used to prescale sample time
int Prescaler = 0;
boolean buttonLatch = false;

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

// Data and variables received from especial command
int Accel[3] = {
  0, 0, 0}; 
int SeekBarValue[8] = {
  0,0,0,0,0,0,0,0};
Servo mServo[2]; 
int TouchPadData[24][2]; // 24 max touch pad objects, each one has 2 axis (x and Y)

void setup() {  
  pinMode(A5, INPUT);     // seet A5 as digital input
  digitalWrite(A5, HIGH); // Enable pull up register

  // start the Ethernet connection and the server: 
  Ethernet.begin(mac, ip); 
  server.begin();

  // Initialize touch pad data, 
  // this is to avoid having random numbers in them
  for(int i = 0; i < 24; i++){
    TouchPadData[i][0] = 0; //X
    TouchPadData[i][1] = 0; //Y
  }

  // Initialize Output PORTS 
  for(int i = 0; i < MAX_RELAYS; i++){ 
    pinMode(RelayPins[i], OUTPUT); 
  }

  // Initialize servos
  mServo[0].attach(8); 
  mServo[0].write(90);   
  mServo[1].attach(9); 
  mServo[1].write(90);

  // Load last known status from eeprom 
  RelayStatus = EEPROM.read(STATUS_EEADR); 
  for(int i = 0; i < MAX_RELAYS; i++){ 
    // Turn on and off according to relay status
    if((RelayStatus & (1 << i)) == 0){ 
      digitalWrite(RelayPins[i], LOW);
      server.println("<Butn" + RelayAppId[i] + ":0");
    } 
    else {
      digitalWrite(RelayPins[i], HIGH); 
      server.println("<Butn" + RelayAppId[i] + ":1");      
    }  
  } 

  // Greet arduino total control on top of the app
  server.println("Thanks for your support!"); 
  
  // Make the app talk in english (lang number 00, use 01 to talk in your default language)
  server.println("<TtoS00: welcome to arduino total control");
} 

void loop() { 
  String sSample; 
  String sSampleNo;  
  int iSample; 
  int appData; 

  delay(1); 

  // This is true each second approx. 
  if(Prescaler++ > 1000){ 
    Prescaler = 0; // Reset prescaler 

    // Send 3 analog samples to be displayed at text tags 
    iSample = analogRead(A0);  // Take sample
    sSample = String(iSample); // Convert into string

    // Example of how to display text in top of the app
    server.println("Sensor0: " + sSample);  

    // Example of how to use text and imgs tags 
    iSample = analogRead(A1);
    sSample = String(iSample);
    server.println("<Text00:Temp1: " + sSample);
    if(iSample > 683)
      server.println("<Imgs00:1"); // Pressed state
    else if(iSample > 341)
      server.println("<Imgs00:2"); // Extra state
    else
      server.println("<Imgs00:0"); // Default state    

    iSample = analogRead(A2);
    sSample = String(iSample);
    server.println("<Text01:Speed2: " + sSample);
    if(iSample > 683)
      server.println("<Imgs01:1"); // Pressed state
    else if(iSample > 341)
      server.println("<Imgs01:2"); // Extra state
    else
      server.println("<Imgs01:0"); // Default state    

    iSample = analogRead(A3);
    sSample = String(iSample);
    server.println("<Text02:Photo3: " + sSample);
    if(iSample > 683)
      server.println("<Imgs02:1"); // Pressed state
    else if(iSample > 341)
      server.println("<Imgs02:2"); // Extra state
    else
      server.println("<Imgs02:0"); // Default state    

    iSample = analogRead(A4);
    sSample = String(iSample); 
    server.println("<Text03:Flux4: " + sSample);
    if(iSample > 683)
      server.println("<Imgs03:1"); // Pressed state
    else if(iSample > 341)
      server.println("<Imgs03:2"); // Extra state
    else
      server.println("<Imgs03:0"); // Default state      
  } 

  // =========================================================== 
  // This is the point were you get data from the App 
  client = server.available(); 
  if (client){ 
    appData = client.read(); 
  }

  switch(appData){ 
  case CMD_SPECIAL: 
    // After this function accel and seek bar values are updated
    DecodeSpecialCommand();
    
    // Example of how to use seek bar data
    analogWrite(13, TouchPadData[4][0]);
    
    // Example of how to control two servos using accel data
    mServo[0].write(((Accel[0] + 1000) * 9) / 100);
    mServo[1].write(((Accel[1] + 1000) * 9) / 100);
    break; 

  case CMD_ALIVE: 
    // Character '[' is received every 2.5s, use
    // this event to tell the android all relay states 
    server.println("Beginner's Code"); 

    for(int i = 0; i < MAX_RELAYS; i++){ 
      // Refresh button states to app (<BtnXX:Y\n)
      if(digitalRead(RelayPins[i])){ 
        server.println("<Butn" + RelayAppId[i] + ":1");
        server.println("<Imgs" + RelayAppId[i] + ":1");
      } 
      else {
        server.println("<Butn" + RelayAppId[i] + ":0"); 
        server.println("<Imgs" + RelayAppId[i] + ":0");
      }  
    } 
    // If you need to update the seek bar values uncomment the below code
    // Update seekbar Values
    /*
    for(int i = 0; i < 8; i++){
      server.println("<Skb" + String(i) + ":" + SBtoString(SeekBarValue[i]));
    }
    */
    break; 

  default:
    // If not '<' or '[' then appData may be for relays
    for(int i = 0; i < MAX_RELAYS; i++){
      if(appData == CMD_ON[i]){
        // Example of how to make beep alarm sound
        server.println("<Alrm00");
        setRelayState(i, 1);
      }
      else if(appData == CMD_OFF[i])
        setRelayState(i, 0);
    }
  } 

  // Manual button
  if(!digitalRead(A5)){ // If button pressed
    // don't change relay status until button has been released and pressed again
    if(buttonLatch){ 
      setRelayState(0, !digitalRead(RelayPins[0])); // toggle relay 0 state
      buttonLatch = false;                       
    }
  }
  else{
    // button released, enable next push
    buttonLatch = true;
  }
  // ========================================================== 
} 

// Sets the relay state for this example
// relay: 0 to 7 relay number
// state: 0 is off, 1 is on
void setRelayState(int relay, int state){  
  if(state == 1){ 
    digitalWrite(RelayPins[relay], HIGH);           // Write ouput port
    server.println("<Butn" + RelayAppId[relay] + ":1"); // Feedback button state to app
    server.println("<Imgs" + RelayAppId[relay] + ":1"); // Set image to pressed state

    RelayStatus |= (0x01 << relay);                 // Set relay status
    EEPROM.write(STATUS_EEADR, RelayStatus);        // Save new relay status
  } 
  else {
    digitalWrite(RelayPins[relay], LOW);            // Write ouput port
    server.println("<Butn" + RelayAppId[relay] + ":0"); // Feedback button state to app
    server.println("<Imgs" + RelayAppId[relay] + ":0"); // Set image to default state

    RelayStatus &= ~(0x01 << relay);                // Clear relay status
    EEPROM.write(STATUS_EEADR, RelayStatus);        // Save new relay status
  }
}

// DecodeSpecialCommand
//
// A '<' flags a special command comming from App. Use this function
// to get Accelerometer data (and other sensors in the future)
// Input:
//   None
// Output:
//   None
void DecodeSpecialCommand(){
  // Read the whole command
  String thisCommand = Readln();

  // First 5 characters will tell us the command type
  String commandType = thisCommand.substring(0, 5);

  if(commandType.equals("AccX:")){
    // Next 6 characters will tell us the command data
    String commandData = thisCommand.substring(5, 11);
    if(commandData.charAt(0) == '-') // Negative acceleration
      Accel[0] = -commandData.substring(1, 6).toInt();
    else
      Accel[0] = commandData.substring(1, 6).toInt();
  }

  if(commandType.equals("AccY:")){
    // Next 6 characters will tell us the command data
    String commandData = thisCommand.substring(5, 11);
    if(commandData.charAt(0) == '-') // Negative acceleration
      Accel[1] = -commandData.substring(1, 6).toInt();
    else
      Accel[1] = commandData.substring(1, 6).toInt();
  }

  if(commandType.equals("AccZ:")){
    // Next 6 characters will tell us the command data
    String commandData = thisCommand.substring(5, 11);
    if(commandData.charAt(0) == '-') // Negative acceleration
      Accel[2] = -commandData.substring(1, 6).toInt();
    else
      Accel[2] = commandData.substring(1, 6).toInt();
  }

  if(commandType.substring(0, 4).equals("PadX")){
    // Next 2 characters will tell us the touch pad number
    int padNumber = thisCommand.substring(4, 6).toInt();
    // Next 3 characters are the X axis data in the message
    String commandData = thisCommand.substring(8, 13);
    TouchPadData[padNumber][0] = commandData.toInt();
  }

  if(commandType.substring(0, 4).equals("PadY")){
    // Next 2 characters will tell us the touch pad number
    int padNumber = thisCommand.substring(4, 6).toInt();
    // Next 3 characters are the Y axis data in the message
    String commandData = thisCommand.substring(8, 13);
    TouchPadData[padNumber][1] = commandData.toInt();
  }

  if(commandType.substring(0, 3).equals("Skb")){
    // Next 6 characters will tell us the command data
    String commandData = thisCommand.substring(5, 11);
    int sbNumber = commandType.charAt(3) & ~0x30;
    SeekBarValue[sbNumber] = commandData.substring(1, 6).toInt();
  }
}

// Readln  
// Use this function to read a String line from Bluetooth 
// returns: String message, note that this function will pause the program 
//          until a hole line has been read. 
String Readln(){ 
  char inByte = -1; 
  String message = ""; 

  while(inByte != '\n'){ 
    inByte = -1; 

    client = server.available(); 
    if (client)
      inByte = client.read(); 

    if(inByte != -1) 
      message.concat(String(inByte)); 
  } 

  return message; 
}


