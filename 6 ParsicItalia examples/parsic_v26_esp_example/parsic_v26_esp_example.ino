/*Arduino Total Control ESP Firmware
 REMEMBER TO DISCONNECT BLUETOOT TX PIN WHEN UPLOADING SKETCH
 Basic functions to control and display information into the app.This code works for every arduino board.
 
 ESP test on arduino pins serial 1 18 and 19
 works well with 3.6V on vcc med overheat
 Server on port "sPort":
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

#include <EEPROM.h>

// Baud rate for ESP module
#define BAUD_RATE 115200
String sPort = "80";

#define ChipSelect1 3
#define ChipSelect2 4
#define myMISO     50 // UNO: 12 MEGA: 50
#define myMOSI     51 // UNO: 11 MEGA: 51
#define mySCK      52 // UNO: 13 MEGA: 52
#define mySS       53 // UNO: 10 MEGA: 53
#define V26_ADDRESS 7

// Special commands
#define CMD_SPECIAL '<'
#define CMD_ALIVE   '['

// Data and variables received from especial command
int Accel[3] = {0, 0, 0}; 
int SeekBarValue[8] = {0,0,0,0,0,0,0,0};

// Number of relays
#define MAX_RELAYS 8 
#define MAX_INPUTS 3

// Relay 1 will report status to toggle button and image 1, relay 2 to button 2 and so on.
boolean buttonLatch[] = {false, false, false};
String RelayAppId[] = {"04", "05", "06", "07", "08", "09", "10", "11"};
// Command list (turn on - off for eachr relay)
const char CMD_ON[] = {'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L'};
const char CMD_OFF[] = {'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l'};

// Used to keep track of the relay status in EEPROM
int RelayStatus = 0; 
int STATUS_EEADR = 20; 
int Prescaler = 0;

void setup() {   
  Serial.begin(BAUD_RATE);
  myESP_Init();
  ParsicInit(V26_ADDRESS);

  // Load last known status from eeprom 
  RelayStatus = EEPROM.read(STATUS_EEADR); 
  for(int i = 0; i < MAX_RELAYS; i++){ 
    // Turn on and off according to relay status
    String stringNumber = String(i);
    if((RelayStatus & (1 << i)) == 0)
      ParsicDigitalWrite(V26_ADDRESS, i, 0);
    else 
      ParsicDigitalWrite(V26_ADDRESS, i, 1); 
  }   
  pinMode(13, OUTPUT);
} 

void loop() { 
  int appData = -1; 
  int iSample = 0;
  String appMessage;
  String sSample = "";
  delay(1); 
  
  // This is true each 1/10 second approx. 
  if(false){//Prescaler++ > 1000){ 
    Prescaler = 0; // Reset prescaler 

    // Send 4 analog samples from V26 to be displayed at text tags 
    // Example of how to use text and imgs tags 
    iSample = ParsicAnalogRead(1);
    sSample = String(iSample);
    myESP_Println("<Text00:Temp1: " + sSample, 0);
    if(iSample > 683)
      myESP_Println("<Imgs00:1", 0); // Pressed state
    else if(iSample > 341)
      myESP_Println("<Imgs00:2", 0); // Extra state
    else
      myESP_Println("<Imgs00:0", 0); // Default state    

    iSample = ParsicAnalogRead(2);
    sSample = String(iSample);
    myESP_Println("<Text01:Speed2: " + sSample, 0);
    if(iSample > 683)
      myESP_Println("<Imgs01:1", 0); // Pressed state
    else if(iSample > 341)
      myESP_Println("<Imgs01:2", 0); // Extra state
    else
      myESP_Println("<Imgs01:0", 0); // Default state    
      
    iSample = ParsicAnalogRead(3);
    sSample = String(iSample);
    myESP_Println("<Text02:Photo3: " + sSample, 0);
    if(iSample > 683)
      myESP_Println("<Imgs02:1", 0); // Pressed state
    else if(iSample > 341)
      myESP_Println("<Imgs02:2", 0); // Extra state
    else
      myESP_Println("<Imgs02:0", 0); // Default state    
     
    iSample = ParsicAnalogRead(4);
    sSample = String(iSample); 
    myESP_Println("<Text03:Flux4: " + sSample, 0);
    if(iSample > 683)
      myESP_Println("<Imgs03:1", 0); // Pressed state
    else if(iSample > 341)
      myESP_Println("<Imgs03:2", 0); // Extra state
    else
      myESP_Println("<Imgs03:0", 0); // Default state      
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
    for(int i = 0; i < MAX_RELAYS; i++){ 
      // Refresh button states to app (<BtnXX:Y\n)
      if(ParsicDigitalReadOutput(V26_ADDRESS, i)){ 
        myESP_Println("<Butn" + RelayAppId[i] + ":1", 0);
        myESP_Println("<Imgs" + RelayAppId[i] + ":1", 0);
      } 
      else {
        myESP_Println("<Butn" + RelayAppId[i] + ":0", 0); 
        myESP_Println("<Imgs" + RelayAppId[i] + ":0", 0);
      } 
    } 
    break;

  default:
    // If not '<' or '[' then appData may be for relays
    for(int i = 0; i < MAX_RELAYS; i++){
      if(appData == CMD_ON[i]){
        digitalWrite(13, HIGH);
        // Example of how to make beep alarm sound
        myESP_Println("<Alrm00", 0);
        setRelayState(i, 1);
      }
      else if(appData == CMD_OFF[i]){
        digitalWrite(13, LOW);
        setRelayState(i, 0);
      }
    }
  } 

  // Manual buttons on inputs
  for(int i = 0; i < MAX_INPUTS; i++){
    if(!ParsicDigitalRead(V26_ADDRESS, i)){ // If button pressed
      // don't change relay status until button has been released and pressed again
      if(buttonLatch[i]){ 
        setRelayState(i, !ParsicDigitalReadOutput(V26_ADDRESS, i)); // toggle relay 0 state
        buttonLatch[i] = false;                       
      }
    }
    else{
      // button released, enable next push
      buttonLatch[i] = true;
    }
  }
  // ========================================================== 
} 

// Sets the relay state for this example
// relay: 0 to 7 relay number
// state: 0 is off, 1 is on
void setRelayState(int relay, int state){  
  if(state == 1){ 
    ParsicDigitalWrite(V26_ADDRESS, relay, 1);          // Write ouput port
    myESP_Println("<Butn" + RelayAppId[relay] + ":1\n<Imgs" + RelayAppId[relay] + ":1" , 0); // Feedback button state to app
    //myESP_Println("<Butn" + RelayAppId[relay] + ":1", 0); // Feedback button state to app
    //myESP_Println("<Imgs" + RelayAppId[relay] + ":1", 0); // Set image to pressed state
    
    RelayStatus |= (0x01 << relay);                 // Set relay status
    EEPROM.write(STATUS_EEADR, RelayStatus);        // Save new relay status
  } 
  else {
    ParsicDigitalWrite(V26_ADDRESS, relay, 0);          // Write ouput port
    myESP_Println("<Butn" + RelayAppId[relay] + ":0\n<Imgs" + RelayAppId[relay] + ":0" , 0); // Feedback button state to app
    //myESP_Println("<Imgs" + RelayAppId[relay] + ":0", 0); // Set image to default state
   
    RelayStatus &= ~(0x01 << relay);                // Clear relay status
    EEPROM.write(STATUS_EEADR, RelayStatus);        // Save new relay status
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
void DecodeSpecialCommand(String thisCommand){ 
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
  delay(30);
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

//                     Bit meaning                                             Reset Value
#define IODIRA   0x00 // IO7 IO6 IO5 IO4 IO3 IO2 IO1 IO0                         1111 1111
#define IPOLA    0x01 // IP7 IP6 IP5 IP4 IP3 IP2 IP1 IP0                         0000 0000
#define GPINTENA 0x02 // GPINT7 GPINT6 GPINT5 GPINT4 GPINT3 GPINT2 GPINT1 GPINT0 0000 0000
#define DEFVALA  0x03 // DEF7 DEF6 DEF5 DEF4 DEF3 DEF2 DEF1 DEF0                 0000 0000
#define INTCONA  0x04 // IOC7 IOC6 IOC5 IOC4 IOC3 IOC2 IOC1 IOC0                 0000 0000
#define IOCON    0x05 // BANK MIRROR SEQOP DISSLW HAEN ODR INTPOL —             0000 0000
#define GPPUA    0x06 // PU7 PU6 PU5 PU4 PU3 PU2 PU1 PU0                         0000 0000
#define INTFA    0x07 // INT7 INT6 INT5 INT4 INT3 INT2 INT1 INTO                 0000 0000
#define INTCAPA  0x08 // ICP7 ICP6 ICP5 ICP4 ICP3 ICP2 ICP1 ICP0                 0000 0000
#define GPIOA    0x09 // GP7 GP6 GP5 GP4 GP3 GP2 GP1 GP0                         0000 0000
#define OLATA    0x0A // OL7 OL6 OL5 OL4 OL3 OL2 OL1 OL0                         0000 0000

#define IODIRB   0x10 // IO7 IO6 IO5 IO4 IO3 IO2 IO1 IO0                         1111 1111
#define IPOLB    0x11 // IP7 IP6 IP5 IP4 IP3 IP2 IP1 IP0                         0000 0000
#define GPINTENB 0x12 // GPINT7 GPINT6 GPINT5 GPINT4 GPINT3 GPINT2 GPINT1 GPINT0 0000 0000
#define DEFVALB  0x13 // DEF7 DEF6 DEF5 DEF4 DEF3 DEF2 DEF1 DEF0                 0000 0000
#define INTCONB  0x14 // IOC7 IOC6 IOC5 IOC4 IOC3 IOC2 IOC1 IOC0                 0000 0000
#define IOCON    0x15 // BANK MIRROR SEQOP DISSLW HAEN ODR INTPOL —             0000 0000
#define GPPUB    0x16 // PU7 PU6 PU5 PU4 PU3 PU2 PU1 PU0                         0000 0000
#define INTFB    0x17 // INT7 INT6 INT5 INT4 INT3 INT2 INT1 INTO                 0000 0000
#define INTCAPB  0x18 // ICP7 ICP6 ICP5 ICP4 ICP3 ICP2 ICP1 ICP0                 0000 0000
#define GPIOB    0x19 // GP7 GP6 GP5 GP4 GP3 GP2 GP1 GP0                         0000 0000
#define OLATB    0x1A // OL7 OL6 OL5 OL4 OL3 OL2 OL1 OL0                         0000 0000

#define ChipSelect1 3
#define ChipSelect2 4
#define myMISO     50 // UNO: 12 MEGA: 50
#define myMOSI     51 // UNO: 11 MEGA: 51
#define mySCK      52 // UNO: 13 MEGA: 52
#define mySS       53 // UNO: 10 MEGA: 53
#define V26_ADDRESS 7

void ParsicInit(int devAdd){
  /*Special code for parsic*/
  pinMode(ChipSelect1, OUTPUT);        // Chip Select Pin
  pinMode(ChipSelect2, OUTPUT);        // Chip Select Pin
  digitalWrite(ChipSelect1, HIGH);     // Active Low
  digitalWrite(ChipSelect2, HIGH);     // Active Low
  mySPI_Init();
  
  // Initialize Digital IO expander
  /* ERRATA: no matter if address pins are enabled or not, circuit will always take this pins intoaccount
  *  Default address for parsic is 7 according circuit diagram
  */
  WriteRegister(0, 0xA0, 0xA8);        // all circuits start with 0 as address 
  WriteRegister(devAdd, IODIRA, 0x00); // set port A as output (relays -  green LED)
  WriteRegister(devAdd, IODIRB, 0xFF); // set port B as input (digital inputs - red LED)
}

