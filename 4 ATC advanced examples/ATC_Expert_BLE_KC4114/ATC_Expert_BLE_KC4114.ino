/*Arduino Total Control for Advanced programers
 REMEMBER TO REMOVE BLUETOOTH WHEN UPLOADING SKETCH
 Functions to control and display information in the app. 
 This code works for every arduino board with Kc4114 bluetooth module connected to Serial.

 * 4114 Bluetooth Module attached to Serial port
 * Controls 6 relays connected to RelayPins[MAX_RELAYS]
 * Take analog samples from sensors connected from A0 to A5
 * Buttons connected from GND to DigitalInputs explains how to manualy turn on/off relays
 * Relay states are remembered

 How to enable analog relay trigger
 This features enable turning on and off relays when determinate analog inputs are out of limits
 Use TriggerRelayEnable to enable wich analog inputs will trigger their respective relay
 Use TriggerThreshold to set up the trigger level

 To send data to app use tags:
 For buttons: (<ButnXX:Y\n), XX 0 to 19 is the button number, Y 0 or 1 is the state
 Example: Serial.println("<Butn05:1"); will turn the app button 5 on

 For texts: <TextXX:YYYY\n, XX 0 to 19 is the text number, YYYY... is the string to be displayed
 Example: Serial.println("<Text01:A1: 253"); will display the text "A1: 253" at text 1

 For images: <ImgsXX:Y\n, XX 0 to 19 is the image number, Y is the image state(0, 1 or 2)to be displayed
 Example: Serial.println("<Imgs02:1"); will change image 2 to the pressed state

 For sound alarms: <Alrm00 will make the app beep.

 Make the app vibrate: <Vibr00:YYY\n, YYY is a number from 000 to 999, and represents the vibration time in ms

 Make the app talk: Text to Speech tag <TtoS0X:YYYY\n, X is 0 for english and 1 for your default language, YYYY... is any string
 Example: Serial.println("<TtoS00:Hello world");

 Change the seek bar values in app: <SkbX:YYY\n, where X is the seek bar number from 0 to 7, and YYY is the seek bar value

 Use <Abar tags to modify the current analog bar values inapp: <AbarXX:YYY\n, where XX is a number from 0 to 11, the bar number, and YYY is a 3 digit ingeger from 000 to 255, the bar value

 Use <Logr tags to store string data in the app temporary log: "<Logr00:" + "any string" + "\n"

 If a no tag-new line ending string is sent, it will be displayed at the top of the app
 Example: Serial.println("Hello Word"); "Hello Word" will be displayed at the top of the app

 Special information can be received from app, for example sensor data and seek bar info:
 * "<PadXxx:YYY\n, xx 00 to 24 is the touch pad number, YYY is the touch pad X axis data 0 to 255
 * "<PadYxx:YYY\n, xx 00 to 24 is the touch pad number, YYY is the touch pad Y axis data 0 to 255
 * "<SkbX:SYYYYY\n", X 0 to 7 is the seek bar number, YYYYY is the seek bar value from 0 to 255
 * "<AccX:SYYYYY\n", X can be "X,Y or Z" is the accelerometer axis, YYYYY is the accelerometer value
    in m/s^2 multiplied by 100, example: 981 == 9.81m/s^2
 * "S" is the value sign (+ or -)

 TIP: To select another serial port use ctr+f, find: Serial change to SerialX

 Author: Juan Luis Gonzalez Bello
 Date: Mar 2016
 Get the app: https://play.google.com/store/apps/details?id=com.apps.emim.btrelaycontrol
 ** After copy-paste of this code, use Tools -> Atomatic Format
 */

#include <EEPROM.h>

// Baud rate for bluetooth module
#define BAUD_RATE 115200 // TIP> Set your bluetooth baud rate to 115200 for better performance

// Special commands
#define CMD_SPECIAL '<' // This is the begining of a special command
#define CMD_ALIVE   '[' // This command is received from the app every 2.5s
#define CMD_DATA    '+' // All buttons and images commands should start with "+"

/*
 * Relay outputs config
 */
#define MAX_RELAYS 6
// Relay 1 is at pin 8, relay 2 is at pin 9 and so on.
int RelayPins[MAX_RELAYS]  = {8, 9, 10, 11, 12, 13};
// Relay 1 will report status to toggle button and image 00, relay 2 to button 01 and so on.
String RelayAppId[MAX_RELAYS] = {"00", "01", "02", "03", "04", "05"};
// Command list (turn on - off for eachr relay)
const char CMD_ON[MAX_RELAYS] = {'A', 'B', 'C', 'D', 'E', 'F'};
const char CMD_OFF[MAX_RELAYS] = {'a', 'b', 'c', 'd', 'e', 'f'};
// Used to keep track of the relay status in eeprom
int RelayStatus = 0;
int STATUS_EEADR = 20;

