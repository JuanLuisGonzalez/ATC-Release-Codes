/*Arduino Total Control for Advanced programers
 REMEMBER TO REMOVE BLUETOOT WHEN UPLOADING SKETCH
 Functions to control and display information into the app.
 This code works for every arduino board with bluetooth HM10 module connected to Serial.

 * Bluetooth Module attached to Serial port
 * Controls 6 relays connected to RelayPins[MAX_RELAYS]
 * Take analog samples from sensors connected from AnalogInputs[MAX_A_INPUTS]
 * Buttons from GND to DigitalInputs[MAX_D_INPUTS] explain how to manualy turn on/off relays

 How to enable analog relay trigger
 Use TriggerRelayEnable to enable which analog inputs will trigger their respective relay
 Use TriggerThreshold to set up the trigger level

 To send data to app use tags:
 For buttons: (<ButnXX:Y\n), XX 0 to 19 is the button number, Y 0 or 1 is the state
 Example: Serial.println("<Butn05:1"); will turn the app button 5 on

 For texts: <TextXX:YYYY\n, XX 0 to 19 is the text number, YYYY... is the string to be displayed
 Example: Serial.println("<Text01:A1: 253"); will display the text "A1: 253" at text 1

 For images: <ImgsXX:Y\n, XX 0 to 19 is the image number, Y is the image state(0, 1 or 2)to be displayed
 Example: Serial.println("<Imgs02:1"); will change image 2 to the pressed state

 Make sound alarm: <Alrm00\n will make the app beep.

 Make the app vibrate: <Vibr00:YYY\n, YYY is a number from 000 to 999, and represents the vibration time in ms

 Make the app talk: Text to Speech tag <TtoS0X:YYYY\n, X is 0 for english and 1 for your default language, YYYY... is any string
 Example: Serial.println("<TtoS00:Hello world");

 Change the seek bar values in app: <SkbX:YYY\n, where X is the seek bar number from 0 to 7, and YYY is the seek bar value

 Use <Abar tags to modify the current analog bar values inapp: <AbarXX:YYY\n, where XX is a number from 0 to 11, the bar number,
 and YYY is a 3 digit integer from 000 to 255, the bar value

 Use <Logr tags to store string data in the app temporary log: "<Logr00:" + "any string" + "\n"

 If a no-tag newline ending string is sent, it will be displayed at the top of the app
 Example: Serial.println("Hello Word"); "Hello Word" will be displayed at the top of the app

 Special information can be received from app, for example touch pad, sensor data and seek bar info:
 * "<PadXxx:YYY\n, xx 00 to 24 is the touch pad number, YYY is the touch pad X axis data 0 to 255
 * "<PadYxx:YYY\n, xx 00 to 24 is the touch pad number, YYY is the touch pad Y axis data 0 to 255
 * "<SkbX:SYYYYY\n", X 0 to 7 is the seek bar number, YYYYY is the seek bar value from 0 to 255
 * "<AccX:SYYYYY\n", X can be "X,Y or Z" is the accelerometer axis, YYYYY is the accelerometer value
    in m/s^2 multiplied by 100, example: 981 == 9.81m/s^2
 * "S" is the value sign (+ or -)

 TIP: To select another serial port use ctr+f, find: Serial change to Serial
 Get the app: https://play.google.com/store/apps/details?id=com.apps.emim.btrelaycontrol

/* General purpose input / ouotput BLE board
 Bluetooth Module attached to Serial port
 Controls 6 relays connected to RelayPins[MAX_RELAYS]
 Take analog samples from sensors connected from AnalogInputs[MAX_A_INPUTS]
 Buttons from GND to DigitalInputs[MAX_D_INPUTS] explain how to manualy turn on/off relays

 Author: Juan Luis Gonzalez Bello
 Date: june 2017
 */ 

#include <EEPROM.h>

// Baud rate for bluetooth module
// (Default 9600 for most modules)
#define BAUD_RATE 115200 // TIP> Set your bluetooth baud rate to 115200 for better performance

// Special commands
#define CMD_SPECIAL '<'
#define CMD_ALIVE   '['

#define UPDATE_FAIL  0
#define UPDATE_ACCEL 1
#define UPDATE_SKBAR 2
#define UPDATE_TPAD  3
#define UPDATE_SPCH  4