void ParsicDigitalWrite(int devAdd, int pin, int aState){
  int currentStatus = ReadRegister(devAdd, OLATA);
  if(aState == 1)
    WriteRegister(devAdd, OLATA, currentStatus | (0x01 << pin));
  else
    WriteRegister(devAdd, OLATA, currentStatus & (~(0x01 << pin)));  
}

boolean ParsicDigitalReadOutput(int devAdd, int pin){
  if(ReadRegister(devAdd, OLATA) & (0x01 << pin))
    return true;
  else
    return false;
}

boolean ParsicDigitalRead(int devAdd, int pin){
  if(ReadRegister(devAdd, GPIOB) & (0x01 << pin))
    return true;
  else
    return false;
}

// Read operation
// Send the data in the next order:
// Single channel is always selected (not differential)
//  SPI out: 0 0 0 0 0 0 0 1   single channel channel channel x x  x  x     x  x  x  x  x  x  x  x
//  SPI in:  x x x x x x x x   x       x      x       x       x x B9 B8    B7 B6 B5 B4 B3 B2 B1 B0
int ParsicAnalogRead(int channel){ 
  digitalWrite(ChipSelect2, LOW);         // Active Low
  mySPI_Exchange(0x01);                   // Send leading zeroes
  int msbyte = mySPI_Exchange(0x80 | ((channel & 0x07) << 4)); // send channel and get most significative 2 bits
  msbyte &= 0x03;
  int lsbyte = mySPI_Exchange(0xFF);      // send dumy data and get least significative 8 bits
  digitalWrite(ChipSelect2, HIGH);        // Active Low
  return ((msbyte << 8) | lsbyte);
}

