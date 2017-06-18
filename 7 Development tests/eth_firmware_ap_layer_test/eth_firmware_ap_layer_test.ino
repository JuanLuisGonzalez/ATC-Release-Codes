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
 
 For sound alarms: <Alrm00 will make the app beep.
 
 Make the app talk: Text to Speech tag <TtoS0X:YYYY\n, X is 0 for english and 1 for your default language, YYYY... is any string
 Example: Serial.println("<TtoS00:Hello world");

 Change the seek bar values in app: <SkbX:YYY\n, where X is the seek bar number from 0 to 7, and YYY is the seek bar value

 Use <Abar tags to modify the current analog bar values inapp: <AbarXX:YYY\n, where XX is a number from 0 to 11, the bar number,
 and YYY is a 3 digit integer from 000 to 255, the bar value

 Use <Logr tags to store string data in the app temporary log: "<Logr00:" + "any string" + "\n"
 
 If a  no tag new line ending string is sent, it will be displayed at the top of the app
 Example: server.println("Hello Word"); "Hello Word" will be displayed at the top of the app
 
  Special information can be received from app, for example sensor data and seek bar info:
 * "<PadXxx:YYY\n, xx 00 to 24 is the touch pad number, YYY is the touch pad X axis data 0 to 255
 * "<PadYxx:YYY\n, xx 00 to 24 is the touch pad number, YYY is the touch pad Y axis data 0 to 255
 * "<SkbX:SYYYYY\n", X 0 to 7 is the seek bar number, YYYYY is the seek bar value from 0 to 255
 * "<AccX:SYYYYY\n", X can be "X,Y or Z" is the accelerometer axis, YYYYY is the accelerometer value 
    in m/s^2 multiplied by 100, example: 981 == 9.81m/s^2
 * "S" is the value sign (+ or -)

 /*
 Author: Juan Luis Gonzalez Bello 
 Date: June 2017   
 Get the app: https://play.google.com/store/apps/details?id=com.apps.emim.btrelaycontrol
 ** After copy-paste of this code, use Tools -> Atomatic Format 
 */

#include <SPI.h> 
#include <Ethernet.h> 

// Special commands
#define CMD_SPECIAL '<'
#define CMD_ALIVE   '['

// Enter a MAC address and IP address for your controller below. 
// The IP address will be dependent on your local network: 
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; 
IPAddress ip(172,21,117,60); 

// Initialize the Ethernet server library 
// with the IP address and port you want to use 
// (port 80 is default for HTTP): 
EthernetServer server(80); 
EthernetClient client; 

// Data and variables received from especial command
#define ACCEL_AXIS 3
#define SKBAR_NO   8
#define TPAD_NO    24
#define TPAD_AXIS  2

#define UPDATE_FAIL  0
#define UPDATE_ACCEL 1
#define UPDATE_SKBAR 2
#define UPDATE_TPAD  3
#define UPDATE_SPCH  4

int Accel[ACCEL_AXIS] = {
  0, 0, 0}; 
int SeekBarValue[SKBAR_NO] = {
  0,0,0,0,0,0,0,0};
int TouchPadData[TPAD_NO][TPAD_AXIS]; // 24 max touch pad objects, each one has 2 axis (x and Y)
String SpeechRecorder = "";

void setup() {  
  // start the Ethernet connection and the server: 
  Ethernet.begin(mac, ip); 
  server.begin();
  Serial.begin(115200);

  // Initiate touch pad array
  for(int i = 0; i < TPAD_NO; i++){
    for(int j = 0; j < TPAD_AXIS; j++){
      TouchPadData[i][j] = 0;
    }
  }

  // Set up pins for LED
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  digitalWrite(5, LOW);
  digitalWrite(6, LOW);
  digitalWrite(7, LOW);
} 

