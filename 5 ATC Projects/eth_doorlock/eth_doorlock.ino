/*Arduino Total Control Firmware for Ethernet
 Basic functions to control and display information into the app.
 This code works for every arduino board.

 * Ethernet shield Module attached
 - For UNO pins 10, 11, 12 and 13 are dedicated
 - For MEGA pins 10, 50, 51 and 52 are dedicated

 To send data to app use tags:
 For buttons: (<ButnXX:Y\n), XX 0 to 19 is the button number, Y 0 or 1 is the state
 Example: server.println("<Butn05:1"); will turn the app button 5 on

 For texts: <TextXX:YYYY\n, XX 0 to 19 is the text number, YYYY... is the string to be displayed
 Example: server.println("<Text01:A1: 253"); will display the text "A1: 253" at text 1

 For images: <ImgsXX:Y\n, XX 0 to 19 is the image number, Y is the image state(0, 1 or 2)to be displayed
 Example: server.println("<Imgs02:1"); will change image 2 to the pressed state

 Make the app vibrate: <Vibr00:YYY\n, YYY is a number from 000 to 999, and represents the vibration time in ms
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
 Date: May 2017
 Get the app: https://play.google.com/store/apps/details?id=com.apps.emim.btrelaycontrol
 ** After copy-paste of this code, use Tools -> Atomatic Format
 */

#include <SPI.h>
#include <Ethernet.h>

// Special commands
#define CMD_ALIVE   '['
#define CMD_SPECIAL '<'

// Signal to update special commands
#define UPDATE_FAIL  0
#define UPDATE_ACCEL 1
#define UPDATE_SKBAR 2
#define UPDATE_TPAD  3
#define UPDATE_SPCH  4

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(172, 21, 117, 60);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(8080);
EthernetClient client;

// Data and variables received from especial command
int Accel[3] = {
  0, 0, 0
};
int SeekBarValue[8] = {
  0, 0, 0, 0, 0, 0, 0, 0
};
int TouchPadData[24][2]; // 24 max touch pad objects, each one has 2 axis (x and Y)
String SpeechRecorder = "";

// Sistem specific ports and variables
#define TIME_OUT 20000 // 20 seconds
#define OUT_NO       6
#define ST_CLOSED    0
#define ST_OPEN      1
#define PASS_NO     10
#define PASS_SIZE    6
#define CMD_ARM      'A'

int outs[OUT_NO] = {1, 2, 3, 4, 5, 6};
boolean FirstTimeConnection = false;
int TimeOutCounter = 0;
String Password = "1 3 2 5 4 6";
String PasswordTest = "";
int LockStatus = ST_CLOSED;
int LockSequence = 0;
int PassNumbers[PASS_NO] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

void setup() {
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();

  // Used to ground output leds
  pinMode(7, OUTPUT);
  digitalWrite(7, LOW);

  // Initialize outputs
  for (int i = 0; i < OUT_NO; i++) {
    pinMode(outs[i], OUTPUT);
    digitalWrite(outs[i], LOW);
  }
}

void loop() {
  int appData;
  int commandType;
  delay(1);

  if (FirstTimeConnection) {
    digitalWrite(outs[0], HIGH);
    if (TimeOutCounter++ > TIME_OUT) {
      FirstTimeConnection = false;
      TimeOutCounter = 0;
    }
  }
  else {
    digitalWrite(outs[0], LOW);
    LockStatus = ST_CLOSED;
  }

  // lock or release according to the status
  if (LockStatus == ST_OPEN) {
    for (int i = 1; i < OUT_NO; i++) {
      digitalWrite(outs[i], HIGH);
    }
  }
  else {
    for (int i = 1; i < OUT_NO; i++) {
      digitalWrite(outs[i], LOW);
    }
  }

  // Check password
  if (LockSequence == PASS_SIZE) {
    server.println("Recivido:" + PasswordTest);
    if (PasswordTest.equals(Password)) {
      server.println("<Vibr00:050");
      server.println("<TtoS01: contraseña correcta, adelante");
      LockStatus = ST_OPEN;
    }
    else {
      server.println("<TtoS01: contraseña incorrecta");
      server.println("<Alrm00");
      server.println("<Vibr00:999");
      LockStatus = ST_CLOSED;
    }
    LockSequence = 0;
    PasswordTest = "";
  }
  // Should not happen
  if (LockSequence > PASS_SIZE) {
    LockStatus = ST_CLOSED;
    LockSequence = 0;
    PasswordTest = "";
  }

  // ===========================================================
  // This is the point were you get data from the App
  client = server.available();
  if (client) {
    appData = client.read();
  }

  switch (appData) {
    case CMD_SPECIAL:
      // Special command received
      // After this function accel and seek bar values are updated
      commandType = DecodeSpecialCommand();
      if (commandType == UPDATE_SPCH) {
        server.println("Recivido:" + SpeechRecorder);
        if (SpeechRecorder.equals(Password)) {
          server.println("<Vibr00:050");
          server.println("<TtoS01: contraseña correcta, adelante");
          LockStatus = ST_OPEN;
        }
        else {
          server.println("<TtoS01: contraseña incorrecta");
          server.println("<Alrm00");
          server.println("<Vibr00:999");
          LockStatus = ST_CLOSED;
        }
      }
      break;

    case CMD_ALIVE:
      // Character '[' is received every 2.5s
      server.println("ATC ready");
      // Reset timeout
      TimeOutCounter = 0;
      // Greet first connection
      if (!FirstTimeConnection) {
        FirstTimeConnection = true;
        server.println("<TtoS01: Bienvenido a casa, juan luis");
      }
      break;

    case CMD_ARM:
      server.println("<TtoS01: Bloqueado");
      LockStatus = ST_CLOSED;
    break;
    
    default:
      for (int i = 0; i < PASS_NO; i++) {
        if (appData == PassNumbers[i]) {
          if(LockSequence == 0){
            PasswordTest = String(char(PassNumbers[i]));
          }
          else{
            PasswordTest = PasswordTest + " " + char(PassNumbers[i]);
          }
          LockSequence++;
        }
      }
  }
  // ==========================================================
}

// DecodeSpecialCommand
//
// A '<' flags a special command comming from App. Use this function
// to get Accelerometer data (and other sensors in the future)
// Input:
//   None
// Output:
//   None
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

    client = server.available();
    if (client)
      inByte = client.read();

    if (inByte != -1)
      message.concat(String(inByte));
  }

  return message;
}


