#include <Servo.h>
#include "font.h"

/*
Hardware connections

------8x8 LED
VCC from LaunchPad
GND from LaunchPad
LATCH 6
CLOCK 8
DATA 13

------CC3000
VCC from power supply
Default BP pin assignment is OK

------Servo
Datapin @ pin#40
VCC from from CC3000 power supply
GND from LP

*/

/*
 Reading api.openweathermap.org
 
 This sketch connects to api.openweathermap.org using LaunchPad + CC3000 BoosterPack. 
 It parses the XML returned, and looks for temp, humidity & time of last update.

 
 This example uses the DHCP routines in the WiFi CC3000 library which is part of the 
 Energia core from version 10
 
 This example uses the String library, which is part of the Energia core.

 This example uses the Serial monitor to display de results of the call to openweathermap.
 
 Circuit:
 * CC3000 WiFi BoosterPack
 * TI LaunchPad
 
 created in 8 Aug 2012
 by David Alcubierre
 
 Ethernet part based on 'Twitter Client' by Tom Igoe (public domain, http://arduino.cc/en/Tutorial/TwitterClient).
 
 This code is in the public domain.
 
 */
#include "SPI.h"
#include "WiFi.h"

//////////////////////////////////////////////////////////
//THINGSPEAK TWEET STUFF
// ThingSpeak Settings
//char thingSpeakAddress[] = "api.thingspeak.com";
IPAddress thingSpeakIp(184,106,153,149);
// ThingSpeak API key
String thingTweetAPIKey = "S3URIFVMENLZ05EUJ";

// Variable Setup
int failedCounter = 0;
boolean sentTweet = false;

int milestoneSize = 10;
int milestone = milestoneSize;
//////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////
// The 8x8 led matrix
#define LATCH 6
#define CLOCK 8
#define DATA 13
#define DELAYTIME 1 // microseconds to wait after setting pin

#define TILES 7 // number of tiles
#define SPEED 3 // smaller number is faster

// The string to display:
char str[50] = "";


// Image buffer, you can modify the bits and they will be displayed on the 8x8 matrix with the sendImage() function
unsigned char image[8*TILES];

unsigned long last8x8Update = 0;
unsigned long updateInterval = 250;
/////////////////////////////////////////////////////////


char ssid[] = "ecsapps2";     //  your network SSID (name) 
char pass[] = "ecsapps2";     //  your network SSID (name) 
String FACEBOOKID = "811518302198156";


int motorDir = 90;
// initialize the library instance:
WiFiServer server(80);
WiFiClient client;

const unsigned long requestInterval = 15000;  // delay between requests

IPAddress hostIp;
uint8_t ret;

boolean requested;                   // whether you've made a request since connecting
unsigned long lastAttemptTime = 0;            // last time you connected to the server, in milliseconds

String currentLine= "";        // string to hold the text from server
String tempString = "";         // string to hold temp
unsigned int prevFacebookLikes = 0; 
unsigned int facebookLikes = 0;
unsigned int numLikes = 2460;
unsigned int startLikes = 0;
unsigned int multiplier = 0;
boolean readingTemp = false;    // if you're currently reading the temp

Servo myservo;  // create servo object to control a servo 

int temp = 0;

boolean firstRun = true;

void setup() {
  
  ///////////////////////////////////////////////////////////////////
  memset(image,0xFF,sizeof(image));
  pinMode(CLOCK, OUTPUT);
  pinMode(DATA, OUTPUT);
  pinMode(LATCH, OUTPUT);
  analogReference(INTERNAL1V5);
  //////////////////////////////////////////////////////////////////
  
  // reserve space for the strings:
  currentLine.reserve(100);
  tempString.reserve(10);
  Serial.begin(115200);    
  myservo.attach(40);  // attaches the servo on pin P2.5 to the servo object 
  delay(50);
  myservo.write(96);              // tell servo to go to position in variable 'pos' 

  
  WiFi.setCSpin(18);  //P2_2 on F5529LP
  WiFi.setENpin(2);  //P6_5 on F5529LP
  WiFi.setIRQpin(19); //P2_0 on F5529LP
  
  delay(20);
  // Connect to an AP with WPA/WPA2 security
  Serial.println("Connecting to WiFi....");  
 WiFi.begin(ssid, pass);
  
  //WiFi.begin(ssid);
  server.begin();
  Serial.println("Connect success!");
  Serial.println("Waiting for DHCP address");
  // Wait for DHCP address
  delay(5000);
  // Print WiFi status and DHCP address
  // To print the status and DHCP info again, type "i" in the Serial monitor and press send.
  printWifiData();

  // connect to api.openweathermap.org:
  connectToServer();

}

