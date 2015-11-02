/*Arduino Total Control for Advanced programers
 REMEMBER TO DISCONNECT BLUETOOT TX PIN WHEN UPLOADING SKETCH
 Basic functions to control and display information into the app.This code works for every arduino board.

 * Bluetooth Module attached to Serial port
, * Controls 8 relays, appliances, circuits, etc. (for n relays check uno/mega code)
 * Relays connected to RelayPins[MAX_RELAYS]
 * Take analog samples from sensors connected to A0 to A4
 * A button from GND to A5 explains how to manualy turn on/off relay 0
 * Pins 8 and 9 are used to control a couple of servos via accelerometer data
 * Relay states are remembered

 To send data to app use tags:f
 For buttons: (<ButnXX:Y\n), XX 0 to 19 is the button number, Y 0 or 1 is the state
 Example: Serial.println("<Butn05:1"); will turn the app button 5 on

 For texts: <TextXX:YYYY\n, XX 0 to 19 is the text number, YYYY... is the string to be displayed
 Example: Serial.println("<Text01:A1: 253"); will display the text "A1: 253" at text 1

 For images: <ImgsXX:Y\n, XX 0 to 19 is the image number, Y is the image state(0, 1 or 2)to be displayed
 Example: Serial.println("<Imgs02:1"); will change image 2 to the pressed state

 For sound alarms: <Alrm00 will make the app beep.

 Make the app talk: Text to Speech tag <TtoS0X:YYYY\n, X is 0 for english and 1 for your default language, YYYY... is any string
 Example: Serial.println("<TtoS00:Hello world");

 If a  no tag new line ending string is sent, it will be displayed at the top of the app
 Example: Serial.println("Hello Word"); "Hello Word" will be displayed at the top of the app

 Special information can be received from app, for example sensor data and seek bar info:
 * "<SkbX:SYYYYY\n", X 0 to 7 is the seek bar number, YYYYY is the seek bar value from 0 to 255
 * "<AccX:SYYYYY\n", X can be "X,Y or Z" is the accelerometer axis, YYYYY is the accelerometer value
    in m/s^2 multiplied by 100, example: 981 == 9.81m/s^2
 * "S" is the value sign (+ or -)

 TIP: To select another serial port use ctr+f, find: Serial change to Serial

 Author: Juan Luis Gonzalez Bello
 Date: March 2015
 Get the app: https://play.google.com/store/apps/details?id=com.apps.emim.btrelaycontrol
 ** After copy-paste of this code, use Tools -> Atomatic Format
 */

#include <EEPROM.h>
#include <Servo.h>

// Baud rate for bluetooth module
// (Default 9600 for most modules)
#define BAUD_RATE 9600 // TIP> Set your bluetooth baud rate to 115200 for better performance

// Special commands
#define CMD_SPECIAL '<'
#define CMD_ALIVE   '['

// Number of relays
#define MAX_RELAYS 24
#define MAX_INPUTS 3

// Relay 1 is at pin 2, relay 2 is at pin 3 and so on.
int RelayPins[MAX_RELAYS]  = {2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25};

// Relay 1 will report status to toggle button and image 1, relay 2 to button 2 and so on.
String RelayAppId[MAX_RELAYS] = {"00","00","00","00","00","00","00","00","00","00","00","00",
                                 "00","00","00","00","00","00","00","00","00","00","00","00"};
                                 
// Command list (turn on - off for eachr relay)
const char CMD_ON[MAX_RELAYS] = {'A','B','C','D','E','F','G','H','I','J','K','L',
                                  'M','N','O','P','Q','R','S','T','U','V','W','X'};
const char CMD_OFF[MAX_RELAYS] = {'a','b','c','d','e','f','g','h','i','j','k','l',
                                    'm','n','o','p','q','r','s','t','u','v','w','x'};

// Used to keep track of the relay status
unsigned long RelayStatus = 0; // we need a 32bit variable to store all 24 status
int STATUS_EEADR = 20;         // we will use 4 bytes to store a 32 bit variable in eeprom

