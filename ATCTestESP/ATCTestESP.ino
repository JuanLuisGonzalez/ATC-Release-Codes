/*Arduino Total Control ESP Firmware
 REMEMBER TO DISCONNECT BLUETOOT TX PIN WHEN UPLOADING SKETCH
 Basic functions to control and display information into the app.This code works for every arduino board.

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
 GPIO2 goes to Vcc
 CH_PD goes to Vcc
 RST   goes to Vcc

 ESP test on arduino pins serial 1 18 and 19
 works well with 3.6V on vcc med overheat

 Network Set up:
 Open serial terminal (set it to your module Baud rate, and select both NL & CR
 When sending AT, module must answer "OK", dont proceed until you have the module's ok

 Main commands
 Use AT+CWLAP to list all available networks

 Use the following sequences to connect to your network
 AT+RST
 AT+GMR
 AT+CWMODE=3
 AT+CWJAP="INFINITUMtcyv","f598702e0c"

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

Now you are connected, the following section is for normal communication with app

 To send data to app use tags:
 For buttons: (<ButnXX:Y\n), XX 0 to 19 is the button number, Y 0 or 1 is the state
 Example: Serial.println("<Butn05:1"); will turn the app button 5 on

 For texts: <TextXX:YYYY\n, XX 0 to 19 is the text number, YYYY... is the string to be displayed
 Example: Serial.println("<Text01:A1: 253"); will display the text "A1: 253" at text 1

 For images: <ImgsXX:Y\n, XX 0 to 19 is the image number, Y is the image state(0, 1 or 2)to be displayed
 Example: Serial.println("<Imgs02:1"); will change image 2 to the pressed state

 For sound alarms: <Alrm00 will make the app beep.

 Change the seek bar values in app: <SkbX:YYY\n, where X is the seek bar number from 0 to 7, and YYY is the seek bar value

 Make the app talk: Text to Speech tag <TtoS0X:YYYY\n, X is 0 for english and 1 for your default language, YYYY... is any string
 Example: Serial.println("<TtoS00:Hello world");

 If a  no tag new line ending string is sent, it will be displayed at the top of the app
 Example: Serial.println("Hello Word"); "Hello Word" will be displayed at the top of the app

 Special information can be received from app, for example sensor data and seek bar info:
 * "<PadXxx:YYY\n, xx 00 to 24 is the touch pad number, YYY is the touch pad X axis data 0 to 255
 * "<PadYxx:YYY\n, xx 00 to 24 is the touch pad number, YYY is the touch pad Y axis data 0 to 255
 * "<SkbX:SYYYYY\n", X 0 to 7 is the seek bar number, YYYYY is the seek bar value from 0 to 255
 * "<AccX:SYYYYY\n", X can be "X,Y or Z" is the accelerometer axis, YYYYY is the accelerometer value
    in m/s^2 multiplied by 100, example: 981 == 9.81m/s^2
 * "S" is the value sign (+ or -)

 TIP: To select another Serial port use ctr+f, find: Serial change to SerialX

 Author: Juan Luis Gonzalez Bello
 Date: November 2015
 Get the app: https://play.google.com/store/apps/details?id=com.apps.emim.btrelaycontrol
 ** After copy-paste of this code, use Tools -> Atomatic Format
 */

#include <EEPROM.h>

// Baud rate for ESP module
#define BAUD_RATE 115200

String sPort = "80";

// Special commands
#define CMD_SPECIAL '<'
#define CMD_ALIVE   '['
#define RX_PIN       0 // Check this out! this is to enable arduino pull up on rx pin, critical!

