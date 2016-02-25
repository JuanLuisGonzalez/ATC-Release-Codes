/*Arduino Total Control ESP Firmware
 REMEMBER TO DISCONNECT BLUETOOT TX PIN WHEN UPLOADING SKETCH
 REAL basic functions for controlling stuff via ATC app and wifi module ESP.
 Please use the "example" Codes to get all the awesome features the app has!!!
 If using this sketch, DO NOT ENABLE accelerometer data in the app or seek bars or funny stuff will happen :)
 
 * ESP test on arduino pins serial port, pins 0 and 1
 * works well with 3.6V on vcc, themodule seems to overheat by itself
    How to set a Server on port 80:
    Main commands
    AT+RST
    AT+GMR
    AT+CWMODE=3
    AT+CWJAP="Your network","your password"
    AT+CIFSR
    AT+CIPMUX=1
    AT+CIPSERVER=1,80
 
    How to receive data: 
    +IPD,0,2:[ (for this exaple 0 is the channel, and 2 is the data lenght)
    Rare error down, blue module uses 1 as channel and black one uses 0
 
    How to send data: 
    AT+CIPSEND=0,3 (0: channel, 3 data size)
 
 Author: Juan Luis Gonzalez Bello 
 Date: Feb 2016   
 Get the app: https://play.google.com/store/apps/details?id=com.apps.emim.btrelaycontrol 
 **/

// Baud rate for ESP module
#define BAUD_RATE 115200
String sPort = "80";

// Special commands
#define CMD_SPECIAL '<'
#define CMD_ALIVE   '['

void setup() {   
  Serial.begin(BAUD_RATE);
  myESP_Init();

  // Initialize Output PORTS 
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);

  // Greet arduino total control on top of the app 
  myESP_Println("Thanks for your support!", 0); 
} 

void loop() { 
  int appData = -1; 
  String appMessage;
  
  // =========================================================== 
  // This is the point were you get data from the App 
  if(Serial1.available()){
    appMessage = myESP_Read(0);    // Read channel 0
    appData = appMessage.charAt(0);
  }
  switch(appData){ 
  case 'A': // Turn on pin 2
    digitalWrite(2, HIGH);
    break; 
  
  case 'a': // turn off pin 2
    digitalWrite(2, LOW);
    break; 
  
  case 'B': 
    digitalWrite(3, HIGH);
    break; 
  
  case 'b': 
    digitalWrite(3, LOW);
    break; 
  
  case 'C': 
    digitalWrite(4, HIGH);
    break; 
  
  case 'c': 
    digitalWrite(4, LOW);
    break; 
  
  case 'D': 
    digitalWrite(5, HIGH);
    break; 
  
  case 'd': 
    digitalWrite(5, LOW);
    break; 
  
  case 'E': 
    digitalWrite(6, HIGH);
    break; 
  
  case 'e': 
    digitalWrite(6, LOW);
    break; 
    
  case CMD_SPECIAL: 
    // I hope you did not enabled accelerometer or seek bars, so this will never be called
    break; 

  case CMD_ALIVE: 
    // Character '[' is received every 2.5s, use 
    // this event to tell the android we are still here
    myESP_Println("Please move to example code", 0); 
    break; 
  }
  // ========================================================== 
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



