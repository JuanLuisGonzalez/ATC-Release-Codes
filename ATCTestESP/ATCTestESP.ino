/*Arduino Total Control ESP Firmware
 REMEMBER TO DISCONNECT BLUETOOT TX PIN WHEN UPLOADING SKETCH
 Basic functions to control and display information into the app.This code works for every arduino board.
 
 ESP test on arduino pins serial 1 18 and 19
 works well with 3.6V on vcc med overheat
 Server on port 80:
 Main commands
 AT+RST
 AT+GMR
 AT+CWMODE=3
 AT+CWJAP="Integra","6361726E65"
 AT+CIFSR
 AT+CIPMUX=1
 AT+CIPSERVER=1,80
 When data received: +IPD,0,2:[ (for this exaple 0 is the channel, and 2 is the data lenght)
 Rare error down, blue module uses 1 as channel and black one uses 0
 To send data AT+CIPSEND=0,3 (0: channel, 3 data size)

 You may use the following to monitor what is coming into arduino
 
  if(Serial1.available()){
    Serial.write(Serial1.read());
  }

  if(Serial.available()){
    Serial1.write(Serial.read());
  }

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

// Baud rate for ESP module
#define BAUD_RATE 115200
String sPort = "80";

// Special commands
#define CMD_SPECIAL '<'
#define CMD_ALIVE   '['

// Data and variables received from especial command
int Accel[3] = {0, 0, 0}; 
int SeekBarValue[8] = {0,0,0,0,0,0,0,0};
int TouchPadData[24][2]; // 24 max touch pad objects, each one has 2 axis (x and Y)

void setup() {   
  Serial.begin(BAUD_RATE);
  myESP_Init();
  
  // Initialize touch pad data, 
  // this is to avoid having random numbers in them
  for(int i = 0; i < 24; i++){
    TouchPadData[i][0] = 0; //X
    TouchPadData[i][1] = 0; //Y
  }
  
  pinMode(13, OUTPUT);
} 

void loop() { 
  int appData = -1; 
  String appMessage;
  static int counter = 0;
  static int gato = 0;
  //delay(1);

  if(counter++ >= 100){
     myESP_Println("<Text03:Any counter: " + String(gato++), 0); // send on channel 0
     counter = 0;
  }
  
  // =========================================================== 
  // This is the point were you get data from the App 
  if(Serial1.available()){
    appMessage = myESP_Read(0);    // Read channel 0
    appData = appMessage.charAt(0);
  }
  switch(appData){ 
  case CMD_SPECIAL: 
    // Special command received, seekbar value and accel value updates 
    DecodeSpecialCommand(appMessage.substring(1));// we already have the data :)
    analogWrite(13, SeekBarValue[5]);
    break; 

  case CMD_ALIVE: 
    // Character '[' is received every 2.5s
    myESP_Println("ATC using ESP8266 ready", 0);
    
    // If you need to update the seek bar values uncomment the below code
    // WARNING increase the baud rate up to 115200 in order to have a
    // smooth operation
    // Update seekbar Values
    /*
    for(int i = 0; i < 8; i++){
      myESP_Println("<Skb" + String(i) + ":" + SBtoString(SeekBarValue[i]));
    }
    */
    break;

  case 'E':
    digitalWrite(13, HIGH);
  break;

  case 'e':
    digitalWrite(13, LOW); 
  break;
  } 
  // ========================================================== 
} 

String SBtoString(int value){
  String sValue = "";
  if(value < 10)
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
void DecodeSpecialCommand(String thisCommand){ 
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

    if (Serial1.available() > 0) 
      inByte = Serial1.read(); 

    if(inByte != -1) 
      message.concat(String(inByte)); 
  } 

  return message; 
}

// Set up wifi module on Serial1, esp on channel 0
void myESP_Init(){
  // Initialize serial port
  Serial1.begin(BAUD_RATE);
  pinMode(19, INPUT_PULLUP);
  
  // Reset module
  Serial1.println("AT+RST");
  delay(4000);
  Serial1.println("AT+CIPMUX=1");
  delay(500);
  Serial1.println("AT+CIPSERVER=1," + sPort);
  delay(500);
}

// Send string data to app
void myESP_Println(String data, int channel){
  // Get data size, add 2 for nl and cr
  int dataSize = data.length() + 2;
  
  // submit cipsend command for channel
  Serial1.println("AT+CIPSEND=" + String(channel) + "," + String(dataSize));
  delay(10); // needed?
  
  // Print actual data
  Serial1.println(data);
}

// Receive string data from app (a char can be tough as a single character string)
// remember: +IPD,0,2:[ (for this exaple 0 is the channel, and 2 is the data lenght)
String myESP_Read(int channel){
  String dataFromClient = "";
  String message = "";
  
  // only if character '+' is received
  if(Serial1.read() == '+'){
    dataFromClient = Readln();                                   // read the whole line
    String sCommand = dataFromClient.substring(0, 3);
    if(sCommand.equals("IPD")){                                  // check this is actually the command we expect
      int clientChannel = dataFromClient.charAt(4) & ~0x30;      // convert to int
      if(clientChannel == channel){                              // only return data if it is from the intended channel
         int twoPointsIndex = dataFromClient.indexOf(':');       // find the ':' on the command
         message = dataFromClient.substring(twoPointsIndex + 1); // set the message
      }
    }
  }
  return message;
}



