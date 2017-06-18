/* ATC_Expert_EXP8266
 REMEMBER TO REMOVE ESP8266 WHEN UPLOADING SKETCH
 Basic functions to control and display information into the app. This code works with any arduino board with ESP8266 connected.

 * ESP8266 Module attached to Serial port
 * Controls 6 relays connected to RelayPins[MAX_RELAYS]
 * Take analog samples from sensors connected from A0 to A5
 * Buttons from GND to DigitalInputs explains how to manualy turn on/off relays
 * Relay states are remembered

 Steps for using the ESP module
 ESP8266 module is a powerfull piece of hardware, but to start using it, you MUST follow the correct steps for wiring,
 network setting, and programming.

 Wiring:
 Most ESP modules do not have the pins identified in board, so take into account the following
 <- Antenna facig this way

 RXD   *  * Vcc
 GPIO0 *  * RST
 GPIO2 *  * CH_PD
 GND   *  * TXD

 Vcc is 3.3V
 RXD goes to your arduino Tx
 TXD goes to your arduino Rx
 GPIO2 goes to Vcc (GPIO0 and GPIO2 not connected on last test)
 CH_PD goes to Vcc
 RST   goes to Arduino Reset

 Network Set up:
 Open serial terminal (set it to your module Baud rate, and select both NL & CR
 When sending AT, module must answer "OK", dont proceed until you have the module's ok

 Main commands

 Use the following command to check connection, the module ip should appear in the second line.
 If you get 0.0.0.0 means you are not connected yet.
 AT+CIFSR

 Finnaly, this works better with 115200 baud rate,
 use: AT+CIOBAUD=115200 to set the new baud rate

 1. AT
 This is just a hello message, and if the ESP-12 is in the correct mode, it will return an "OK" message.
 2. AT+GMR
 This command returns the firmware version currently on the chip.
 3. AT+CWMODE?
 This command returns the mode of operation. If the mode is not 3, we will change it to 3 using the following command :
 AT+CWMODE=3
 This mode makes the ESP8266 behave both as a WiFi client as well as a WiFi Access point.
 4. AT+CWLAP
 The LAP (List Access Points) lists the WiFi networks around. Next, we choose our WiFi network
 5. AT+CWJAP="your_network_name","your_wifi_network_password"
 This command JAP (Join Access Point) makes the ESP-12 join your WiFi Network.
 6. AT+CIFSR
 This command returns the IP address of the ESP-12 as the second line and the gateway IP address as the first line if it managed to connect successfully


 To send data to app use tags:
 For buttons: (<ButnXX:Y\n), XX 0 to 19 is the button number, Y 0 or 1 is the state
 Example: Serial.println("<Butn05:1"); will turn the app button 5 on

 For texts: <TextXX:YYYY\n, XX 0 to 19 is the text number, YYYY... is the string to be displayed
 Example: Serial.println("<Text01:A1: 253"); will display the text "A1: 253" at text 1

 For images: <ImgsXX:Y\n, XX 0 to 19 is the image number, Y is the image state(0, 1 or 2)to be displayed
 Example: Serial.println("<Imgs02:1"); will change image 2 to the pressed state

 Make the app vibrate: <Vibr00:YYY\n, YYY is a number from 000 to 999, and represents the vibration time in ms

 For sound alarms: <Alrm00 will make the app beep.

 Make the app talk: Text to Speech tag <TtoS0X:YYYY\n, X is 0 for english and 1 for your default language, YYYY... is any string
 Example: Serial.println("<TtoS00:Hello world");

 Change the seek bar values in app: <SkbX:YYY\n, where X is the seek bar number from 0 to 7, and YYY is the seek bar value

 If a  no tag new line ending string is sent, it will be displayed at the top of the app
 Example: Serial.println("Hello Word"); "Hello Word" will be displayed at the top of the app

 Special information can be received from app, for example sensor data and seek bar info:
 * "<PadXxx:YYY\n, xx 00 to 24 is the touch pad number, YYY is the touch pad X axis data 0 to 255
 * "<PadYxx:YYY\n, xx 00 to 24 is the touch pad number, YYY is the touch pad Y axis data 0 to 255
 * "<SkbX:SYYYYY\n", X 0 to 7 is the seek bar number, YYYYY is the seek bar value from 0 to 255
 * "<AccX:SYYYYY\n", X can be "X,Y or Z" is the accelerometer axis, YYYYY is the accelerometer value
    in m/s^2 multiplied by 100, example: 981 == 9.81m/s^2
 * "S" is the value sign (+ or -)

 TIP: To select another serial port use ctr+f, find: Serial change to Serial

 Author: Juan Luis Gonzalez Bello
 Date: Mar 2016
 Get the app: https://play.google.com/store/apps/details?id=com.apps.emim.btrelaycontrol
 ** After copy-paste of this code, use Tools -> Atomatic Format
 */

#include <EEPROM.h>

// Baud rate for bluetooth module
// (Default 9600 for most modules)
#define BAUD_RATE 115200 // TIP> Set your bluetooth baud rate to 115200 for better performance
#define RX_PIN       0 // Check this out! this is to enable arduino pull up on rx pin, critical!
String sPort = "8080";

