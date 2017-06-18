/*Arduino Total Control Firmware
 REMEMBER TO DISCONNECT BLUETOOT TX PIN WHEN UPLOADING SKETCH
 Program to control an RC car from ATC

 * Bluetooth Module attached to Serial port
 * L298N double H bridge attached to pins 2,3,4,5
 * L298N enable pin attached to pin 6 for speed control.
 * Front lights (LEDS) connected to pin 7
 * Rear light Right connected to pin 8
 * Rear light Left connected to pin 9
 * ACS712 sensor connected to pin A1

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

 Author: Juan Luis Gonzalez Bello
 Date: May 2017
 Get the app: https://play.google.com/store/apps/details?id=com.apps.emim.btrelaycontrol
 */

// Baud rate for bluetooth module
// (Default 9600 for most modules)
#define BAUD_RATE 115200 // Tip: Configure your bluetooth device for 115200 for better performance  

// Special commands
#define CMD_SPECIAL '<'
#define CMD_ALIVE   '['

#define UPDATE_FAIL  0
#define UPDATE_ACCEL 1
#define UPDATE_SKBAR 2
#define UPDATE_TPAD  3
#define UPDATE_SPCH  4

// Data and variables received from especial command
int Accel[3] = {0, 0, 0};
int SeekBarValue[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int TouchPadData[24][2]; // 24 max touch pad objects, each one has 2 axis (x and Y)
String SpeechRecorder = "";

// System specific outputs
#define O_IN1 2
#define O_IN2 3
#define O_IN3 4
#define O_IN4 5
#define O_ENB 6
#define O_FL  7
#define O_RL  8
#define O_LL  9

// System specific inputs
#define I_CURRENT A1
#define I_RX      0

// System specific commands
#define CMD_D       'A' // Adelante
#define CMD_N       'a' // Neutral
#define CMD_R       'B' // Reversa
#define CMD_RIGHT   'C'
#define CMD_CENTER  'c'
#define CMD_LEFT    'D'
#define CMD_LON     'E'
#define CMD_LOFF    'e'
#define CMD_ECO_ON  'F'
#define CMD_ACC_ON  'G'
#define CMD_ACC_OFF 'g'

String CMD_SLON  = "luces";
String CMD_SLOFF = "luces fuera";
String ID_RT     = "07";
String ID_LT     = "06";
String ID_ECO    = "05";
String ID_LIGHT  = "04";
String ID_BLIGHT = "00";

// System specific variables
boolean EcoMode     = false;
boolean AccelMode   = false;
boolean FrontLights = false;
int DriveCurrent = 0;
int DriveSpeed = 0;

#define ST_N  0
#define ST_D  1
#define ST_R  2
int DriveStatus = ST_N;

#define ST_NONE        0 // Apagadas
#define ST_BREAK       1 // Freno (ambas luces encendidas)
#define ST_TURN_LEFT   2 // Direccional izquierda (parpadeando)
#define ST_TURN_RIGHT  3 // Direccional derecha (parpadeando)
int DirLightStatus = ST_NONE;

void setup() {
  // initialize BT Serial port
  Serial.begin(BAUD_RATE);

  // Initialize inputs and outputs
  pinMode(O_IN1, OUTPUT);
  pinMode(O_IN2, OUTPUT);
  pinMode(O_IN3, OUTPUT);
  pinMode(O_IN4, OUTPUT);
  pinMode(O_ENB, OUTPUT);
  pinMode(O_RL, OUTPUT);
  pinMode(O_LL, OUTPUT);
  pinMode(O_FL, OUTPUT);
  pinMode(I_RX, INPUT_PULLUP);
  pinMode(I_CURRENT, INPUT);

  digitalWrite(O_IN1, LOW);
  digitalWrite(O_IN2, LOW);
  digitalWrite(O_IN3, LOW);
  digitalWrite(O_IN4, LOW);
  digitalWrite(O_LL, LOW);
  digitalWrite(O_RL, LOW);
  digitalWrite(O_FL, LOW);
  digitalWrite(O_ENB, HIGH);

  // Greet the app
  Serial.println("<Imgs" + ID_LIGHT + ":0");
  Serial.println("<TtoS01:Vehiculo ATC en linea");
}

void loop() {
  int tagType;
  int appData;
  delay(1);

  CurrentSensing();
  FrontLightsControl(FrontLights);
  RearLightSM(DirLightStatus);
  DriveSM(DriveStatus);

  // =================================================================
  // This is the point were you get data from the App
  appData = Serial.read();   // Get a byte from app, if available
  switch (appData) {
    case CMD_SPECIAL:
      // Special command received, seekbar value and accel value updates
      tagType = DecodeSpecialCommand();

      // Activate lights with voice
      if (tagType == UPDATE_SPCH) {
        if (SpeechRecorder.equals(CMD_SLON)) {
          FrontLights = true;
          Serial.println("<Imgs" + ID_LIGHT + ":1");
          Serial.println("<Butn" + ID_BLIGHT + ":1");
        }
        if (SpeechRecorder.equals(CMD_SLOFF)) {
          FrontLights = false;
          Serial.println("<Imgs" + ID_LIGHT + ":0");
          Serial.println("<Butn" + ID_BLIGHT + ":0");
        }
      }

      // Update drive speed value
      DriveSpeed = SeekBarValue[0];
      Serial.println("<Text03:" + myIntToString(DriveSpeed));

      // Enable accelerometer control
      if (AccelMode) {
        if (Accel[1] > 200) {
          digitalWrite(O_IN3, LOW);
          digitalWrite(O_IN4, HIGH);
          DirLightStatus = ST_TURN_LEFT;
        }
        else if (Accel[1] < -200) {
          digitalWrite(O_IN3, HIGH);
          digitalWrite(O_IN4, LOW);
          DirLightStatus = ST_TURN_RIGHT;
        }
        else {
          digitalWrite(O_IN3, LOW);
          digitalWrite(O_IN4, LOW);
          DirLightStatus = ST_NONE;
        }
      }
      break;

    case CMD_ALIVE:
      // Character '[' is received every 2.5s
      Serial.println("ATC RC Car Ready");
      break;

    case CMD_D:
      DriveStatus = ST_D;
      break;

    case CMD_N:
      DriveStatus    = ST_NONE;
      DirLightStatus = ST_NONE;
      break;

    case CMD_R:
      DriveStatus    = ST_R;
      DirLightStatus = ST_BREAK;
      break;

    case CMD_RIGHT:
      digitalWrite(O_IN3, HIGH);
      digitalWrite(O_IN4, LOW);
      DirLightStatus = ST_TURN_RIGHT;
      break;

    case CMD_LEFT:
      digitalWrite(O_IN3, LOW);
      digitalWrite(O_IN4, HIGH);
      DirLightStatus = ST_TURN_LEFT;
      break;

    case CMD_CENTER:
      digitalWrite(O_IN3, LOW);
      digitalWrite(O_IN4, LOW);
      DirLightStatus = ST_NONE;
      break;

    case CMD_LON:
      Serial.println("<TtoS01: encender luces");
      Serial.println("<Imgs" + ID_LIGHT + ":1");
      FrontLights = true;
      break;

    case CMD_LOFF:
      Serial.println("<TtoS01: apagar luces");
      Serial.println("<Imgs" + ID_LIGHT + ":0");
      FrontLights = false;
      break;

    case CMD_ECO_ON:
      Serial.println("<Skb0:" + myIntToString(DriveSpeed));
      if (EcoMode) {
        Serial.println("<TtoS01: modo eco apagado");
        Serial.println("<Imgs" + ID_ECO + ":0");
        EcoMode = false;
      }
      else {
        Serial.println("<TtoS01: modo eco activado");
        Serial.println("<Imgs" + ID_ECO + ":1");
        EcoMode = true;
      }
      break;

    case CMD_ACC_ON:
      Serial.println("<TtoS01: modo acelerometro activado");
      AccelMode = true;
      break;

    case CMD_ACC_OFF:
      Serial.println("<TtoS01: modo acelerometro desactivado");
      AccelMode = false;
      break;
  }
  // =================================================================
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
 * Current sensing
 */
void CurrentSensing(){
  static int prescaler = 0;
  
  if (prescaler++ > 100) {
    prescaler = 0;
    int analogValue = analogRead(I_CURRENT);
    DriveCurrent = ((((analogValue - 512) * 74) / 10) + DriveCurrent) >> 1; // Basic low pass (average) filter
    Serial.println("<Text00: Corriente: " + String(DriveCurrent / 100) + "." 
                      + myIntToString((abs(DriveCurrent) % 100) * 10) + "A");
    Serial.println("<Text01: A1: " + String(analogValue));
    Serial.println("<Abar00:" + myIntToString(abs(DriveCurrent)));
  }
}

/**
 * Front lights control
 */
void FrontLightsControl(boolean lightStatus){
  // Front light control
  if (lightStatus) {
    digitalWrite(O_FL, HIGH);
  }
  else {
    digitalWrite(O_FL, LOW);
  }
}

/**
 * State machine for controlling drive power
 */
void DriveSM(int myStatus) {
  static int theSpeed = 0;
  static bool skipThis = false;

  switch (myStatus) {
    case ST_N:
      digitalWrite(O_IN1, LOW);
      digitalWrite(O_IN2, LOW);
      theSpeed = 0;
      break;

    case ST_D:
      digitalWrite(O_IN1, HIGH);
      digitalWrite(O_IN2, LOW);
      if (EcoMode) {
        if (skipThis) {
          if (theSpeed < DriveSpeed) theSpeed++;
          analogWrite(O_ENB, theSpeed);
        }
        skipThis = !skipThis;
      }
      else {
        analogWrite(O_ENB, DriveSpeed);
      }
      break;

    case ST_R:
      digitalWrite(O_IN1, LOW);
      digitalWrite(O_IN2, HIGH);
      theSpeed = 0;
      break;
  }
}

/**
 * State machine for rear light control (break or turn)
 */
void RearLightSM(int myStatus) {
  static int dir_prescaler = 0;
  static boolean onlyOnce = false;

  switch (myStatus) {
    case ST_NONE:
      digitalWrite(O_RL, LOW);
      digitalWrite(O_LL, LOW);
      if (onlyOnce) {
        Serial.println("<Imgs" + ID_RT + ":0");
        Serial.println("<Imgs" + ID_LT + ":0");
      }
      break;

    case ST_BREAK:
      digitalWrite(O_RL, HIGH);
      digitalWrite(O_LL, HIGH);
      break;

    case ST_TURN_LEFT:
      Serial.println("<Imgs" + ID_RT + ":0");
      digitalWrite(O_RL, LOW);
      dir_prescaler++;
      onlyOnce = true;
      if (dir_prescaler > 500) {
        dir_prescaler = 0;
        Serial.println("<Imgs" + ID_LT + ":" + String(digitalRead(O_LL) ? 0 : 1));
        digitalWrite(O_LL, digitalRead(O_LL) ? LOW : HIGH);
      }
      break;

    case ST_TURN_RIGHT:
      Serial.println("<Imgs" + ID_LT + ":0");
      digitalWrite(O_LL, LOW);
      dir_prescaler++;
      onlyOnce = true;
      if (dir_prescaler > 500) {
        dir_prescaler = 0;
        Serial.println("<Imgs" + ID_RT + ":" + String(digitalRead(O_RL) ? 0 : 1));
        digitalWrite(O_RL, digitalRead(O_RL) ? LOW : HIGH);
      }
      break;
  }
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
 * Readln
 * Use this function to read a String line from Bluetooth
 * returns: String message, note that this function will pause the program
 *          until a hole line has been read.
 */
String Readln() {
  char inByte = -1;
  String message = "";

  while (inByte != '\n') {
    inByte = -1;

    if (Serial.available() > 0)
      inByte = Serial.read();

    if (inByte != -1)
      message.concat(String(inByte));
  }

  return message;
}
