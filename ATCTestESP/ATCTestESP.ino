// ESP test on arduino
// Server on port 80

#define BAUD_RATE 115200

void setup() {
  Serial.begin(BAUD_RATE);
  Serial1.begin(BAUD_RATE);
  pinMode(13, OUTPUT);
  pinMode(19, INPUT_PULLUP);
  
  // Reset module
  Serial1.println("AT+RST");
  delay(3000);
  Serial1.println("AT+CIPMUX=1");
  delay(3000);
  Serial1.println("AT+CIPSERVER=1,80");
}

void loop() {
  int dataIn;
  
  if(Serial1.available()){
    dataIn = Serial1.read();
    Serial.print((char)dataIn);
  }

  if(Serial.available()){
    Serial1.print((char)Serial.read());
  }
  
  /*
  if(dataIn == 71){
    digitalWrite(13, HIGH);
    String page = "<TITLE>ATC Test Server MCP</TITLE><H3>Test for ATC!<H3>";
    Serial1.println("AT+CIPSEND=0,59");
    delay(300);
    Serial1.println(page);
    //Serial.println();
    //Serial.println("");
    delay(1000);
    Serial1.println("AT+CIPCLOSE=0");
    digitalWrite(13, LOW);
  }*/
}
