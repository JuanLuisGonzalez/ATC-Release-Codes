/*Arduino Total Control Firmware
 REMEMBER TO DISCONNECT BLUETOOT TX PIN WHEN UPLOADING SKETCH
 Basic functions to control and display information into the app.This code works for every arduino board.
 
 * Bluetooth Module attached to Serial port 
 
 To send data to app use tags:
 For buttons: (<ButnXX:Y\n), XX 0 to 19 is the button number, Y 0 or 1 is the state
 Example: Serial.println("<Butn05:1"); will turn the app button 5 on
 
 For texts: <TextXX:YYYY\n, XX 0 to 19 is the text number, YYYY... is the string to be displayed
 Example: Serial.println("<Text01:A1: 253"); will display the text "A1: 253" at text 1
 
 For images: <ImgsXX:Y\n, XX 0 to 19 is the image number, Y is the image state(0, 1 or 2)to be displayed
 Example: Serial.println("<Imgs02:1"); will change image 2 to the pressed state
 
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
 
 TIP: To select another Serial port use ctr+f, find: Serial change to SerialX
 /*
 Author: Juan Luis Gonzalez Bello 
 Date: June 2017
 Get the app: https://play.google.com/store/apps/details?id=com.apps.emim.btrelaycontrol
 ** After copy-paste of this code, use Tools -> Atomatic Format 
 */

// Baud rate for bluetooth module
// (Default 9600 for most modules)
#define BAUD_RATE 9600 // Tip: Configure your bluetooth device for 115200 for better performance  

// Special commands
#define CMD_SPECIAL '<'
#define CMD_ALIVE   '['

// Return values for special command
#define UPDATE_FAIL  0
#define UPDATE_ACCEL 1
#define UPDATE_SKBAR 2
#define UPDATE_TPAD  3
#define UPDATE_SPCH  4

// Data and variables received from especial command
int Accel[3] = {0, 0, 0}; 
int SeekBarValue[8] = {0,0,0,0,0,0,0,0};
int TouchPadData[24][2]; // 24 max touch pad objects, each one has 2 axis (x and Y)
String SpeechRecorder = "";

void setup() {   
  // initialize BT Serial port
  Serial.begin(BAUD_RATE); 
} 

void loop() { 
  int appData; 
  
  // ================================================================= 
  // This is the point were you get data from the App 
  appData = Serial.read();   // Get a byte from app, if available 
  switch(appData){ 
  case CMD_SPECIAL: 
    // Special command received, seekbar value and accel value updates 
    DecodeSpecialCommand();
    break; 

  case CMD_ALIVE: 
    // Character '[' is received every 2.5s
    Serial.println("ATC ready");
    break;
  } 
  // ================================================================= 
} 

/**
 * DecodeSpecialCommand
 * A '<' flags a special command comming from App. Use this function 
 * to get Accelerometer data (and other sensors in the future)
 * Input:    None 
 * Output:   None
 */
int DecodeSpecialCommand() {
  int tagType = UPDATE_FAIL;
  int isAccelData = -1;
  int isPadData = -1;
   
  // Read the whole command
  String thisCommand = Readln();

  // First 5 characters will tell us the command type
  String commandType = thisCommand.substring(0, 5);

  if (commandType.equals("AccX:"))
    isAccelData = 0;
  if (commandType.equals("AccY:"))
    isAccelData = 1;
  if (commandType.equals("AccZ:"))
    isAccelData = 2;
 
  if(isAccelData!= -1){
    // Next 6 characters will tell us the command data
    String commandData = thisCommand.substring(5, 11);
    if (commandData.charAt(0) == '-') // Negative acceleration
      Accel[isAccelData] = -commandData.substring(1, 6).toInt();
    else
      Accel[isAccelData] = commandData.substring(1, 6).toInt();
    tagType = UPDATE_ACCEL;
  }

  if (commandType.substring(0, 4).equals("PadX"))
    isPadData = 0;
  if (commandType.substring(0, 4).equals("PadY"))
    isPadData = 1;
      
  if(isPadData != -1){
    // Next 2 characters will tell us the touch pad number
    int padNumber = thisCommand.substring(4, 6).toInt();
    // Next 3 characters are the X axis data in the message
    String commandData = thisCommand.substring(8, 13);
    TouchPadData[padNumber][isPadData] = commandData.toInt();
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

/** 
 * Convert integer integer into 3 digit string
 */
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

/** 
 * Readln
 * Use this function to read a String line from Bluetooth
 * returns: String message, note that this function will pause the program
 *          until a hole line has been read.
 */ 
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
