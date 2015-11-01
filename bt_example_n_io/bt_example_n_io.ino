/*Arduino Total Control for Advanced programers
 REMEMBER TO DISCONNECT BLUETOOT TX PIN WHEN UPLOADING SKETCH
 Basic functions to control and display information into the app.This code works for every arduino board.

 * Bluetooth Module attached to Serial port
, * Controls 48 relays, appliances, circuits, etc. (for n relays check uno/mega code)
 * Relays connected to OutputPins[MAX_RELAYS]
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
 Date: November 2015
 Get the app: https://play.google.com/store/apps/details?id=com.apps.emim.btrelaycontrol
 ** After copy-paste of this code, use Tools -> Atomatic Format
 */

#include <EEPROM.h>

// Baud rate for bluetooth module
// (Default 9600 for most modules)
#define BAUD_RATE 9600 // TIP> Set your bluetooth baud rate to 115200 for better performance

// Special commands
#define CMD_SPECIAL '<'
#define CMD_ALIVE   '['

// Number of relays
#define MAX_OUTPUTS 48
#define MAX_INPUTS 3

// This array contains the outputs pin assingment (warning, most arduinos do not have this much pins)
int OutputPins[MAX_OUTPUTS]  = {2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,
                               29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49};

// This links the arduino output pin with the app item (image "01" or button "20" for example)
// first half are button IDs and second have are picture IDs
const String RelayAppId[MAX_OUTPUTS] = {"00","01","02","03","04","05","06","07","08","09","10",
                       "11","12","13","14","15","16","17","18","19","20",
                       "21","22","23",
                       "00","01","02","03","04","05","06","07","08","09","10",
                       "11","12","13","14","15","16","17","18","19","20",
                       "21","22","23"};

// Command list (turn on - off for eachr relay)
const char CMD_OUTPUT[MAX_OUTPUTS] = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r', 's','t','u',
                           'v','w','x','y','z','A','B','C','D','E', 'F','G','H','I','J','K','L','M','N','O', 'P',
                           'Q','R','S','T','U','V'};
/*                         
const char CMD_OFF[] = {"a0","b0","c0","d0","e0","f0","g0","h0","i0","j0","k0","l0","m0","n0","o0",
                        "p0","q0","r0","s0","t0","u0","v0","w0","x0","y0","z0","A0","B0","C0","D0",
                        "E0","F0","G0","H0","I0","J0","K0","L0","M0","N0","O0","P0","Q0","R0","S0",
                        "T0","U0","V0",};
*/

// Used to keep track of the relay status
unsigned long StatusToggleBtn = 0; // Stores relay stays 00 to 23
unsigned long StatusImageBtn  = 0; // Stores relay stays 24 to 48
int STATUS_EEADR = 20; // from here we will use 6 bytes (1 int = 2 bytes) 

// Data and variables received from especial command
int Accel[3] = {0, 0, 0};
int SeekBarValue[8] = {0,0,0,0,0,0,0,0};

// Save relaystate for all 48 outputs in the eeprom
void SaveRelayState(int relay, int state){
    if(relay < (MAX_OUTPUTS / 2)){// this is a toggle button state
      if(state)
        StatusToggleBtn = StatusToggleBtn | (0x01 << relay);  // Set the corresponding bit
      else
        StatusToggleBtn = StatusToggleBtn & ~(0x01 << relay); // Clear the corresponding bit
      myWrite32(STATUS_EEADR, StatusToggleBtn); // Save the status
    }
    else{ // this is a image button state
      if(state)
        StatusImageBtn = StatusImageBtn | (0x01 << (relay - (MAX_OUTPUTS / 2)));  // Set the corresponding bit
      else
        StatusImageBtn = StatusImageBtn & ~(0x01 << (relay - (MAX_OUTPUTS / 2))); // Clear the corresponding bit
      myWrite32(STATUS_EEADR + 4, StatusImageBtn); // Save the status
    }
}