// Used to prescale sample time
int Prescaler = 0;
boolean buttonLatch[] = {false, false, false};
int ButtonPins[] = {A5, 27, 28};

// Data and variables received from especial command
int Accel[3] = {0, 0, 0};
int SeekBarValue[8] = {0,0,0,0,0,0,0,0};

unsigned long myRead32(int address){
  int a32bitNumber = ((int)EEPROM.read(address) << 24)     | ((int)EEPROM.read(address + 1) << 16) | 
                     ((int)EEPROM.read(address + 2) <<  8) | (int)EEPROM.read(address + 3);
  return a32bitNumber;  
}

void myWrite32(int address, long number){
  EEPROM.write(address + 3,      number & 0x000000FF);
  EEPROM.write(address + 2, (number & 0x0000FF00) >> 8);
  EEPROM.write(address + 1, (number & 0x00FF0000) >> 16);
  EEPROM.write(address + 0, (number & 0xFF000000) >> 24);
}

void setup() {
  // Set each input as digital input + pull up resistor
  for(int i = 0; i < 3; i++){
    pinMode(ButtonPins[i], INPUT);
    digitalWrite(ButtonPins[i], HIGH);
  }

  // initialize BT Serial port
  Serial.begin(BAUD_RATE);

  // Initialize Output PORTS
  for(int i = 0; i < MAX_RELAYS; i++){
    pinMode(RelayPins[i], OUTPUT);
  }

  // Load last known status from eeprom
  RelayStatus = myRead32(STATUS_EEADR);
  for(int i = 0; i < MAX_RELAYS; i++){
    // Turn on and off according to relay status
    String stringNumber = String(i);
    if((RelayStatus & (1 << i)) == 0){
      digitalWrite(RelayPins[i], LOW);
      Serial.println("<Butn" + RelayAppId[i] + ":0");
    }
    else {
      digitalWrite(RelayPins[i], HIGH);
      Serial.println("<Butn" + RelayAppId[i] + ":1");
    }
  }

  // Greet arduino total control on top of the app
  Serial.println("Thanks for your support!");

  // Make the app talk in english (lang number 00, use 01 to talk in your default language)
  Serial.println("<TtoS00: welcome to arduino total control");
}

void loop() {
  String sSample;
  String sSampleNo;
  int iSample;
  int appData;

  delay(1);

  // This is true each 1/2 second approx.
  if(Prescaler++ > 500){
    Prescaler = 0; // Reset prescaler

    // Send 5 analog samples to be displayed at text tags
    iSample = analogRead(A0);  // Take sample
    sSample = String(iSample); // Convert into string

    // Example of how to display text in top of the app
    Serial.println("Sensor0: " + sSample);

    // Example of how to use text and imgs tags
    iSample = analogRead(A1);
    sSample = String(iSample);
    Serial.println("<Text00:Temp1: " + sSample);
    if(iSample > 683)
      Serial.println("<Imgs00:1"); // Pressed state
    else if(iSample > 341)
      Serial.println("<Imgs00:2"); // Extra state
    else
      Serial.println("<Imgs00:0"); // Default state

    iSample = analogRead(A2);
    sSample = String(iSample);
    Serial.println("<Text01:Speed2: " + sSample);
    if(iSample > 683)
      Serial.println("<Imgs01:1"); // Pressed state
    else if(iSample > 341)
      Serial.println("<Imgs01:2"); // Extra state
    else
      Serial.println("<Imgs01:0"); // Default state

    iSample = analogRead(A3);
    sSample = String(iSample);
    Serial.println("<Text02:Photo3: " + sSample);
    if(iSample > 683)
      Serial.println("<Imgs02:1"); // Pressed state
    else if(iSample > 341)
      Serial.println("<Imgs02:2"); // Extra state
    else
      Serial.println("<Imgs02:0"); // Default state

    iSample = analogRead(A4);
    sSample = String(iSample);
    Serial.println("<Text03:Flux4: " + sSample);
    if(iSample > 683)
      Serial.println("<Imgs03:1"); // Pressed state
    else if(iSample > 341)
      Serial.println("<Imgs03:2"); // Extra state
    else
      Serial.println("<Imgs03:0"); // Default state
  }

  // ===========================================================
  // This is the point were you get data from the App
  appData = Serial.read();   // Get a byte from app, if available
  switch(appData){
  case CMD_SPECIAL:
    // Special command received
    DecodeSpecialCommand();
    analogWrite(13, SeekBarValue[0]);
    break;

  case CMD_ALIVE:
    // Character '[' is received every 2.5s, use
    // this event to tell the android all relay states
    for(int i = 0; i < MAX_RELAYS; i++){
      // Refresh button states to app (<BtnXX:Y\n)
      if(digitalRead(RelayPins[i])){
        Serial.println("<Butn" + RelayAppId[i] + ":1");
        Serial.println("<Imgs" + RelayAppId[i] + ":1");
      }
      else {
        Serial.println("<Butn" + RelayAppId[i] + ":0");
        Serial.println("<Imgs" + RelayAppId[i] + ":0");
      }
    }
    break;

  default:
    // If not '<' or '[' then appData may be for relays
    for(int i = 0; i < MAX_RELAYS; i++){
      if(appData == CMD_ON[i]){
        // Example of how to make beep alarm sound
        Serial.println("<Alrm00");
        setRelayState(i, 1);
      }
      else if(appData == CMD_OFF[i])
        setRelayState(i, 0);
    }
  }

  // Manual buttons
  for(int i = 0; i < MAX_INPUTS; i++){
    if(!digitalRead(ButtonPins[i])){ // If button pressed
      // don't change relay status until button has been released and pressed again
      if(buttonLatch[i]){
        setRelayState(i, !digitalRead(RelayPins[i])); // toggle relay 0 state
        buttonLatch[i] = false;
      }
    }
    else{
      // button released, enable next push
      buttonLatch[i] = true;
    }
  }
  // ==========================================================
}