void loop()
{
  
  //int a =0, m=0, j=0, now=0, avg=0, prev=0, val=0;
  if (client.connected()) {
    //Serial.println("Client connected!");
    if (client.available()) {
      //Serial.println("Client available!");
      // read incoming bytes:
      char inChar = client.read();
      //Serial.print(inChar);
      // add incoming byte to end of line:
      currentLine += inChar; 
      // if you get a newline, clear the line:
      //Serial.println("trying to parse...");
      if (inChar == '\n') {
        //Serial.print("clientReadLine = ");
        //Serial.println(currentLine);
        currentLine = "";
      } 
      
      // LOOKING FOR FACEBOOK DATA
      // if the current line ends with <temperature value=, it will
      // be followed by the temp:
      if ( currentLine.endsWith("<like_count>")) {
        // temperatue data is beginning. Clear the temp string:
        readingTemp = true; 
        tempString = "";
      }      
      
      // PULLING FACEBOOK DATA
      // if you're currently reading the bytes for facebook likes,
      // add them to the temporary string:
      if (readingTemp) {
        if (inChar != '<') {
          tempString += inChar;
        } 
        else {
          // if you got a termination character,
          // you've reached the end of the facebook like data:
          readingTemp = false;
          facebookLikes = getInt(tempString);
          tempString = tempString.substring(1, tempString.length());
          Serial.print("Facebook Like Count: ");
          Serial.println(facebookLikes); // print count to screen
          String temporary = "  ";
          temporary.concat(tempString);
          temporary.concat(" Likes!! (+");
          temporary.concat(facebookLikes-numLikes);
          temporary.concat(")         ");
          temporary.toCharArray(str, temporary.length()+1);
          Serial.print("str = ");
          Serial.println(str);          
          showText(str);
          
          if(firstRun){//first time checking num of facebook likes, so save starting like count
            numLikes = facebookLikes;
            startLikes = facebookLikes; // save num of likes at start
            firstRun = false;
          }
          if(facebookLikes > numLikes){
             multiplier = facebookLikes - numLikes;
             
             myservo.write(150);              // tell servo to go to position in variable 'pos' 
             delay(100*multiplier);
             myservo.write(96);              // tell servo to go to position in variable 'pos' 
             numLikes = facebookLikes;
          }
          
          //numLikes = ;
          
          
           
          // close the connection to the server:
          client.stop(); 
          Serial.println("Disconnected from client.\n");
          connectToServer();
        }
      }      
    }   

    //Serial.print("Facebook likes so far... = ");
    //Serial.println(facebookLikes - startLikes);
    if((facebookLikes - startLikes) > milestone){  //send tweet!
        String tweetString = "MILESTONE! Facebook Like count = ";
        Serial.print("MILESTONE! We have past ");
        Serial.print(milestone);
        Serial.println(" facebook likes!");
        tweetString.concat(milestone);
        //updateThingTweet(tweetString);
        milestone= milestone+milestoneSize;
    }
  }
  //else if (millis() - lastAttemptTime > requestInterval) {
    // if you're not connected, and request interval time have passed since
    // your last connection, then attempt to connect again to get new data:
    //connectToServer();
  //}
  //if(millis()-last8x8Update > updateInterval){
  //  showText(str);
  //  last8x8Update = millis();
  //}

}