void RetrieveRelayStatus(){
  // Load last known status from eeprom for each type of button
  StatusToggleBtn = myRead32(STATUS_EEADR);
  StatusImageBtn  = myRead32(STATUS_EEADR + 4);

  // Update staus in app for toggle buttons
  for(int i = 0; i < (MAX_OUTPUTS / 2); i++){
    // Turn on and off according to relay status
    if((StatusToggleBtn & (1 << i)) == 0){
      digitalWrite(OutputPins[i], LOW);
      Serial.println("<Butn" + RelayAppId[i] + ":0");
    }
    else {
      digitalWrite(OutputPins[i], HIGH);
      Serial.println("<Butn" + RelayAppId[i] + ":1");
    }
  }  

  // Update staus in app for image buttons
  for(int i = 0; i < (MAX_OUTPUTS / 2); i++){
    // Turn on and off according to relay status
    if((StatusImageBtn & (1 << i)) == 0){
      digitalWrite(OutputPins[i + (MAX_OUTPUTS / 2)], LOW);
      Serial.println("<Imgs" + RelayAppId[i + (MAX_OUTPUTS / 2)] + ":0");
    }
    else {
      digitalWrite(OutputPins[i + (MAX_OUTPUTS / 2)], HIGH);
      Serial.println("<Imgs" + RelayAppId[i + (MAX_OUTPUTS / 2)] + ":1");
    }
  } 
}

unsigned long myRead32(int address){
  int a32bitNumber = ((int)EEPROM.read(address) << 24)     | ((int)EEPROM.read(address + 1) << 16) | 
                     ((int)EEPROM.read(address + 2) <<  8) | (int)EEPROM.read(address + 3);
  return a32bitNumber;  
}

void myWrite32(int address, long number){
  EEPROM.write(address,      number & 0x000000FF);
  EEPROM.write(address + 1, (number & 0x0000FF00) >> 8);
  EEPROM.write(address + 2, (number & 0x00FF0000) >> 16);
  EEPROM.write(address + 3, (number & 0xFF000000) >> 24);
}

void setup() {
  // initialize BT Serial port
  Serial.begin(BAUD_RATE);

  // Initialize Output PORTS
  for(int i = 0; i < MAX_OUTPUTS; i++){
    pinMode(OutputPins[i], OUTPUT);
  }

  // update app and arduino pins for each of the 48 outputs
  RetrieveRelayStatus();

  // Greet arduino total control on top of the app
  Serial.println("Thanks for your support!");

  // Make the app talk in english (lang number 00, use 01 to talk in your default language)
  Serial.println("<TtoS00: welcome to arduino total control");
}

void loop() {
  int appData = -1;
  delay(1);

  // ===========================================================
  // This is the point were you get data from the App
  appData = Serial.read();   // Get a byte from app, if available
  switch(appData){
  case CMD_SPECIAL:
    // Special command received
    DecodeSpecialCommand();
    break;

  case CMD_ALIVE:
    // Character '[' is received every 2.5s, use
    // this event to tell the android all relay states
    RetrieveRelayStatus();
    break;

  default:
    // If not '<' or '[' then appData may be for relays
    for(int i = 0; i < MAX_OUTPUTS; i++){
      if(appData == CMD_OUTPUT[i]){
        char outputState = -1;
        while(outputState == -1){ // wait until next char is read
          outputState = Serial.read();
        }
        int outputStateNo = outputState & ~0x30; //convert "char" into "int"
        setRelayState(i, outputStateNo);         //set relayState 
      }
    }
  }
  // ==========================================================
}

// Sets the relay state for this example
// relay: 0 to 48 relay number
// state: 0 is off, 1 is on
void setRelayState(int relay, int state){
  if(state == 1){
    digitalWrite(OutputPins[relay], HIGH); // Write ouput port
    if(relay < (MAX_OUTPUTS / 2))
      Serial.println("<Butn" + RelayAppId[relay] + ":1"); // Feedback button state to app
    else
      Serial.println("<Imgs" + RelayAppId[relay] + ":1"); // Set image to pressed state
  }
  else {
    digitalWrite(OutputPins[relay], LOW); // Write ouput port
    if(relay < (MAX_OUTPUTS / 2))
      Serial.println("<Butn" + RelayAppId[relay] + ":0"); // Feedback button state to app
    else
      Serial.println("<Imgs" + RelayAppId[relay] + ":0"); // Set image to default state
  }
  SaveRelayState(relay, state);          // Save new relay status
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
