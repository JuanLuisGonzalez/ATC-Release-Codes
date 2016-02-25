/*Arduino Total Control for Advanced programers
 REMEMBER TO DISCONNECT BLUETOOT TX PIN WHEN UPLOADING SKETCH
 Basic program to control LED intensity / Motor / Fan speed using seekbars, touchpad and accelerometer

 * Bluetooth Module attached to Serial port
 * Connect a transistor or amplifier on the selected outputs (Make sure these have PWM function!!!) 
   to control whatever you want.
 * Enable touch pad 0 in app
 * Enable accelerometer on X in app
 * Enable Seekbar 5 in app

 Author: Juan Luis Gonzalez Bello
 Date: Feb 2016
 Get the app: https://play.google.com/store/apps/details?id=com.apps.emim.btrelaycontrol
 ** After copy-paste of this code, use Tools -> Atomatic Format
 */

#include <EEPROM.h>
#include <Servo.h>

// Baud rate for bluetooth module
// (Default 9600 for most modules)
#define BAUD_RATE 115200 // TIP> Set your bluetooth baud rate to 115200 for better performance

// Special commands
#define CMD_SPECIAL '<'
#define CMD_ALIVE   '['

// Data and variables received from especial command
int Accel[3] = {0, 0, 0};
int SeekBarValue[8] = {0,0,0,0,0,0,0,0};
int TouchPadData[24][2]; // 24 max touch pad objects, each one has 2 axis (x and Y)

void setup() {
  // Initialize touch pad data, 
  // this is to avoid having random numbers in them
  for(int i = 0; i < 24; i++){
    TouchPadData[i][0] = 0; //X
    TouchPadData[i][1] = 0; //Y
  }

  // initialize BT Serial port
  Serial.begin(BAUD_RATE);

  // Initialize Output PORTS Make sure these have PWM function!!!
  pinMode(13, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(10, OUTPUT);

  // Greet arduino total control on top of the app
  Serial.println("Thanks for your support!");

  // Make the app talk in english (lang number 00, use 01 to talk in your default language)
  Serial.println("<TtoS00: welcome to arduino total control fan, motor, and whatever control");
}

void loop() {
  int appData;
  delay(1);

  // ===========================================================
  // This is the point were you get data from the App
  appData = Serial.read();   // Get a byte from app, if available
  switch(appData){
  case CMD_SPECIAL:
    // Special command received
    DecodeSpecialCommand();
    
    // Example of how to use seek bar data, Seekbar No. 5
    analogWrite(13, SeekBarValue[5]);
    
    // Example of how to use touchpad dat, touchpad 0,  X axis
    analogWrite(12, TouchPadData[0][0]);

    // Example of how to use touchpad data, touchpad 0, Y axis
    analogWrite(11, TouchPadData[0][1]);    

    // Example of how to use accelerometer data, Acceleration on X axis
    // simplified convertion -981 -> 0 and 981 -> 255 approx
    // Round 981 to 1000, and 255 to 250, then apply the line equiation...
    analogWrite(10, (Accel[0] + 1000) * 25 / 200);
    break;

  case CMD_ALIVE:
    // Character '[' is received every 2.5s, use
    // this event to tell the android we are still connected
    Serial.println("We are connected :)");
    break;
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

    if (Serial.available() > 0)
      inByte = Serial.read();

    if(inByte != -1)
      message.concat(String(inByte));
  }

  return message;
}