/*
 * Digital input config
 */
#define MAX_D_INPUTS 6
boolean DigitalLatch[MAX_D_INPUTS] = {false, false, false, false, false, false};
boolean DIStatus[MAX_D_INPUTS] = {false, false, false, false, false, false};
int DigitalInputs[MAX_D_INPUTS] = {2, 3, 4, 5, 6, 7};
String DIAppId[MAX_D_INPUTS] = {"06", "07", "08", "09", "10", "11"};

/*
 * Analog input config
 */
#define MAX_A_INPUTS 6
int AnalogInputs[MAX_A_INPUTS] = {A0, A1, A2, A3, A4, A5};
String AIAppId[MAX_A_INPUTS] = {"12", "13", "14", "15", "16", "17"};
int TriggerRelayEnable[MAX_A_INPUTS] = {false, false, false, false, true, true};
int TriggerThreshold[MAX_A_INPUTS] = {512, 512, 512, 512, 512, 512};

// Data and variables received from especial command
int Accel[3] = {0, 0, 0};
int SeekBarValue[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int TouchPadData[24][2]; // 24 max touch pad objects, each one has 2 axis (x and Y)
String SpeechRecorder = "";

// Kc4114 specifics
// All commands from the app that are not special commands must start with + (from buttons)
String const COMMAND_RESET  = "AT Rset Set";
static const String COMMAND_BYPASS = "AT Uart Set bypass E";
static const String COMMAND_COMS   = "AT Coms Set 0";

// Ignore the incoming data when these states are received
static const String STATE_RSET   = "cmd rset";
static const String STATE_INIT   = "[init]";
static const String STATE_IDLE   = "[idle]";
static const String STATE_FAST   = "[fast]";
static const String STATE_SLOW   = "[slow]";
static const String STATE_SCAN   = "[scan]";
static const String STATE_CCONN  = "[conn]";
static const String STATE_DCON   = "[dcon]";
static const String STATE_BONDED = "[bonded]";
static const String STATE_BOND   = "Bond";
static const String STATE_CONN   = "Conn";
static const String STATE_SEND   = "send 1"; 
static const String IvalidParam  = "ErrInvalidParam";

void Kc4114_begin(long baudRate){
  Serial.begin(baudRate);
  Serial.setTimeout(50);
  Serial.println(COMMAND_RESET);
  delay(5000);
  // empty input buffer
  while(Serial.available() > 0) {
    char t = Serial.read();
  }
  Serial.println(COMMAND_BYPASS);
  Serial.println(COMMAND_COMS);
}

void Kc4114_println(String data){
  Serial.println(data);
  delay(500);
}

String Kc4114_readln(){
  char buffer[50];
  Serial.readBytesUntil('\n', buffer, 100);  
  return String(buffer);
}

void setup() {
  // Initialize kc module at baud_rate
  Kc4114_begin(BAUD_RATE);
  
  // Initialize touch pad data, 
  // this is to avoid having random numbers in them
  for(int i = 0; i < 24; i++){
    TouchPadData[i][0] = 0; //X
    TouchPadData[i][1] = 0; //Y
  }
  
  // Initialize digital input ports with input pullup
  for (int i = 0; i < MAX_D_INPUTS; i++) {
    pinMode(DigitalInputs[i], INPUT_PULLUP);
  }

  // Initialize analog input ports
  for (int i = 0; i < MAX_A_INPUTS; i++) {
    pinMode(AnalogInputs[i], INPUT);
  }

  // Initialize relay output ports
  for (int i = 0; i < MAX_RELAYS; i++) {
    pinMode(RelayPins[i], OUTPUT);
  }

  // Load last known status from eeprom
  RelayStatus = EEPROM.read(STATUS_EEADR);
  for (int i = 0; i < MAX_RELAYS; i++) {
    // Turn on and off according to relay status
    if ((RelayStatus & (1 << i)) == 0) {
      digitalWrite(RelayPins[i], LOW);
      Kc4114_println("<Butn" + RelayAppId[i] + ":0");
      Kc4114_println("<Imgs" + RelayAppId[i] + ":0");
    }
    else {
      digitalWrite(RelayPins[i], HIGH);
      Kc4114_println("<Butn" + RelayAppId[i] + ":1");
      Kc4114_println("<Imgs" + RelayAppId[i] + ":1");
    }
  }

  // Greets on top of the app
  Kc4114_println("ATC Expert BLE");

  // Make the app talk in english (lang number 00, use 01 to talk in your default language)
  Kc4114_println("<TtoS00:Welcome to ATC expert");
}

void loop() {
  String appData = "";
  int testRelay;
  static int analogPrescaler = 0;
  static int digitalPrescaler = 0;
  delay(1);

  // ===========================================================
  // This is true each 1/2 second approx.
  if (analogPrescaler++ > 500) {
    analogPrescaler = 0; // Reset prescaler

    // Take analog samples and send to app
    for (int i = 0; i < MAX_A_INPUTS; i++) {
      int sample = analogRead(AnalogInputs[i]);
      // Trigger relay output if feature enabled
      if(TriggerRelayEnable[i]){
        if(sample > TriggerThreshold[i]){
          // Example of how to make beep alarm sound
          Kc4114_println("<Alrm00");
          setRelayState(i, 1);
        }
        else
          setRelayState(i, 0);
      }
      // Use <Text tags to display alphanumeric information in app
      Kc4114_println("<Text" + AIAppId[i] + ":" + "An: " + String(sample));
      // Use <Imgs tags to dinamically change pictures in app
      Kc4114_println("<Imgs" + AIAppId[i] +  EvaluateAnalogRead(sample));
      // Use <Abar tags to change analog bar levels from 0 to 255
      Kc4114_println("<Abar" + RelayAppId[i] +  ":" + myIntToString(sample >> 2));
    }
  }

  // ===========================================================
  // This is the point were you get data from the App
  appData = Kc4114_readln();   // Get a byte from app, if available
  switch (appData.charAt(0)) {
    case CMD_SPECIAL:
      // Special command received
      DecodeSpecialCommand(appData.substring(1));

      // Example of how to use seek bar data to dim a LED connected in pin 13
      // analogWrite(13, SeekBarValue[5]);

      // Example of how to use speech recorder
      testRelay = 0; // this is the relay number which will turn off or on with voice
      Kc4114_println("<Text" + RelayAppId[testRelay] + ":" + SpeechRecorder); // display received voice command below relay picture
      if(SpeechRecorder.equals("turn off")){
        setRelayState(testRelay, 0);
      }
      if(SpeechRecorder.equals("turn on")){
        setRelayState(testRelay, 1);
      }
      break;

    case CMD_ALIVE:
      // Character '[' is received every 2.5s, use
      // this event to refresh the android all relay states
      for (int i = 0; i < MAX_RELAYS; i++) {
        // Refresh button states to app (<BtnXX:Y\n)
        if (digitalRead(RelayPins[i])) {
          Kc4114_println("<Butn" + RelayAppId[i] + ":1");
          Kc4114_println("<Imgs" + RelayAppId[i] + ":1");
        }
        else {
          Kc4114_println("<Butn" + RelayAppId[i] + ":0");
          Kc4114_println("<Imgs" + RelayAppId[i] + ":0");
        }
      }
      // Greets on top of the app
      Kc4114_println("ATC Expert BT");
      break;

    case CMD_DATA:
      // If not '<' or '[' then appData may be for turning on or off relays
      for (int i = 0; i < MAX_RELAYS; i++) {
        if (appData.charAt(1) == CMD_ON[i]) {
          // Example of how to make phone vibrate
          Kc4114_println("<Vibr00:100");
          // Example of how to make beep alarm sound
          Kc4114_println("<Alrm00");
          setRelayState(i, 1);
          Kc4114_println("<Logr00: Command: " + String(CMD_ON[i]) + " received");
          Kc4114_println("<Logr00: Relay: " + String(i) + " turned on");
        }
        else if (appData.charAt(1) == CMD_OFF[i]){
          setRelayState(i, 0);
          Kc4114_println("<Logr00: Command: " + String(CMD_OFF[i]) + " received");
          Kc4114_println("<Logr00: Relay: " + String(i) + " turned off");
        }
      }
     break;
  }

  // ==========================================================================
  // Manual buttons
  // Here, digital inputs are used to toggle relay outputs in board
  // this condition is true each 1/10 of a second approx
  if (digitalPrescaler++ > 100) {
    digitalPrescaler = 0;
    for (int i = 0; i < MAX_D_INPUTS; i++) {
      if (!digitalRead(DigitalInputs[i])) { // If button pressed
        // don't change status until button has been released and pressed again
        if (DigitalLatch[i]) {
          DIStatus[i] = !DIStatus[i];
          if(DIStatus[i])
            Kc4114_println("<Imgs" + DIAppId[i] + ":1"); // Set image to pressed state
          else
            Kc4114_println("<Imgs" + DIAppId[i] + ":0"); // Set image to default state
          // Uncomment line below if you want to turn on and off relays using physical buttons
          //setRelayState(i, !digitalRead(RelayPins[i])); // toggle relay 0 state
          DigitalLatch[i] = false;
        }
      }
      else {
        // button released, enable next push
        DigitalLatch[i] = true;
      }
    }
  }
}

// Sets the relay state for this example
// relay: 0 to 5 relay number
// state: 0 is off, 1 is on
void setRelayState(int relay, int state) {
  if (state == 1) {
    digitalWrite(RelayPins[relay], HIGH);           // Write ouput port
    Kc4114_println("<Butn" + RelayAppId[relay] + ":1"); // Feedback button state to app
    Kc4114_println("<Imgs" + RelayAppId[relay] + ":1"); // Set image to pressed state

    RelayStatus |= (0x01 << relay);                 // Set relay status
    EEPROM.write(STATUS_EEADR, RelayStatus);        // Save new relay status
  }
  else {
    digitalWrite(RelayPins[relay], LOW);            // Write ouput port
    Kc4114_println("<Butn" + RelayAppId[relay] + ":0"); // Feedback button state to app
    Kc4114_println("<Imgs" + RelayAppId[relay] + ":0"); // Set image to default state

    RelayStatus &= ~(0x01 << relay);                // Clear relay status
    EEPROM.write(STATUS_EEADR, RelayStatus);        // Save new relay status
  }
}

// Convert integer integer into 3 digit string
String myIntToString(int number){
  if(number < 10){
    return "00" + String(number);
  }
  else if(number < 100){
    return "0" + String(number);
  }
  else
    return String(number);
}

// Evaluate analog read
// Use this function to tell the app if the analog sample is good to display picture in state 0, 1, or 2
String EvaluateAnalogRead(int analogSample) {
  if (analogSample < 330) {
    return ":0"; // Default  state
  }
  else if (analogSample < 660) {
    return ":1"; // Pressed state
  }
  else {
    return ":2"; // Extra state
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
void DecodeSpecialCommand(String message) {
  // Read the whole command
  String thisCommand = message;

  // First 5 characters will tell us the command type
  String commandType = thisCommand.substring(0, 5);

  if (commandType.equals("AccX:")) {
    // Next 6 characters will tell us the command data
    String commandData = thisCommand.substring(5, 11);
    if (commandData.charAt(0) == '-') // Negative acceleration
      Accel[0] = -commandData.substring(1, 6).toInt();
    else
      Accel[0] = commandData.substring(1, 6).toInt();
  }

  if (commandType.equals("AccY:")) {
    // Next 6 characters will tell us the command data
    String commandData = thisCommand.substring(5, 11);
    if (commandData.charAt(0) == '-') // Negative acceleration
      Accel[1] = -commandData.substring(1, 6).toInt();
    else
      Accel[1] = commandData.substring(1, 6).toInt();
  }

  if (commandType.equals("AccZ:")) {
    // Next 6 characters will tell us the command data
    String commandData = thisCommand.substring(5, 11);
    if (commandData.charAt(0) == '-') // Negative acceleration
      Accel[2] = -commandData.substring(1, 6).toInt();
    else
      Accel[2] = commandData.substring(1, 6).toInt();
  }

  if (commandType.substring(0, 4).equals("PadX")) {
    // Next 2 characters will tell us the touch pad number
    int padNumber = thisCommand.substring(4, 6).toInt();
    // Next 3 characters are the X axis data in the message
    String commandData = thisCommand.substring(8, 13);
    TouchPadData[padNumber][0] = commandData.toInt();
  }

  if (commandType.substring(0, 4).equals("PadY")) {
    // Next 2 characters will tell us the touch pad number
    int padNumber = thisCommand.substring(4, 6).toInt();
    // Next 3 characters are the Y axis data in the message
    String commandData = thisCommand.substring(8, 13);
    TouchPadData[padNumber][1] = commandData.toInt();
  }

  if (commandType.substring(0, 3).equals("Skb")) {
    // Next 6 characters will tell us the command data
    String commandData = thisCommand.substring(5, 11);
    int sbNumber = commandType.charAt(3) & ~0x30;
    SeekBarValue[sbNumber] = commandData.substring(1, 6).toInt();
  }

  if (commandType.equals("StoT:")) {
    // Next characters are the converted speech
    SpeechRecorder = thisCommand.substring(5, thisCommand.length() - 1); // there is a trailing character not known
  }
}
