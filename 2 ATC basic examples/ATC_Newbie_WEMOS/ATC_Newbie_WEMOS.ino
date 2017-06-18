/*
 *  This sketch demonstrates how to set up an ATC compatiblewemos.
 *  For more info Look at https://play.google.com/store/apps/details?id=com.apps.emim.btrelaycontrol
 *  server_ip is the IP address of the ESP8266 module, will be
 *  printed to Serial when the module is connected.
 */

#include <ESP8266WiFi.h>

#define BAUD_RATE 115200
#define PORT      80

// Special commands
#define CMD_SPECIAL '<'
#define CMD_ALIVE   '['

const char* ssid = "your-ssid";
const char* password = "your-password";

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(PORT);
WiFiClient client;

void setup() {
  Serial.begin(BAUD_RATE);
  delay(10);

  // Initialize Output PORTS
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());
}

void loop() {
  delay(10);
  // ===========================================================
  // This is the point were you get data from the App
  // Check if a client has connected
  client = server.available();
  while (client) {
    int appData = -1;
    if (client.available()) {
      appData = client.read();
    }
    delay(10);

    // ===========================================================
    // This is the point were you get data from the App
    switch (appData) {
      case 'A': // Turn on pin 2
        digitalWrite(2, HIGH);
        client.println("<Imgs00:1"); // ON state 
        break;

      case 'a': // turn off pin 2
        digitalWrite(2, LOW);
        client.println("<Imgs00:0"); // OFF state 
        break;

      case 'B':
        digitalWrite(3, HIGH);
        client.println("<Imgs01:1"); // ON state 
        break;

      case 'b':
        digitalWrite(3, LOW);
        client.println("<Imgs01:0"); // OFF state 
        break;

      case 'C':
        digitalWrite(4, HIGH);
        client.println("<Imgs02:1"); // ON state 
        break;

      case 'c':
        digitalWrite(4, LOW);
        client.println("<Imgs02:0"); // OFF state 
        break;

      case 'D':
        digitalWrite(5, HIGH);
        client.println("<Imgs03:1"); // ON state 
        break;

      case 'd':
        digitalWrite(5, LOW);
        client.println("<Imgs03:0"); // OFF state 
        break;

      case 'E':
        digitalWrite(6, HIGH);
        client.println("<Imgs04:1"); // ON state 
        break;

      case 'e':
        digitalWrite(6, LOW);
        client.println("<Imgs04:0"); // OFF state 
        break;

      case CMD_SPECIAL:
        // I hope you did not enabled accelerometer or seek bars, so this will never be called
        break;

      case CMD_ALIVE:
        // Character '[' is received every 2.5s, use
        // this event to tell the android we are still here
        client.println("ATC + WEMOS");
        break;
    }
    // ==========================================================
  }
}
