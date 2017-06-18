/*Arduino Total Control Firmware
 REMEMBER TO DISCONNECT BLUETOOT TX PIN WHEN UPLOADING SKETCH
 Basic functions to control and display information into the app.This code works for every arduino board.
 
 * Bluetooth Module attached to Serial port (pins 0,1) 
 * H - Bridge attached to pins 2,3,4,5
 * Himidity sensor attached to A0
 
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
 
 Author: Juan Luis Gonzalez Bello 
 Date: Nov 2015
 Get the app: https://play.google.com/store/apps/details?id=com.apps.emim.btrelaycontrol
 ** After copy-paste of this code, use Tools -> Atomatic Format 
 */

// Baud rate for bluetooth module
#define BAUD_RATE 9600 // Tip: Configure your bluetooth device for 115200 for better performance  

// Special commands
#define CMD_SPECIAL '<'
#define CMD_ALIVE   '['

// Data and variables received from especial command
int Accel[3] = {0, 0, 0}; 
int SeekBarValue[8] = {0,0,0,0,0,0,0,0};
int TouchPadData[24][2]; // 24 max touch pad objects, each one has 2 axis (x and Y)

// Peristaltic pump variables
#define PP_STEP_DELAY   50 // step delay in ms
#define HUMIDITY_SENSOR A0 // Humidity sensor input pin
#define HB_PIN_NO        4 // Number of pins used in H bridge
#define SM_NO_STEPS      8 // Number of different steps

int HBridgePins[HB_PIN_NO] = {5, 4, 3, 2};
int STEP_MOTOR_SEQUENCE[SM_NO_STEPS][HB_PIN_NO] = {{0,0,0,1},
                                                         {0,0,1,1},
                                                         {0,0,1,0},
                                                         {0,1,1,0},
                                                         {0,1,0,0},
                                                         {1,1,0,0},
                                                         {1,0,0,0},
                                                         {1,0,0,1}};
int StepNumber = 0;

// Peristaltic pump commands
#define CMD_PP_10STEPS_CW   'A'
#define CMD_PP_100STEPS_CW  'B'
#define CMD_PP_10STEPS_CCW  'C'
#define CMD_PP_100STEPS_CCW 'D'
#define CMD_PP_DO_NOTHING   'x'

void setup() {   
  pinMode(7, OUTPUT);   // Set as output
  digitalWrite(7, LOW); // And turn off 
  
  // initialize BT Serial port
  Serial.begin(BAUD_RATE);

  // Set output pins
  for(int i = 0; i < HB_PIN_NO; i++){
    pinMode(HBridgePins[i], OUTPUT);   // Set as output
    digitalWrite(HBridgePins[i], LOW); // And turn off 
  }

  // Set input pins
  pinMode(HUMIDITY_SENSOR, INPUT);  // Ensure its input
  pinMode(0, INPUT_PULLUP);         // Needed on some boards
} 

void loop() { 
  static int prescaler = 0;
  int appData; 

  // This is to create a virtual "heartbeat"
  delay(1);
  
  // This is true each 1/10 of a second approx
  if(prescaler++ > 100){
    prescaler = 0;
    Serial.println("<Text01: Humidity value: " + String(analogRead(HUMIDITY_SENSOR)));
  }
  
  // =========================================================== 
  // This is the point were you get data from the App 
  appData = Serial.read();   // Get a byte from app, if available 
  switch(appData){ 
  case CMD_SPECIAL: 
    // Special command received, seekbar value and accel value updates 
    DecodeSpecialCommand();
    break; 

  case CMD_ALIVE: 
    // Character '[' is received every 2.5s
    Serial.println("ATC Peri Pump");
    break;

  case CMD_PP_10STEPS_CW: 
    Serial.println("<Text00:Rotating 10 steps clockwise");
    PP_Rotate(10, true);
    break;
  
  case CMD_PP_100STEPS_CW: 
    Serial.println("<Text00:Rotating 100 steps clockwise");
    PP_Rotate(100, true);
    break;
  
  case CMD_PP_10STEPS_CCW: 
    Serial.println("<Text00:Rotating 10 steps counter clockwise");
    PP_Rotate(10, false);
    break;
  
  case CMD_PP_100STEPS_CCW: 
    Serial.println("<Text00:Rotating 100 steps counter clockwise");
    PP_Rotate(100, false);
    break;
  } 
  // ========================================================== 
} 

// PP_Rotate
// Steps: number of steps to rotate
// dir: true: CW, false: CCW
void PP_Rotate(int steps, boolean dir){
  for(int i = 0; i < steps; i++){
    StepNumber = (++StepNumber) % SM_NO_STEPS; // StepNumber goes from 0 to 7
    for(int j = 0; j < HB_PIN_NO; j++){
      if(dir)
        digitalWrite(HBridgePins[j], STEP_MOTOR_SEQUENCE[StepNumber][j]);
      else
        digitalWrite(HBridgePins[j], STEP_MOTOR_SEQUENCE[7 - StepNumber][j]);
    }
    Serial.println("Rotating");
    delay(PP_STEP_DELAY);
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
    int padNumber = thisCommand.substring(5, 7).toInt();
    // Next 3 characters are the X axis data in the message
    String commandData = thisCommand.substring(9, 12);
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
    String commandData = thisCommand.substring(4, 6);
    int sbNumber = commandType.charAt(3) & ~0x30;
    SeekBarValue[sbNumber] = commandData.substring(8, 13).toInt();
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