void loop() { 
  int appData; 
  int TagType = 0;

  // =========================================================== 
  // This is the point were you get data from the App 
  client = server.available(); 
  if (client){ 
    appData = client.read();
    Serial.print((char)appData); 
  }

  switch(appData){ 
  case 'A':
  digitalWrite(6, HIGH);
  server.println("<Butn02:1");
  server.println("<Imgs03:1");
  server.println("<Logr00: Pin 6 HIGH");
  break;

  case 'a':
  digitalWrite(6, LOW);
  server.println("<Butn02:0");
  server.println("<Imgs03:0");
  server.println("<Logr00: Pin 6 LOW");
  break;
  
  case CMD_SPECIAL: 
    // Special command received 
    // After this function accel and seek bar values are updated
    TagType = DecodeSpecialCommand();
    
    if(TagType == UPDATE_TPAD)
    // Print touch pad array
    for(int i = 0; i < TPAD_NO; i++){
      for(int j = 0; j < TPAD_AXIS; j++){
        if(TouchPadData[i][j] != 0) // don't display cero value pads
          Serial.println("MCU: Touchpad value: " + String(i) + ": " + String(TouchPadData[i][j]));
      }
    }

    if(TagType == UPDATE_SKBAR)
    // Print seekbar array
    for(int i = 0; i < SKBAR_NO; i++){
      Serial.println("MCU: Seekvar value: " + String(i) + ": " + String(SeekBarValue[i]));
    }    

    if(TagType == UPDATE_ACCEL)
    // Print accel array
    for(int i = 0; i < ACCEL_AXIS; i++){
      Serial.println("MCU: Accelerometer value " + String(i) + ": "  + String(Accel[i]));
    }

    if(TagType == UPDATE_SPCH)
    // Print speech recognition
    Serial.println("MCU: " + SpeechRecorder);
    if(SpeechRecorder.equals("encender")){
      digitalWrite(5, HIGH);
      server.println("<Logr00: Pin 5 LOW");
    }
    else if(SpeechRecorder.equals("apagar")){
      digitalWrite(5, LOW);
      server.println("<Logr00: Pin 6 LOW");
    }
    break; 

  case CMD_ALIVE: 
    // Character '[' is received every 2.5s
    server.println("Hola Mundo");
    server.println("<Text00:Hola Mundo");
    server.println("<Text01: A1 = " + String(analogRead(A1)));
    //server.println("<Skb0:" + String(myIntToString(analogRead(A1)>>2)));
    server.println("<Abar01:" +  String(myIntToString(analogRead(A1)>>2)));
    server.println("<Abar00:" +  String(myIntToString(analogRead(A1)>>2)));
    server.println("<Alrm00");
    server.println("<Vibr00:100");
   // server.println("<TtoS01:Hola mundo");
    Serial.println("MCU: Hola Mundo"); 
    break; 
  } 
  // ========================================================== 
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

// DecodeSpecialCommand
//
// A '<' flags a special command comming from App. Use this function
// to get Accelerometer data (and other sensors in the future)
// Input:
//   None
// Output:
//   None
int DecodeSpecialCommand(){
  int tagType = UPDATE_FAIL;
  
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
    tagType = UPDATE_ACCEL;
  }

  if(commandType.equals("AccY:")){
    // Next 6 characters will tell us the command data
    String commandData = thisCommand.substring(5, 11);
    if(commandData.charAt(0) == '-') // Negative acceleration
      Accel[1] = -commandData.substring(1, 6).toInt();
    else
      Accel[1] = commandData.substring(1, 6).toInt();
    tagType = UPDATE_ACCEL;
  }

  if(commandType.equals("AccZ:")){
    // Next 6 characters will tell us the command data
    String commandData = thisCommand.substring(5, 11);
    if(commandData.charAt(0) == '-') // Negative acceleration
      Accel[2] = -commandData.substring(1, 6).toInt();
    else
      Accel[2] = commandData.substring(1, 6).toInt();
    tagType = UPDATE_ACCEL;
  }

  if(commandType.substring(0, 4).equals("PadX")){
    // Next 2 characters will tell us the touch pad number
    int padNumber = thisCommand.substring(4, 6).toInt();
    // Next 3 characters are the X axis data in the message
    String commandData = thisCommand.substring(8, 13);
    TouchPadData[padNumber][0] = commandData.toInt();
    tagType = UPDATE_TPAD;
  }

  if(commandType.substring(0, 4).equals("PadY")){
    // Next 2 characters will tell us the touch pad number
    int padNumber = thisCommand.substring(4, 6).toInt();
    // Next 3 characters are the Y axis data in the message
    String commandData = thisCommand.substring(8, 13);
    TouchPadData[padNumber][1] = commandData.toInt();
    tagType = UPDATE_TPAD;
  }

  if(commandType.substring(0, 3).equals("Skb")){
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
String Readln(){ 
  char inByte = -1; 
  String message = ""; 

  while(inByte != '\n'){ 
    inByte = -1; 

    client = server.available(); 
    if (client)
      inByte = client.read(); 

    if(inByte != -1) 
      message.concat(String(inByte)); 
  } 
  Serial.print(message); 
  return message; 
}


