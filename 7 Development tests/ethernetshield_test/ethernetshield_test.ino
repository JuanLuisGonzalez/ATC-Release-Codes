/*
 * Test for ethetnet shield
 * This program will repeat the information received into the serial monitor,
 * any info taped into the serial monitor will be sent to the ethetnet shield.
 */

#include <SPI.h>
#include <Ethernet.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(172, 21, 117, 60);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);
EthernetClient client;

void setup() {
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.begin(115200);
}

void loop() {
  int appData;

  // ===========================================================
  // This is the point were you get data from the App
  client = server.available();
  if (client) {
    appData = client.read();
    Serial.print((char)appData);
  }

  if (Serial.available()) {
    appData = Serial.read();
    server.print((char)appData);
  }
}

