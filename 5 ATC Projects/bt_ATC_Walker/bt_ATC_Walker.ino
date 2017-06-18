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
 
 Author: Juan Luis Gonzalez Bello 
 Date: Nov 2015
 Get the app: https://play.google.com/store/apps/details?id=com.apps.emim.btrelaycontrol
 ** After copy-paste of this code, use Tools -> Atomatic Format 
 */

#include<Servo.h>
#include<EEPROM.h>

// Baud rate for ble module
#define BAUD_RATE 115200  

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
int SeekBarValue[8] = {90,90,90,90,90,90,90,90};
int TouchPadData[24][2]; // 24 max touch pad objects, each one has 2 axis (x and Y)
String SpeechRecorder = "";

// Sistem specific variables
#define SERVO_NO 8
#define INIT_POS 90

// Atmega328p has 2k ram and 1k EEPROM.
// Constants for eeprom addresses
#define F_EE_ADD    0
#define F_EE_PADD 170
#define B_EE_ADD  200
#define B_EE_PADD 370
#define R_EE_ADD  400
#define R_EE_PADD 570
#define L_EE_ADD  600
#define L_EE_PADD 770

// Commands to save (s), play (p) and delete (d) positions
#define CMD_S_F  'A'
#define CMD_P_F  'a'
#define CMD_D_F  '1' 
#define CMD_S_B  'B'
#define CMD_P_B  'b'
#define CMD_D_B  '2'
#define CMD_S_R  'C'
#define CMD_P_R  'c'
#define CMD_D_R  '3'
#define CMD_S_L  'D'
#define CMD_P_L  'd'
#define CMD_D_L  '4'
#define CMD_STOP 'S'
#define MAX_POSITIONS 10

// Robot walking status
#define ST_NONE 0
#define ST_F    1
#define ST_B    2
#define ST_R    3
#define ST_L    4

// Servo position limits
int MAX_POS[SERVO_NO] = {180, 144, 116, 144, 117, 157, 132, 126};
int MIN_POS[SERVO_NO] = {60,   54,  0,  57,  51,  50,  63,  31};

Servo mServo[SERVO_NO];
int ServoAngle[SERVO_NO];
int ServoPINS[SERVO_NO] = {2,3,4,5,6,7,8,9};
int ids[] = {16, 17, 18, 23}; // touch pad IDS
int RobotStatus = ST_NONE;

int FSpointer = 0;
int FPositions[MAX_POSITIONS][SERVO_NO];
int BSpointer = 0;
int BPositions[MAX_POSITIONS][SERVO_NO];
int LSpointer = 0;
int LPositions[MAX_POSITIONS][SERVO_NO];
int RSpointer = 0;
int RPositions[MAX_POSITIONS][SERVO_NO];

void mServoInit(){
  // Initialize servos
  for(int i = 0; i < SERVO_NO;i++){
    mServo[i].attach(ServoPINS[i]);
    ServoAngle[i] = INIT_POS;
    mServoSetPos(i, INIT_POS);
  }

  // Initialize touch pad value to 90 in order to avoid weird moves
  for(int i = 0; i < 24; i++){
    TouchPadData[i][0] = 90; //X
    TouchPadData[i][1] = 90; //Y
  }

  // Restore pointers from memory
  FSpointer = EEPROM.read(F_EE_PADD);
  if(FSpointer == 0xFF) FSpointer = 0;
  RSpointer = EEPROM.read(R_EE_PADD);
  if(RSpointer == 0xFF) FSpointer = 0;
  LSpointer = EEPROM.read(L_EE_PADD);
  if(LSpointer == 0xFF) FSpointer = 0;
  BSpointer = EEPROM.read(B_EE_PADD);
  if(BSpointer == 0xFF) BSpointer = 0;

  // Restore forward positions
  for(int i = 0; i < FSpointer; i++){
    for(int j = 0; j < SERVO_NO; j++){
      FPositions[i][j] = EEPROM.read(F_EE_ADD + (i * SERVO_NO) + j);
    }
  }

  // Restore backward positions
  for(int i = 0; i < BSpointer; i++){
    for(int j = 0; j < SERVO_NO; j++){
      BPositions[i][j] = EEPROM.read(B_EE_ADD + (i * SERVO_NO) + j);
    }
  }

  // Restore left positions
  for(int i = 0; i < LSpointer; i++){
    for(int j = 0; j < SERVO_NO; j++){
      LPositions[i][j] = EEPROM.read(L_EE_ADD + (i * SERVO_NO) + j);
    }
  }

  // Restore right positions
  for(int i = 0; i < RSpointer; i++){
    for(int j = 0; j < SERVO_NO; j++){
      RPositions[i][j] = EEPROM.read(R_EE_ADD + (i * SERVO_NO) + j);
    }
  }
}

void mServoSetPos(int servoNo, int value){
  if(value > MAX_POS[servoNo]) value = MAX_POS[servoNo];
  if(value < MIN_POS[servoNo]) value = MIN_POS[servoNo];
  mServo[servoNo].write(value);
  ServoAngle[servoNo] = value; 
}