void updateThingTweet(String tsData)
{
  Serial.println("Updating ThingSpeak");
  
  if (client.connect(thingSpeakIp, 80)) {
    Serial.println("Connected to ThingSpeak!");
    Serial.println();
    tsData = "api_key="+thingTweetAPIKey+"&status="+tsData;
    
    client.print("POST /apps/thingtweet/1/statuses/update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(tsData.length());
    client.print("\n\n");
    client.print(tsData);
    
    Serial.println("Twitter Updated!");
    delay(500);
    client.stop();
    connectToServer();  // reconnect to facebook for latest like count
    failedCounter = 0;
  } else {  
    failedCounter++;
    client.stop();
 
    Serial.println("Connection to ThingSpeak Failed ("+String(failedCounter, DEC)+")");   
    Serial.println();
  }
}


void connectToServer() {
  // attempt to connect, and wait a millisecond:
  Serial.println("connecting to server...");
  String content = "";
  if (client.connect(hostIp, 80)) {
    Serial.println("Connected! Making HTTP request...");
    // make HTTP GET request to Facebook:
    
    client.println("GET /restserver.php?format=xml&method=fql.multiquery&pretty=0&queries=%7B$22photo_info$22%3A%22select+like_info+from+photo+where+object_id+IN+%28811518302198156%29%22%7D HTTP/1.1");
    ///restserver.php?format=xml&method=fql.multiquery&pretty=0&queries=%7B$22photo_info$22%3A%22select+like_info+from+photo+where+object_id+IN+%28811518302198156%29%22%7D
    //GET FACEBOOK PAGE LIKES
    //GET /restserver.php?format=html&method=fql.multiquery&pretty=0&queries=%7B%22page_info%22%3A%22select%20fan_count%20from%20page%20where%20page_id%20IN%20%2827479046178%29%22%7D
    
    //GET FACEBOOK PHOTO LIKES
    //GET /restserver.php?format=xml&method=fql.multiquery&pretty=0&queries=%7B$22photo_info$22%3A%22select+like_info+from+photo+where+object_id+IN+%2810151842856142998%29%22%7D
    
//    client.print("GET /restserver.php?format=");
//    Serial.print("GET /restserver.php?format=");
//    client.print("xml&method=fql.multiquery&pretty=0");
//    Serial.print("xml&method=fql.multiquery&pretty=0");
//    client.print("&queries=%7B%22page_info%22%3A%22select%20");
//    Serial.print("&queries=%7B%22page_info%22%3A%22select%20");
//    client.print("fan_count%20from%20page%20where%20page_id");
//    Serial.print("fan_count%20from%20page%20where%20page_id");
//    client.print("%20IN%20%2827479046178%29%22%7D");
//    Serial.print("%20IN%20%2827479046178%29%22%7D");
    
    // declare correct server
    //Serial.println("got past GET");
    //client.println("HOST: tinyurl.com/kv3sut7\n");
    client.println("HOST: graph.facebook.com");
    //Serial.println("got past HOST");
    
    client.println();
  }
  // note the time of this connect attempt:
  lastAttemptTime = millis();
}

void printHex(int num, int precision) {
      char tmp[16];
      char format[128];

      sprintf(format, "%%.%dX", precision);

      sprintf(tmp, format, num);
      Serial.print(tmp);
}

void printWifiData() {
  // print your WiFi shield's IP address:
  Serial.println();
  Serial.println("IP Address Information:");  
  IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
  Serial.println(ip);
  
  // print your MAC address:
  byte mac[6];  
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printHex(mac[5], 2);
  Serial.print(":");
  printHex(mac[4], 2);
  Serial.print(":");
  printHex(mac[3], 2);
  Serial.print(":");
  printHex(mac[2], 2);
  Serial.print(":");
  printHex(mac[1], 2);
  Serial.print(":");
  printHex(mac[0], 2);
  Serial.println();
  
  uint8_t *ver = WiFi.firmwareVersion();
  Serial.print("Version: ");
  Serial.print(ver[0]);
  Serial.print(".");
  Serial.println(ver[1]);  
  // print your subnet mask:
  IPAddress subnet = WiFi.subnetMask();
  Serial.print("NetMask: ");
  Serial.println(subnet);

  // print your gateway address:
  IPAddress gateway = WiFi.gatewayIP();
  Serial.print("Gateway: ");
  Serial.println(gateway);
  
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  
  ret = WiFi.hostByName("graph.facebook.com", hostIp);
    //ret = WiFi.hostByName("tinyurl.com/kv3sut7", hostIp);
    
  Serial.print("ret: ");
  Serial.println(ret);
  
  Serial.print("Host IP: ");
  Serial.println(hostIp);
}

int getInt(String input){
//  int i = 2;
  
//  while(input[i] != '"'){
//    //Serial.print(i); Serial.print("=");
//    //Serial.println(input[i]);
//    i++;
//  }
  //Serial.print("input string=======================  ");Serial.println(input);
  input = input.substring(1,input.length());
  //Serial.print("input string=======================  ");Serial.println(input);
  char carray[20];
  //Serial.println(input);
  input.toCharArray(carray, sizeof(carray));
  //Serial.print("carray=======================  ");Serial.println(carray);
  temp = atoi(carray);
  //Serial.print("temp=======================  ");Serial.println(temp);
  return temp;
}


// Shift alls bytes in the image right and add a byte in the display buffer
void shiftRight(unsigned char c)
{
  for(unsigned short m=((8*TILES)-1); m>1; m--)
    image[m] = image[m-1];
  image[0] = c;
}

// Shift all bytes in the image buffer left and add byte in the display buffer
void shiftLeft(unsigned char c)
{
  for(unsigned short m=0; m<((8*TILES)-1); m++)
    image[m] = image[m+1];
  image[(8*TILES)-1] = c;
}


// send bits to tiles, each 8x8 tile receives 16 bits to LED tile (bits 0..7 are column, 8..15 are row) 
void sendData(unsigned short data)
{
  for(unsigned short m=0; m<16; m++)
  {
    if ( data & ((unsigned short)1<<m)  )
      digitalWrite(DATA, 1);
    else
      digitalWrite(DATA, 0);
    digitalWrite(CLOCK,1);
    digitalWrite(CLOCK,0);
  }
}

// scan an image on the LED tile
// 8x8 bitmap img is the input
void sendImage(unsigned char *img)
{
  unsigned short data;
  for(unsigned int m=0;m<8;m++)
  {
    for(unsigned int n=0;n<TILES;n++)
    {
      data = (1<<8)<<m;
      data |= *(img+m+(8*n));
      sendData(data);
    }
     // Latch data 

  digitalWrite(LATCH,1);
  digitalWrite(LATCH,0);
  }

 
}

// display a text on the 8x8 display
void showText(char *txt)
{
  int a =0, m=0, j=0;
  while(1)
  {
    sendImage(image);
    if ( a++ > SPEED ) // shift in new line
    {  
      a=0;
      if ( j < 5 ) // copy character
        shiftLeft( *(fontPtr(txt[m]) + (unsigned int)j) );
      else
      {
        shiftLeft(0); // blank line
        sendImage(image);
      // tone(500);
      }
      if ( ++j > 5 ) // next character
      {
        j = 0;
        if ( ++m >= strlen(txt) ) 
          return; 
      } 
    }
  } 
}