/*
 * Relay outputs config
 */
#define MAX_RELAYS 6
// Relay 1 is at pin 8, relay 2 is at pin 9 and so on.
int RelayPins[MAX_RELAYS]  = {2, 3, 4, 5, 6, 7};
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
int DigitalInputs[MAX_D_INPUTS] = {8, 9, 10, 11, 12, 13};
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

void setup() {
  // Initialize touch pad data,
  // this is to avoid having random numbers in them
  for (int i = 0; i < 24; i++) {
    TouchPadData[i][0] = 0; //X
    TouchPadData[i][1] = 0; //Y
  }

  // initialize BT Serial port
  Serial.begin(BAUD_RATE);

  // Initialize digital input ports with input pullup
  for (int i = 0; i < MAX_D_INPUTS; i++) {
    pinMode(DigitalInputs[i], INPUT_PULLUP);
    Serial.print("<Text" + DIAppId[i] + ":Pin " + String(DigitalInputs[i]));
    delay(20);
  }

  // Initialize analog input ports
  for (int i = 0; i < MAX_A_INPUTS; i++) {
    pinMode(AnalogInputs[i], INPUT);
  }

  // Initialize relay output ports
  for (int i = 0; i < MAX_RELAYS; i++) {
    pinMode(RelayPins[i], OUTPUT);
    digitalWrite(RelayPins[i], HIGH); // active low relays
    Serial.print("<Text" + RelayAppId[i] + ":Pin " + String(RelayPins[i]));
    delay(20);
  }

  // Load last known status from eeprom
  RelayStatus = EEPROM.read(STATUS_EEADR);
  for (int i = 0; i < MAX_RELAYS; i++) {
    // Turn on and off according to relay status
    if ((RelayStatus & (1 << i)) == 0) {
      digitalWrite(RelayPins[i], HIGH);
      Serial.print("<Butn" + RelayAppId[i] + ":0");
      delay(20);
      Serial.print("<Imgs" + RelayAppId[i] + ":0");
      delay(20);
    }
    else {
      digitalWrite(RelayPins[i], LOW);
      Serial.print("<Butn" + RelayAppId[i] + ":1");
      delay(20);
      Serial.print("<Imgs" + RelayAppId[i] + ":1");
      delay(20);
    }
  }

  // Greets on top of the app
  Serial.print("ATC Expert BT");
  delay(20);

  // Make the app talk in english (lang number 00, use 01 to talk in your default language)
  Serial.print("<TtoS00:Welcome ATC");
  delay(20);
}