// Special commands
#define CMD_SPECIAL '<'
#define CMD_ALIVE   '['

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

String TheChannel = "0";

void setup() {
  // Initialize touch pad data,
  // this is to avoid having random numbers in them
  for (int i = 0; i < 24; i++) {
    TouchPadData[i][0] = 0; //X
    TouchPadData[i][1] = 0; //Y
  }

  // initialize WiFI module;
  myESP_Init();

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
    if ((RelayStatus & (1 << i)) == 0)
      digitalWrite(RelayPins[i], LOW);
    else
      digitalWrite(RelayPins[i], HIGH);
  }

  // Greets on top of the app
  myESP_Println("ATC Expert ESP8266", 0);

  // Make the app talk in english (lang number 00, use 01 to talk in your default language)
  myESP_Println("<TtoS00: welcome to ATC expert", 0);
}

void loop() {
  int appData = 0;
  int testRelay = 0;
  String appMessage = "";
  String boardMessage = "";
  static int analogPrescaler = 0;
  static int digitalPrescaler = 0;
  delay(1);

  // ===========================================================
  // This is true each 1/2 second approx.
  if (analogPrescaler++ > 1000) {
    analogPrescaler = 0; // Reset prescaler
    boardMessage = "";
    // Take analog samples and send to app
    for (int i = 0; i < MAX_A_INPUTS; i++) {
      int sample = analogRead(AnalogInputs[i]);
      // Trigger relay output if feature enabled
      if (TriggerRelayEnable[i]) {
        if (sample > TriggerThreshold[i])
          setRelayState(i, 1);
        else
          setRelayState(i, 0);
      }
      // Concentrate all samples information in a single data packet
      // Use <Text tags to display alphanumeric information in app
      boardMessage = boardMessage + "<Text" + AIAppId[i] + ":" + "An: " + String(sample) + "\n";
      // Use <Imgs tags to dinamically change pictures in app
      boardMessage = boardMessage + "<Imgs" + AIAppId[i] + EvaluateAnalogRead(sample) + "\n";
    }
    // Send all info in a single print
    myESP_Print(boardMessage, 0);
  }

  // ===========================================================
  // This is the point were you get data from the App
  if (Serial.available() > 10) {
    appMessage = myESP_Read(0);    // Read channel 0
    appData = appMessage.charAt(0);

    switch (appData) {
      case CMD_SPECIAL:
        // Special command received
        DecodeSpecialCommand(appMessage.substring(1));

        // Example of how to use seek bar data to dim a LED connected in pin 13
        // analogWrite(13, SeekBarValue[5]);

        // Example of how to use speech recorder
        testRelay = 0; // this is the relay number which will turn off or on with voice
        myESP_Println("<Text" + RelayAppId[testRelay] + ":" + SpeechRecorder, 0); // display received voice command below relay picture
        if (SpeechRecorder.equals("turn off")) {
          setRelayState(testRelay, 0);
        }
        if (SpeechRecorder.equals("turn on")) {
          setRelayState(testRelay, 1);
        }
        break;

      case CMD_ALIVE:
        // Character '[' is received every 2.5s, use
        // this event to refresh the android all relay states
        displayAllRelayStates();
        // Greets on top of the app
        myESP_Println("ATC Expert ESP8266", 0);
        break;

      default:
        // If not '<' or '[' then appData may be for turning on or off relays
        for (int i = 0; i < MAX_RELAYS; i++) {
          if (appData == CMD_ON[i]) {
            setRelayState(i, 1);
          }
          else if (appData == CMD_OFF[i])
            setRelayState(i, 0);
        }
    }
  }

  // ==========================================================================
  // Manual buttons
  // Here, digital inputs are used to toggle relay outputs in board
  // this condition is true each 1/10 of a second approx
  if (digitalPrescaler++ > 200) {
    digitalPrescaler = 0;
    for (int i = 0; i < MAX_D_INPUTS; i++) {
      if (!digitalRead(DigitalInputs[i])) { // If button pressed
        // don't change relay status until button has been released and pressed again
        if (DigitalLatch[i]) {
          DIStatus[i] = !DIStatus[i];
          if (DIStatus[i])
            myESP_Println("<Imgs" + DIAppId[i] + ":1", 0); // Set image to pressed state
          else
            myESP_Println("<Imgs" + DIAppId[i] + ":0", 0); // Set image to default state
          // Uncomment libe below ig you want to turn on or off relays using physical buttons
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
  String boardMessage = "";

  if (state == 1) {
    digitalWrite(RelayPins[relay], HIGH);               // Write ouput port
  
    boardMessage = boardMessage + "<Vibr00:100" + "\n"; // Example of how to make phone vibrate   
    boardMessage = boardMessage + "<Alrm00" + "\n";     // Example of how to make beep alarm sound
    boardMessage = boardMessage + "<Butn" + RelayAppId[relay] + ":1\n"; // Feedback button state to app
    boardMessage = boardMessage + "<Imgs" + RelayAppId[relay] + ":1\n"; // Set image to pressed state

    RelayStatus |= (0x01 << relay);           // Set relay status
    EEPROM.write(STATUS_EEADR, RelayStatus);  // Save new relay status
  }
  else {
    digitalWrite(RelayPins[relay], LOW);      // Write ouput port
    boardMessage = boardMessage + "<Butn" + RelayAppId[relay] + ":0\n"; // Feedback button state to app
    boardMessage = boardMessage + "<Imgs" + RelayAppId[relay] + ":0\n"; // Set image to pressed state

    RelayStatus &= ~(0x01 << relay);                // Clear relay status
    EEPROM.write(STATUS_EEADR, RelayStatus);        // Save new relay status
  }
  myESP_Print(boardMessage, 0);
}

// Display the current relay states in app
void displayAllRelayStates() {
  String boardMessage = "";
  for (int i = 0; i < MAX_RELAYS; i++) {
    if (digitalRead(RelayPins[i])) {
      boardMessage = boardMessage + "<Butn" + RelayAppId[i] + ":1\n"; // Feedback button state to app
      boardMessage = boardMessage + "<Imgs" + RelayAppId[i] + ":1\n"; // Set image to pressed state
    }
    else {
      boardMessage = boardMessage + "<Butn" + RelayAppId[i] + ":0\n"; // Feedback button state to app
      boardMessage = boardMessage + "<Imgs" + RelayAppId[i] + ":0\n"; // Set image to pressed state
    }
  }
  myESP_Print(boardMessage, 0);
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
void DecodeSpecialCommand(String thisCommand) {
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

// Readln
// Use this function to read a String line from Bluetooth
// returns: String message, note that this function will pause the program
//          until a hole line has been read.
String Readln() {
  char inByte = -1;
  String message = "";
  int timeoutTimer = 0;

  while (inByte != '\n') {
    inByte = -1;

    if (Serial.available() > 0)
      inByte = Serial.read();

    if (inByte != -1)
      message.concat(String(inByte));

    // If we dont find a complete line after x time then abort
    if (timeoutTimer++ > 10000) {
      message = "";
      break;
    }
  }

  return message;
}

/*
 Set up wifi module on Serial, esp on channel 0
 AT+CIPMUX=1
 AT+CIPSERVER=1,80
 When data received: +IPD,0,2:[ (for this exaple 0 is the channel, and 2 is the data lenght)
 Rare error down, blue module uses 1 as channel and black one uses 0
 To send data AT+CIPSEND=0,3 (0: channel, 3 data size)
 */
void myESP_Init() {
  // Initialize serial port
  Serial.begin(BAUD_RATE);
  pinMode(RX_PIN, INPUT_PULLUP);

  // Reset module
  Serial.println("AT+RST");
  delay(3000);
  // Turn ECHO off
  Serial.println("ATE0");
  delay(1000);
  // Set up AP
  Serial.println("AT+CWMODE=3");
  delay(100);
  Serial.println("AT+CWSAP=\"ESP8266\",\"12345678\",3,0");
  delay(100);
  Serial.println("AT+CIPMUX=1");
  delay(500);
  Serial.println("AT+CIPSERVER=1," + sPort);
  delay(500);
}

// Send string data to app
void myESP_Println(String data, int channel) {
  // Get data size, add 2 for nl and cr
  int dataSize = data.length() + 2;

  // submit cipsend command for channel
  Serial.println("AT+CIPSEND=" + TheChannel + "," + String(dataSize));
  Serial.flush(); // wait transmision complete
  delay(3);       // wait esp module process

  // Print actual data
  Serial.println(data);
  Serial.flush(); // wait transmision complete
  delay(20);       // wait esp module process
}

// Send string data to app
void myESP_Print(String data, int channel) {
  int dataSize = data.length();

  // submit cipsend command for channel
  Serial.println("AT+CIPSEND=" + TheChannel + "," + String(dataSize));
  Serial.flush(); // wait transmision complete
  delay(3);       // wait esp module process

  // Print actual data
  Serial.print(data);
  Serial.flush(); // wait transmision complete
  delay(20);       // wait esp module process, normally used for big data string, wait more
}

// This read function is faster
// call this function when you have minimum 9 bytes available
String myESP_Read(int channel) {
  String message = "";
  int plusIndex = -1;
  String theIPD = "";
  String dataFromClient = "";

  // Read a complete line
  dataFromClient = Readln();

  // Find the '+'
  plusIndex = dataFromClient.indexOf('+');
  if (plusIndex == -1) return message;

  // If next 3 chars are IPD then this is a good command
  theIPD = dataFromClient.substring(plusIndex + 1, plusIndex + 4);
  if (theIPD.equals("IPD")) {
    // Get the chanel the data is from
    TheChannel = dataFromClient.substring(plusIndex + 5, plusIndex + 6); 
    // Extract message
    int twoPointsIndex = dataFromClient.indexOf(':');       // find the ':' on the command
    message = dataFromClient.substring(twoPointsIndex + 1); // set the message
  }

  return message;
}

