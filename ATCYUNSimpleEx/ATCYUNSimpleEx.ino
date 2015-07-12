/*Arduino Total Control Firmware for Jun
 Basic functions to control and display information into the app.
 This code works for arduino YUN or YUN shield only.
 Commnunication is made as the following:

 Arduino -> Bridge -> WiFi module on yun -> Your wireless router (WLAN) -> ATC App
 Port: 5555

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
 
 If a  no tag new line ending string is sent, it will be displayed at the top of the app
 Example: Serial.println("Hello Word"); "Hello Word" will be displayed at the top of the app
 
 Special information can be received from app, for example sensor data and seek bar info:
 * "<SkbX:SYYYYY\n", X 0 to 7 is the seek bar number, YYYYY is the seek bar value from 0 to 255
 * "<AccX:SYYYYY\n", X can be "X,Y or Z" is the accelerometer axis, YYYYY is the accelerometer value 
    in m/s^2 multiplied by 100, example: 981 == 9.81m/s^2
 * "S" is the value sign (+ or -)
 
 TIP: To select another Serial port use ctr+f, find: Serial change to SerialX
 
 Author: Juan Luis Gonzalez Bello 
 Date: June 2015
 Get the app: https://play.google.com/store/apps/details?id=com.apps.emim.btrelaycontrol
 ** After copy-paste of this code, use Tools -> Atomatic Format 
 */

#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>

// Special commands
#define CMD_SPECIAL '<'
#define CMD_ALIVE   '['

// Listen on default port 5555, the webserver on the Yun
// ip addres is asigned by your router
YunServer server;
YunClient client;

// Data and variables received from especial command
int Accel[3] = {0, 0, 0}; 
int SeekBarValue[8] = {0,0,0,0,0,0,0,0};

void setup() {
  // Bridge startup, set up comm between arduino an wifi module
  Bridge.begin();

  // Listen for incoming connections
  // Arduino acts as a TCP/IP server
  server.begin();

  pinMode(11, OUTPUT);
  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
}

void loop() {
  int appData = -1; 
  
  // =========================================================== 
  // This is the point were you get data from the App 
  // Get clients coming from server
  if(client) appData = client.read();   // Get a byte from app, if available 
  else client = server.accept();
 
  switch(appData){ 
  case CMD_SPECIAL: 
    // Special command received, seekbar value and accel value updates 
    DecodeSpecialCommand();
    analogWrite(11, SeekBarValue[5]);
    analogWrite(A0, abs((Accel[0] / 95) * 25));
    analogWrite(A1, abs((Accel[1] / 95) * 25));
    break; 

  case CMD_ALIVE: 
    // Character '[' is received every 2.5s
    server.println("ATC ready");
    break;

  case 'S':
  digitalWrite(11, HIGH);
  break;

  case 'x':
  digitalWrite(11, LOW);
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

    if(client) inByte = client.read();
    else client = server.accept();

    if(inByte != -1)
      message.concat(String(inByte)); 
  }

  return message; 
}