// Data and variables received from especial command
int Accel[3] = {0, 0, 0};
int SeekBarValue[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int TouchPadData[24][2]; // 24 max touch pad objects, each one has 2 axis (x and Y)

#define BID_IO_NO       6
#define BID_IO_SET_ADD  30
#define RELAY_EE_ADD    20
#define RELAY_OUT_NO    6
#define ADC_IN_NO       6

//List your bidirectional input/output pins
int BidIOPins[BID_IO_NO] = {2, 3, 4, 5, 6, 7};
String BidIOPinsGraphics[BID_IO_NO] = {"12", "13", "14", "15", "16", "17"};
String ToggleModeGraphics[BID_IO_NO] = {"18", "19", "20", "21", "22", "23"};
// When selectedas outputs, state what command will activate/deactivate them
char BidIOPinCommandOn[BID_IO_NO]  = {'G', 'H', 'I', 'J', 'K', 'L'};
char BidIOPinCommandOff[BID_IO_NO] = {'g', 'h', 'i', 'j', 'k', 'l'};
char ToggleModeCommand[BID_IO_NO]  = {'M', 'N', 'O', 'P', 'Q', 'R'};
// Bidirectional input/output mode, 1 is input, 0 is output
int BidIOMode = B00111111;

// List your analog inputs here
int AnalogInputs[ADC_IN_NO] = {A0, A1, A2, A3, A4, A5};
String AnalogInputsGraphics[ADC_IN_NO] = {"06", "07", "08", "09", "10", "11"};
// Boundaries are used to interact with graphics (from 0 to 1024)
int AnalogBoudaries[ADC_IN_NO][2] = {{300, 600}, {300, 600}, {300, 600}, {300, 600}, {300, 600}, {300, 600}};

// List your relay outputs here
int RelayOutputs[RELAY_OUT_NO] = {8, 9, 10, 11, 12, 13};
String RelayOutputsGraphics[RELAY_OUT_NO] = {"00", "01", "02", "03", "04", "05"};
char RelayOutputsCommandOn[RELAY_OUT_NO]  = {'A', 'B', 'C', 'D', 'E', 'F'};
char RelayOutputsCommandOff[RELAY_OUT_NO] = {'a', 'b', 'c', 'd', 'e', 'f'};
int RelayStatus = 0;

void setup() {
  Serial.begin(BAUD_RATE);
  myESP_Init();

  // Initialize touch pad data,
  // this is to avoid having random numbers in them
  for (int i = 0; i < 24; i++) {
    TouchPadData[i][0] = 0; //X
    TouchPadData[i][1] = 0; //Y
  }

  // Retrieve I/O mode from eeprom
  BidIOMode = EEPROM.read(BID_IO_SET_ADD);

  // Set I/O according to BidIOMode
  for (int i = 0; i < BID_IO_NO; i++) {
    if (BidIOMode & (0x01 << i)) { // check bit by bit
      // if 1
      pinMode(BidIOPins[i], INPUT_PULLUP);
      myESP_Println("<Imgs" + ToggleModeGraphics[i] + ":1", 0);
    }
    else {
      // if 0
      pinMode(BidIOPins[i], OUTPUT);
      myESP_Println("<Imgs" + ToggleModeGraphics[i] + ":0", 0);
    }
  }

  // Restore relays previous state
  RelayStatus = EEPROM.read(RELAY_EE_ADD);

  // Set Relays as outputs
  for (int i = 0; i < RELAY_OUT_NO; i++) {
    pinMode(RelayOutputs[i], OUTPUT);
    if (RelayStatus & (0x01 << i)) { // check bit by bit
      // if 1
      digitalWrite(RelayOutputs[i], HIGH);
    }
    else {
      // if 0
      digitalWrite(RelayOutputs[i], LOW);
    }
  }

  // Set Analogs as inputs
  for (int i = 0; i < ADC_IN_NO; i++) {
    pinMode(AnalogInputs[i], INPUT);
  }
}

void loop() {
  int appData = -1;
  String appMessage;
  static int counter = 0;
  static int counter1 = 0;
  String firmStatus = "";

  delay(1);

  // ==========================================================
  // This is the point where you read all your inputs
  /*  We dont want to overload the ATC app, thus, let this statement
   *  to be true each 100 * 1ms = 100ms
  */
  if (counter++ > 100) {
    counter = 0;
    String bidIOStatus = "";
    
    // Bidirectional input/output
    for (int i = 0; i < BID_IO_NO; i++) {
      // only read if set as input
      if (BidIOMode & (0x01 << i)) {
        if (digitalRead(BidIOPins[i])) {
          // Change grahic in the app
          bidIOStatus = bidIOStatus + "<Imgs" + BidIOPinsGraphics[i] + ":1\n";
          // Pin is high, do something 
        }
        else {
          // Change graphic in the app
          bidIOStatus = bidIOStatus + "<Imgs" + BidIOPinsGraphics[i] + ":0\n";
          // Pin is low, do something
        }
      }
    }
    // concentrate all messages in a single string and send
    myESP_Print(bidIOStatus, 0);
  }

  /*  We dont want to overload the ATC app, thus, let this statement
   *  to be true each 1000 * 1ms = 1000ms = 1 second
  */
  if (counter1++ > 1000) {
    counter1 = 0;
    String analogValues = "";
    
    // Analog input
    for (int i = 0; i < ADC_IN_NO; i++) {
      int sample = analogRead(AnalogInputs[i]);
      analogValues = analogValues + "<Text" + AnalogInputsGraphics[i] + ":V: " + String(sample) + "\n";
      // Change grahic in the app
      if (sample < AnalogBoudaries[i][0]) {
        // Change graphic in the app
        analogValues = analogValues + "<Imgs" + AnalogInputsGraphics[i] + ":0\n";
      }
      else if (sample < AnalogBoudaries[i][1]) {
        // Change graphic in the app
        analogValues = analogValues + "<Imgs" + AnalogInputsGraphics[i] + ":1\n";
      }
      else {
        // Change graphic in the app
        analogValues = analogValues + "<Imgs" + AnalogInputsGraphics[i] + ":2\n";
      }
    }
    // Concentrate all commands in a single ESP_Pritnln
    myESP_Print(analogValues, 0);
  }

  // ===========================================================
  // This is the point were you get data from the App
  if (Serial.available() > 10) {
    appMessage = myESP_Read(0);    // Read channel 0
    appData = appMessage.charAt(0);
    switch (appData) {
      case CMD_SPECIAL: /// not used, but do not delete!
        // Special command received, seekbar value and accel value updates
        DecodeSpecialCommand(appMessage.substring(1));// we already have the data :)
        break;

      case CMD_ALIVE:
        // Character '[' is received every 2.5s
        // use this heartbeat to check all the output (only) states and report to arduino
        myESP_Println("PARSIC Vxx", 0);

        firmStatus = "";

        // Bidirectional input/output
        for (int i = 0; i < BID_IO_NO; i++) {
          // only check if is set as output
          if (!(BidIOMode & (0x01 << i))) {
            if (digitalRead(BidIOPins[i])) {
              // Change graphic in the app
              firmStatus = firmStatus + "<Imgs" + BidIOPinsGraphics[i] + ":1\n";
            }
            else {
              // Change graphic in the app
              firmStatus = firmStatus + "<Imgs" + BidIOPinsGraphics[i] + ":0\n";
            }
          }
        }
        // Concentrate in a single print
        myESP_Println(firmStatus, 0);
        firmStatus = "";

        // Check Relay output
        for (int i = 0; i < RELAY_OUT_NO; i++) {
          // Turn on or off depending on command
          if (digitalRead(RelayOutputs[i])) {
            // Change graphic in the app
            firmStatus = firmStatus + "<Imgs" + RelayOutputsGraphics[i] + ":1\n";
          }
          else {
            // Change graphic in the app
            firmStatus = firmStatus + "<Imgs" + RelayOutputsGraphics[i] + ":0\n";
          }
        }
        // Concentrate in a single print
        myESP_Println(firmStatus, 0);
        firmStatus = "";
        
        // Set I/O according to BidIOMode
        for (int i = 0; i < BID_IO_NO; i++) {
          if (BidIOMode & (0x01 << i)) { // check bit by bit
            // if 1
            firmStatus =  firmStatus + "<Imgs" + ToggleModeGraphics[i] + ":1\n";
          }
          else {
            // if 0
            firmStatus =  firmStatus + "<Imgs" + ToggleModeGraphics[i] + ":0\n";
          }
        }
        // Concentrate in a single print
        myESP_Println(firmStatus, 0);
        break;

      default:
        //if not an special command, then look for output command
        // Bidirectional input/output
        for (int i = 0; i < BID_IO_NO; i++) {
          // only turn on if is set as output
          if (!(BidIOMode & (0x01 << i))) {
            // Turn on or off depending on command
            if (appData == BidIOPinCommandOn[i]) {
              digitalWrite(BidIOPins[i], HIGH);
              // Change graphic in the app
              myESP_Println("<Imgs" + BidIOPinsGraphics[i] + ":1", 0);
            }
            else if (appData == BidIOPinCommandOff[i]) {
              digitalWrite(BidIOPins[i], LOW);
              // Change graphic in the app
              myESP_Println("<Imgs" + BidIOPinsGraphics[i] + ":0", 0);
            }
          }
        }

        // Toggle Bidirectional mode
        for (int i = 0; i < BID_IO_NO; i++) {
          if (appData == ToggleModeCommand[i]) {
            // Make output if input
            if (BidIOMode & (0x01 << i)) {
              pinMode(BidIOPins[i], OUTPUT);
              BidIOMode &= ~(0x01 << i); // Clear bit
              myESP_Println("<Imgs" + ToggleModeGraphics[i] + ":0", 0);
            }
            //Make input if output
            else {
              pinMode(BidIOPins[i], INPUT_PULLUP);
              BidIOMode |= (0x01 << i); // Set bit
              // Change grahic in the app
              myESP_Println("<Imgs" + ToggleModeGraphics[i] + ":1", 0);
            }
            // Save in eeprom
            EEPROM.write(BID_IO_SET_ADD, BidIOMode);
          }
        }

        // Relay output
        for (int i = 0; i < RELAY_OUT_NO; i++) {
          // Turn on or off depending on command
          if (appData == RelayOutputsCommandOn[i]) {
            digitalWrite(RelayOutputs[i], HIGH);
            // update relay status and save it in eeprom
            RelayStatus |= (0x01 << i);// set bit
            EEPROM.write(RELAY_EE_ADD, RelayStatus);
            // Change graphic in the app
            myESP_Println("<Imgs" + RelayOutputsGraphics[i] + ":1", 0);
          }
          else if (appData == RelayOutputsCommandOff[i]) {
            digitalWrite(RelayOutputs[i], LOW);
            // update relay status and save it in eeprom
            RelayStatus &= ~(0x01 << i);// clear bit
            EEPROM.write(RELAY_EE_ADD, RelayStatus);
            // Change graphic in the app
            myESP_Println("<Imgs" + RelayOutputsGraphics[i] + ":0", 0);
          }
        }
    }
  }
  // ==========================================================
}

String SBtoString(int value) {
  String sValue = "";
  if (value < 10)
    sValue = "00" + String(value);
  else if (value < 100)
    sValue = "0" + String(value);
  else
    sValue = String(value);
  return sValue;
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
  delay(5000);
  // Uncomment lines below to set up network for the first time
  //Serial.println("AT+CWMODE=3");
  //delay(100);
  //Serial.println("AT+CWJAP=\"linksys\",\"\"");
  //delay(10000);
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
  Serial.println("AT+CIPSEND=" + String(channel) + "," + String(dataSize));
  Serial.flush(); // wait transmision complete
  delay(3);       // wait esp module process

  // Print actual data
  Serial.println(data);
  Serial.flush(); // wait transmision complete
  delay(6);       // wait esp module process
}

// Send string data to app
void myESP_Print(String data, int channel) {
  int dataSize = data.length();

  // submit cipsend command for channel
  Serial.println("AT+CIPSEND=" + String(channel) + "," + String(dataSize));
  Serial.flush(); // wait transmision complete
  delay(3);       // wait esp module process

  // Print actual data
  Serial.print(data);
  Serial.flush(); // wait transmision complete
  delay(8);       // wait esp module process, normally used for big data string, wait more
}

// Receive string data from app (a char can be tough as a single character string)
// remember: +IPD,0,2:[ (for this exaple 0 is the channel, and 2 is the data lenght)
// Channel function is disabled, it wont care about it
/*
String myESP_Read(int channel) {
  String dataFromClient = "";
  String message = "";

  // only if character '+' is received
  if (Serial.read() == '+') {
    dataFromClient = Readln();                                   // read the whole line
    myESP_Println(dataFromClient, 0);
    String sCommand = dataFromClient.substring(0, 3);
    if (sCommand.equals("IPD")) {                                // check this is actually the command we expect
      int clientChannel = dataFromClient.charAt(4) & ~0x30;      // convert to int
      //if(clientChannel == channel){                              // only return data if it is from the intended channel
      int twoPointsIndex = dataFromClient.indexOf(':');       // find the ':' on the command
      message = dataFromClient.substring(twoPointsIndex + 1); // set the message
      //}
    }
  }
  return message;
}*/

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
    // Extract message
    int twoPointsIndex = dataFromClient.indexOf(':');       // find the ':' on the command
    message = dataFromClient.substring(twoPointsIndex + 1); // set the message
  }

  return message;
}


