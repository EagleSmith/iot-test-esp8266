/* ====== ESP8266 TCP Demo ======
 * Receive & Respond via TCP
 * (Updated Dec 30, 2014)
 * ==========================
 *
 * Change SSID and PASS to match your WiFi settings.
 * The IP address is displayed to soft serial upon successful connection.
 *
 * Inspired by
 * Ray Wang @ Rayshobby LLC
 * http://rayshobby.net/?p=9734
 * http://raysfiles.com/arduino/ESP8266a_arduino.ino
 */

#include <SoftwareSerial.h>

#define BUFFER_SIZE 512
#define esp Serial
#define SSID  "selters5"      // change this to match your WiFi SSID
#define PASS  "!s3uuvH78"     // change this to match your WiFi password
#define PORT  "8001"          // using port 8001 by default

char buffer[BUFFER_SIZE];

// Software Serial for debug
SoftwareSerial dbg(10,11);  // use pins 10, 11 for software serial 

// By default we are looking for OK\r\n
char OKrn[] = "OK\r\n";

// LED Pin
const int LED = 13;


byte wait_for_esp_response(int timeout, char* term=OKrn) {
  unsigned long t=millis();
  bool found=false;
  int i=0;
  int len=strlen(term);
  // wait for at most timeout milliseconds
  // or if OK\r\n is found
  while(millis()<t+timeout) {
    if(esp.available()) {
      buffer[i++]=esp.read();
      if(i>=len) {
        if(strncmp(buffer+i-len, term, len)==0) {
          found=true;
          break;
        }
      }
    }
  }
  buffer[i]=0;
  dbg.print(buffer);
  return found;
}

void setup() {

  // assume esp8266 operates at 57600 baud rate
  // change if necessary to match your modules' baud rate
  esp.begin(57600);
  
  dbg.begin(9600);
  dbg.println("begin.");
  
  // Setup LED PIN as output
  pinMode(LED, OUTPUT);
  
  // blink test
  digitalWrite(LED, HIGH); // LED on
  delay(500); // 500 ms delay
  digitalWrite(LED, LOW); // LED off
    
  setupWiFi();

  // print device IP address
  dbg.print("device ip addr:");
  esp.println("AT+CIFSR");
  wait_for_esp_response(1000);
}

bool read_till_eol() {
  char incomingByte = 0;   // for incoming serial data
  // +IPD,0,7:Hallo
  static int i=0;
  if(esp.available()>0) {
    incomingByte = esp.read();
      buffer[i++]=incomingByte;
      if(i==BUFFER_SIZE)  i=0;
      if(i>1 && buffer[i-2]==13 && buffer[i-1]==10) {
        buffer[i]=0;
        i=0;
        dbg.print(buffer);
        return true;
    }
  }
  return false;
}

void loop() {
  int ch_id, packet_len;
  char *pb;  
  if(read_till_eol()) {
    dbg.println("----------------------------------");
    if(strncmp(buffer, "+IPD,", 5)==0) {
      // request: +IPD,ch,len:data
      sscanf(buffer+5, "%d,%d", &ch_id, &packet_len);
      if (packet_len > 0) {
        // read serial until packet_len character received
        // start from :
        pb = buffer+5;
        while(*pb!=':') pb++;
        pb++;
        dbg.print("Message received: ");
        dbg.println(pb);
        send_Response(ch_id, "ACK");
        if (strncmp(pb, "LEDON", 5) == 0) {
          digitalWrite(LED, HIGH); // LED on
        }
        if (strncmp(pb, "LEDOFF", 6) == 0){
          digitalWrite(LED, LOW); // LED on
        }
      }
    }
  }
}


void send_Response(int ch_id, String content) {
  esp.print("AT+CIPSEND=");
  esp.print(ch_id);
  esp.print(",");
  esp.println(content.length());
  if(wait_for_esp_response(2000, "> ")) {
    esp.print(content);
  } else {
    esp.print("AT+CIPCLOSE=");
    esp.println(ch_id);
  }
}


void setupWiFi() {
  // try empty AT command
  esp.println("AT");
  wait_for_esp_response(1000);

  // set mode 1 (client)
  esp.println("AT+CWMODE=1");
  wait_for_esp_response(1000);  

  // reset WiFi module
  esp.print("AT+RST\r\n");
  wait_for_esp_response(1500);
 
  // join AP
  esp.print("AT+CWJAP=\"");
  esp.print(SSID);
  esp.print("\",\"");
  esp.print(PASS);
  esp.println("\"");
  // this may take a while, so wait for 5 seconds
  wait_for_esp_response(5000);
  
  esp.println("AT+CIPSTO=30");  
  wait_for_esp_response(1000);

  // start server
  esp.println("AT+CIPMUX=1");
  wait_for_esp_response(1000);
  
  esp.print("AT+CIPSERVER=1,"); // turn on TCP service
  esp.println(PORT);
  wait_for_esp_response(1000);
  
    
}
