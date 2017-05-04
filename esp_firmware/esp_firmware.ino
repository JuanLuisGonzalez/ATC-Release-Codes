/* ATC_Expert_EXP8266
 REMEMBER TO REMOVE ESP8266 WHEN UPLOADING SKETCH
 Functions to control and display information into the app. This code works with any arduino board with ESP8266 connected.

 * ESP8266 Module attached to Serial port

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
 GPIO0 and GPIO2 not connected
 CH_PD goes to Vcc
 RST   goes to Vcc

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

 Use <Abar tags to modify the current analog bar values inapp: <AbarXX:YYY\n, where XX is a number from 0 to 11, the bar number,
 and YYY is a 3 digit integer from 000 to 255, the bar value

 Use <Logr tags to store string data in the app temporary log: "<Logr00:" + "any string" + "\n"

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
String sPort = "80";

// Special commands
#define CMD_SPECIAL '<'
#define CMD_ALIVE   '['

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

  // initialize WiFI module;
  myESP_Init();

  // Greets on top of the app
  myESP_Println("ATC Expert ESP8266", 0);

  // Make the app talk in english (lang number 00, use 01 to talk in your default language)
  myESP_Println("<TtoS00: welcome to ATC expert", 0);
}

void loop() {
  int appData = 0;
  String appMessage = "";
  delay(1);

  // ===========================================================
  // This is the point were you get data from the App
  if (Serial.available() > 10) {
    appMessage = myESP_Read(0);    // Read channel 0
    appData = appMessage.charAt(0);

    switch (appData) {
      case CMD_SPECIAL:
        // Special command received
        DecodeSpecialCommand(appMessage.substring(1));
        break;

      case CMD_ALIVE:
        // Character '[' is received every 2.5s, use
        myESP_Println("ATC Expert ESP8266", 0);
        break;
    }
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
 Set up esp 8266 wifi module on Serial
 AT+CIPMUX=1
 AT+CIPSERVER=1,80
 */
void myESP_Init() {
  // Initialize serial port with pull up in rx
  Serial.begin(BAUD_RATE);
  pinMode(RX_PIN, INPUT_PULLUP);

  // Reset module
  Serial.println("AT+RST"); // Reset the board
  delay(5000);
  // Uncomment lines below to set up network for the first time
  //Serial.println("AT+CWMODE=3"); // both wifi client and access point
  //delay(100);
  //Serial.println("AT+CWJAP=\"linksys\",\"\""); // Join access point linksys, no password
  //delay(10000);
  Serial.println("AT+CIPMUX=1"); // Multichannel connection
  delay(500);
  Serial.println("AT+CIPSERVER=1," + sPort); // Open server socket at sPort
  delay(500);
}

// Send string data to app
// To send data AT+CIPSEND=0,X (0: channel, X data size)
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
// To send data AT+CIPSEND=0,X (0: channel, X data size)
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

// This read function is faster
// call this function when you have minimum 9 bytes available
// When data received: +IPD,0,2:[ (for this exaple 0 is the channel, and 2 is the data lenght)
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

// Rare error down, blue module uses 1 as channel and black one uses 0