// Read operation
// Send the data in the next order:
//  opCode: 0 1 0 0 A2 A1 A0 R/W (0 = write, 1 = read)
//  8 bit Register Address
//  Last transaction will return register content
int ReadRegister(int devAdd, int regAdd){
  digitalWrite(ChipSelect1, LOW);       // Active Low
  mySPI_Exchange(0x40 | ((devAdd & 0x07) << 1) | 0x01);
  mySPI_Exchange(regAdd);
  int result = mySPI_Exchange(0x00);    // dumy write
  digitalWrite(ChipSelect1, HIGH);      // Active Low
  return result;
}

// Write operation
// Send the data in the next order:
//  opCode: 0 1 0 0 A2 A1 A0 R/W (0 = write, 1 = read)
//  8 bit Register Address
//  value to write into register
void WriteRegister(int devAdd, int regAdd, int value){
    digitalWrite(ChipSelect1, LOW);      // Active Low
    mySPI_Exchange(0x40 | ((devAdd & 0x07) << 1) | 0x00);
    mySPI_Exchange(regAdd);
    mySPI_Exchange(value);
    digitalWrite(ChipSelect1, HIGH);      // Active Low
}

// SPI_INIT
// Custom Initializes SP interface at fck/4
void mySPI_Init(void){
  // Set Outputs
  pinMode(myMOSI, OUTPUT);
  pinMode(mySCK, OUTPUT);
  pinMode(mySS, OUTPUT);

  // Set Inputs
  pinMode(myMISO, INPUT_PULLUP);
  
  // CS pin is not active 
  digitalWrite(mySS, HIGH);
  
  // Enable SPI, MSB first, Master Mode, set the clock rate fck/2
  SPCR = (1 << 6)|(0 << 5)|(1 << 4)|(0 << 3)|(0 << 2)|(0 << 1)|(0 << 0);
}

// Send and receive a byte from SPI
int mySPI_Exchange(int data){
  /* Start transmission */
  SPDR = data;
  /* Wait for transmission complete */
  while(!(SPSR & (1 << 7)));
  return SPDR;
}