// Sets the relay state for this example
// relay: 0 to 7 relay number
// state: 0 is off, 1 is on
void setRelayState(int relay, int state){
  if(state == 1){
    digitalWrite(RelayPins[relay], HIGH);           // Write ouput port
    Serial.println("<Butn" + RelayAppId[relay] + ":1"); // Feedback button state to app
    Serial.println("<Imgs" + RelayAppId[relay] + ":1"); // Set image to pressed state

    RelayStatus |= (0x01 << relay);                 // Set relay status
    myWrite32(STATUS_EEADR, RelayStatus);            // Save new relay status
  }
  else {
    digitalWrite(RelayPins[relay], LOW);            // Write ouput port
    Serial.println("<Butn" + RelayAppId[relay] + ":0"); // Feedback button state to app
    Serial.println("<Imgs" + RelayAppId[relay] + ":0"); // Set image to default state

    RelayStatus &= ~(0x01 << relay);                // Clear relay status
    myWrite32(STATUS_EEADR, RelayStatus);            // Save new relay status
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
  // Read the hole command
  String thisCommand = Readln();

  // First 5 characters will tell us the command type
  String commandType = thisCommand.substring(0, 5);

  // Next 6 characters will tell us the command data
  String commandData = thisCommand.substring(5, 11);

  if(commandType.equals("AccX:")){
    if(commandData.charAt(0) == '-') // Negative acceleration
      Accel[0] = -commandData.substring(1, 6).toInt();
    else
      Accel[0] = commandData.substring(1, 6).toInt();
  }

  if(commandType.equals("AccY:")){
    if(commandData.charAt(0) == '-') // Negative acceleration
      Accel[1] = -commandData.substring(1, 6).toInt();
    else
      Accel[1] = commandData.substring(1, 6).toInt();
  }

  if(commandType.equals("AccZ:")){
    if(commandData.charAt(0) == '-') // Negative acceleration
      Accel[2] = -commandData.substring(1, 6).toInt();
    else
      Accel[2] = commandData.substring(1, 6).toInt();
  }

  if(commandType.substring(0, 3).equals("Skb")){
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

    if (Serial.available() > 0)
      inByte = Serial.read();

    if(inByte != -1)
      message.concat(String(inByte));
  }

  return message;
}