void FSavePos(){
  if(FSpointer >= MAX_POSITIONS){
    Serial.println("<TtoS01:Maximas posiciones alcanzadas");
    return;
  }
  
  for(int i = 0; i < SERVO_NO; i++){
    FPositions[FSpointer][i] = ServoAngle[i];
    EEPROM.write(F_EE_ADD + FSpointer * SERVO_NO + i, ServoAngle[i]);
  }
  EEPROM.write(F_EE_PADD, FSpointer++);
}

void BSavePos(){
  if(BSpointer >= MAX_POSITIONS){
    Serial.println("<TtoS01:Maximas posiciones alcanzadas");
    return;
  }
  
  for(int i = 0; i < SERVO_NO; i++){
    BPositions[BSpointer][i] = ServoAngle[i];
    EEPROM.write(B_EE_ADD + BSpointer * SERVO_NO + i, ServoAngle[i]);
  }
  EEPROM.write(B_EE_PADD, BSpointer++);
}

void RSavePos(){
  if(RSpointer >= MAX_POSITIONS){
    Serial.println("<TtoS01:Maximas posiciones alcanzadas");
    return;
  }
  
  for(int i = 0; i < SERVO_NO; i++){
    RPositions[RSpointer][i] = ServoAngle[i];
    EEPROM.write(R_EE_ADD + RSpointer * SERVO_NO + i, ServoAngle[i]);
  }
  EEPROM.write(R_EE_PADD, RSpointer++);
}

void LSavePos(){
  if(LSpointer >= MAX_POSITIONS){
    Serial.println("<TtoS01:Maximas posiciones alcanzadas");
    return;
  }
  
  for(int i = 0; i < SERVO_NO; i++){
    LPositions[LSpointer][i] = ServoAngle[i];
    EEPROM.write(L_EE_ADD + LSpointer * SERVO_NO + i, ServoAngle[i]);
  }
  EEPROM.write(L_EE_PADD, LSpointer++);
}

void Play(int aPointer, int aPosArray[][8]){
  if(aPointer == 0) return;
  for(int i = 0; i < aPointer; i++){
    for(int j = 0; j < SERVO_NO; j++){
      mServoSetPos(j, aPosArray[i][j]);
    }
    delay(500);
  }
}

void setup() {   
  // initialize BT Serial port
  Serial.begin(BAUD_RATE); 
  mServoInit();
  Serial.println("<TtoS01:caminante en linea");
  Serial.println("Caminante en linea");
} 

void loop() { 
  int appData; 
  static int prescaler = 0;
  static int selector = 0;
  delay(1);

  // Print angles in app
  if(prescaler++ > 50){
    prescaler = 0;
    selector = (++selector) % 8;
    Serial.println("<Text0" + String(selector) + ":" + String(ServoAngle[selector]));
  }

  // Play the position arrays according to robot status
  switch(RobotStatus){
    case ST_F:
      Play(FSpointer, FPositions);
    break;

    case ST_B:
      Play(BSpointer, BPositions);
    break;

    case ST_R:
      Play(RSpointer, RPositions);
    break;

    case ST_L:
      Play(LSpointer, LPositions);
    break;
  }
  
  // ================================================================= 
  // This is the point were you get data from the App 
  appData = Serial.read();   // Get a byte from app, if available 
  switch(appData){ 
  case CMD_SPECIAL: 
    // Special command received, seekbar value and accel value updates 
    DecodeSpecialCommand();
    
    /* Used for testing with seekbars
    for(int i = 0; i < SERVO_NO; i++){
      mServoSetPos(i, (SeekBarValue[i]*9)/13);
    }
    */
    
    for(int i = 0; i < (SERVO_NO / 2); i++){
      mServoSetPos(i * 2, (TouchPadData[ids[i]][0]*9)/13);
      mServoSetPos((i * 2) + 1, (TouchPadData[ids[i]][1]*9)/13);
    }
    Serial.println("Touch pad received");
    break; 

  case CMD_ALIVE: 
    // Character '[' is received every 2.5s
    Serial.println("ATC Walker ready");
    break;

  case CMD_S_F:
    FSavePos();
  break;

  case CMD_P_F:
    RobotStatus = ST_F;
  break;

  case CMD_D_F:
    FSpointer = 0;
    EEPROM.write(F_EE_PADD, FSpointer);
  break;

  case CMD_S_B:
    BSavePos();
  break;

  case CMD_P_B:
    RobotStatus = ST_B;
  break;

  case CMD_D_B:
    BSpointer = 0;
    EEPROM.write(B_EE_PADD, BSpointer);
  break;

  case CMD_S_R:
    RSavePos();
  break;

  case CMD_P_R:
    RobotStatus = ST_R;
  break;

  case CMD_D_R:
    RSpointer = 0;
    EEPROM.write(R_EE_PADD, RSpointer);
  break;

  case CMD_S_L:
    LSavePos();
  break;

  case CMD_P_L:
    RobotStatus = ST_L;
  break;

  case CMD_D_L:
    LSpointer = 0;
    EEPROM.write(L_EE_PADD, LSpointer);
  break;

  case CMD_STOP:
    RobotStatus = ST_NONE;
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