void loop() {
  int appData;
  int testRelay;
  int tagType;
  static int analogPrescaler = 0;
  static int analogPrescaler1 = 0;
  static int digitalPrescaler = 0;
  static int aliveRefreshPrescaler = 0;
  delay(1);

  // ===========================================================
  // This is true each 1/4 second approx.
  if (analogPrescaler++ > 250) {
    analogPrescaler = 0; // Reset prescaler

    // Take analog samples and send to app once at a time
    int sample = analogRead(AnalogInputs[analogPrescaler1]);
    // Trigger relay output if feature enabled
    if (TriggerRelayEnable[analogPrescaler1]) {
      if (sample > TriggerThreshold[analogPrescaler1]) {
        // Example of how to make beep alarm sound
        Serial.print("<Alrm00");
        delay(15);
        setRelayState(analogPrescaler1, 1);
      }
      else
        setRelayState(analogPrescaler1, 0);
    }
    // Use <Text tags to display alphanumeric information in app
    Serial.print("<Text" + AIAppId[analogPrescaler1] + ":" + "An: " + String(sample));
    delay(15);
    // Use <Imgs tags to dinamically change pictures in app
    Serial.print("<Imgs" + AIAppId[analogPrescaler1] +  EvaluateAnalogRead(sample));
    delay(15);
    // Use <Abar tags to change analog bar levels from 0 to 255
    Serial.print("<Abar" + RelayAppId[analogPrescaler1] +  ":" + myIntToString(sample >> 2));
    delay(15);
    analogPrescaler1 = ++analogPrescaler1 % MAX_A_INPUTS;
  }

  // ===========================================================
  // This is the point were you get data from the App
  appData = Serial.read();   // Get a byte from app, if available
  switch (appData) {
    case CMD_SPECIAL:
      // Special command received
      tagType = DecodeSpecialCommand();

      if (tagType == UPDATE_SKBAR) {
        // Example of how to use seek bar data to dim a LED connected in pin 13
        analogWrite(3, SeekBarValue[0]);
      }

      if (tagType == UPDATE_SPCH) {
        // Example of how to use speech recorder
        testRelay = 0; // this is the relay number which will turn off or on with voice
        Serial.print("<Text" + RelayAppId[testRelay] + ":" + SpeechRecorder); // display received voice command below relay picture
        delay(20);
        if (SpeechRecorder.equals("Apagar")) {
          setRelayState(testRelay, 0);
        }
        if (SpeechRecorder.equals("encender")) {
          setRelayState(testRelay, 1);
        }
      }
      break;

    case CMD_ALIVE:
      // Character '[' is received every 2.5s, use
      // this event to refresh the android all relay states
      // Refresh button states to app (<BtnXX:Y\n)
      if (!digitalRead(RelayPins[aliveRefreshPrescaler])) {
        Serial.print("<Butn" + RelayAppId[aliveRefreshPrescaler] + ":1");
        delay(15);
        Serial.print("<Imgs" + RelayAppId[aliveRefreshPrescaler] + ":1");
        delay(15);
      }
      else {
        Serial.print("<Butn" + RelayAppId[aliveRefreshPrescaler] + ":0");
        delay(15);
        Serial.print("<Imgs" + RelayAppId[aliveRefreshPrescaler] + ":0");
        delay(15);
      }
      // Update one relay at a time
      aliveRefreshPrescaler = (++aliveRefreshPrescaler) % MAX_RELAYS;
      // Greets on top of the app
      Serial.print("ATC Expert BLE");
      delay(15);
      break;

    default:
      // If not '<' or '[' then appData may be for turning on or off relays
      for (int i = 0; i < MAX_RELAYS; i++) {
        if (appData == CMD_ON[i]) {
          // Example of how to make phone vibrate
          Serial.print("<TtoS00:light on"); // 16 char
          delay(20);
          //Serial.print("<Vibr00:100");
          // Example of how to make beep alarm sound
          Serial.print("<Alrm00"); // 7 char
          delay(20);
          setRelayState(i, 1);
          //Serial.print("<Logr00: Command: " + String(CMD_ON[i]) + " received");
          //Serial.print("<Logr00: Relay: " + String(i) + " turned on");
        }
        else if (appData == CMD_OFF[i]) {
          Serial.print("<TtoS00:light off"); // 17 char
          delay(20);
          setRelayState(i, 0);
          //Serial.print("<Logr00: Command: " + String(CMD_OFF[i]) + " received");
          //Serial.print("<Logr00: Relay: " + String(i) + " turned off");
        }
      }
  }

  // ==========================================================================
  // Manual buttons
  // Digital inputs are used to toggle relay outputs in board
  // this condition is true each 1/10 of a second approx
  if (digitalPrescaler++ > 100) {
    digitalPrescaler = 0;
    for (int i = 0; i < MAX_D_INPUTS; i++) {
      if (!digitalRead(DigitalInputs[i])) { // If button pressed
        // don't change status until button has been released and pressed again
        if (DigitalLatch[i]) {
          DIStatus[i] = !DIStatus[i];
          if (DIStatus[i]) {
            Serial.print("<Imgs" + DIAppId[i] + ":1"); // Set image to pressed state
            delay(20);
          }
          else {
            Serial.print("<Imgs" + DIAppId[i] + ":0"); // Set image to default state
            delay(20);
          }
          // Uncomment line below if you want to turn on and off relays using physical buttons
          setRelayState(i, !digitalRead(RelayPins[i])); // toggle relay 0 state
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
    digitalWrite(RelayPins[relay], LOW);           // Write ouput port
    ///Serial.print("<Butn" + RelayAppId[relay] + ":1"); // Feedback button state to app
    Serial.print("<Imgs" + RelayAppId[relay] + ":1"); // Set image to pressed state
    delay(15);
    RelayStatus |= (0x01 << relay);                 // Set relay status
    EEPROM.write(STATUS_EEADR, RelayStatus);        // Save new relay status
  }
  else {
    digitalWrite(RelayPins[relay], HIGH);            // Write ouput port
    //Serial.print("<Butn" + RelayAppId[relay] + ":0"); // Feedback button state to app
    Serial.print("<Imgs" + RelayAppId[relay] + ":0"); // Set image to default state
    delay(15);
    RelayStatus &= ~(0x01 << relay);                // Clear relay status
    EEPROM.write(STATUS_EEADR, RelayStatus);        // Save new relay status
  }
}

// Convert integer integer into 3 digit string
String myIntToString(int number) {
  if (number < 10) {
    return "00" + String(number);
  }
  else if (number < 100) {
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

/**
 * DecodeSpecialCommand
 * A '<' flags a special command comming from App. Use this function
 * to get Accelerometer data (and other sensors in the future)
 * Input:    None
 * Output:   tagType, is the special command type received
 */
int DecodeSpecialCommand() {
  int tagType = UPDATE_FAIL;

  // Read the whole command
  String thisCommand = Readln();

  // First 5 characters will tell us the command type
  String commandType = thisCommand.substring(0, 5);

  if (commandType.equals("AccX:")) {
    // Next 6 characters will tell us the command data
    String commandData = thisCommand.substring(5, 11);
    if (commandData.charAt(0) == '-') // Negative acceleration
      Accel[0] = -commandData.substring(1, 6).toInt();
    else
      Accel[0] = commandData.substring(1, 6).toInt();
    tagType = UPDATE_ACCEL;
  }

  if (commandType.equals("AccY:")) {
    // Next 6 characters will tell us the command data
    String commandData = thisCommand.substring(5, 11);
    if (commandData.charAt(0) == '-') // Negative acceleration
      Accel[1] = -commandData.substring(1, 6).toInt();
    else
      Accel[1] = commandData.substring(1, 6).toInt();
    tagType = UPDATE_ACCEL;
  }

  if (commandType.equals("AccZ:")) {
    // Next 6 characters will tell us the command data
    String commandData = thisCommand.substring(5, 11);
    if (commandData.charAt(0) == '-') // Negative acceleration
      Accel[2] = -commandData.substring(1, 6).toInt();
    else
      Accel[2] = commandData.substring(1, 6).toInt();
    tagType = UPDATE_ACCEL;
  }

  if (commandType.substring(0, 4).equals("PadX")) {
    // Next 2 characters will tell us the touch pad number
    int padNumber = thisCommand.substring(4, 6).toInt();
    // Next 3 characters are the X axis data in the message
    String commandData = thisCommand.substring(8, 13);
    TouchPadData[padNumber][0] = commandData.toInt();
    tagType = UPDATE_TPAD;
  }

  if (commandType.substring(0, 4).equals("PadY")) {
    // Next 2 characters will tell us the touch pad number
    int padNumber = thisCommand.substring(4, 6).toInt();
    // Next 3 characters are the Y axis data in the message
    String commandData = thisCommand.substring(8, 13);
    TouchPadData[padNumber][1] = commandData.toInt();
    tagType = UPDATE_TPAD;
  }

  if (commandType.substring(0, 3).equals("Skb")) {
    // Next 6 characters will tell us the command data
    String commandData = thisCommand.substring(5, 11);
    int sbNumber = commandType.charAt(3) & ~0x30;
    SeekBarValue[sbNumber] = commandData.substring(1, 6).toInt();
    tagType = UPDATE_SKBAR;
  }

  if (commandType.equals("StoT:")) {
    // Next characters are the converted speech
    SpeechRecorder = thisCommand.substring(5, thisCommand.length() - 1); // there is a trailing character not known
    tagType = UPDATE_SPCH;
  }
  return tagType;
}

// Readln
// Use this function to read a String line from Bluetooth
// returns: String message, note that this function will pause the program
//          until a hole line has been read.
String Readln() {
  char inByte = -1;
  String message = "";

  while (inByte != '\n') {
    inByte = -1;

    if (Serial.available() > 0)
      inByte = Serial.read();

    if (inByte != -1)
      message.concat(String(inByte));
  }

  return message;
}
