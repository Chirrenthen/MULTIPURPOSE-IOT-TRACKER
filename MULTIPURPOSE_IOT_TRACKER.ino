#include "WiFi.h"
#define DEBUG true

//******************* Pin Configurations *******************//

#define A9G_PON     D10
#define A9G_LOWP    D2
#define SOS_Button  D3
#define SPY_Button  D4
#define Buzzer      D5

//******************* Necessary Variables *******************//
boolean stringComplete = false;
String inputString = "";
String fromGSM = "";
bool CALL_END = 1;
char* response = " ";
String res = "";
int c = 0;
String msg;
String custom_message;
int SOS_Time = 5; // Press the button 5 sec

//******************* SIM Parameters *******************//

String SOS_NUM = "+91xxxxxxxxxx";

//******************* SOS Button Press  *******************//
void A9G_Ready_msg();

void setup()
{
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, D0, D1);

  pinMode(A9G_PON, OUTPUT);//LOW LEVEL ACTIVE
  pinMode(A9G_LOWP, OUTPUT);//LOW LEVEL ACTIVE
  pinMode(Buzzer, OUTPUT);

  // Power on A9G module
  digitalWrite(A9G_LOWP, HIGH);
  digitalWrite(A9G_PON, HIGH);
  delay(1000);
  digitalWrite(A9G_PON, LOW);
  delay(10000);

  // Making Radio OFF for power saving
  WiFi.mode(WIFI_OFF);  // WiFi OFF
  btStop();   // Bluetooth OFF

  pinMode(SOS_Button, INPUT_PULLUP);
  pinMode(SPY_Button, INPUT_PULLUP);

  // Waiting for A9G to setup everything for 20 sec
  delay(20000);

  digitalWrite(A9G_LOWP, LOW); // Sleep Mode OFF

  // Just Checking
  msg = sendData("AT", 1000, DEBUG);
  while ( msg.indexOf("OK") == -1 ) {
    msg = sendData("AT", 1000, DEBUG);
    Serial.println("Trying");
  }

  // Turning ON GPS
  msg = sendData("AT+GPS=1", 2000, DEBUG);
  while ( msg.indexOf("OK") == -1 ) {
    msg = sendData("AT+GPS=1", 1000, DEBUG);
    Serial.println("Trying");
  }

  // GPS low power
  msg = sendData("AT+GPSLP = 2", 2000, DEBUG);
  while ( msg.indexOf("OK") == -1 ) {
    msg = sendData("AT+GPSLP = 2", 1000, DEBUG);
    Serial.println("Trying");
  }

  // Configuring Sleep Mode to 1
  msg = sendData("AT+SLEEP = 1", 2000, DEBUG);
  while ( msg.indexOf("OK") == -1 ) {
    msg = sendData("AT+SLEEP = 1", 1000, DEBUG);
    Serial.println("Trying");
  }

  // For SMS
  msg = sendData("AT+CMGF = 1", 2000, DEBUG);
  while ( msg.indexOf("OK") == -1 ) {
    msg = sendData("AT+CMGF = 1", 1000, DEBUG);
    Serial.println("Trying");
  }

  msg = sendData("AT+CSMP  = 17,167,0,0 ", 2000, DEBUG);
  while ( msg.indexOf("OK") == -1 ) {
    msg = sendData("AT+CSMP  = 17,167,0,0 ", 1000, DEBUG);
    Serial.println("Trying");
  }

  msg = sendData("AT+CPMS = \"SM\",\"ME\",\"SM\" ", 2000, DEBUG);
  while ( msg.indexOf("OK") == -1 ) {
    msg = sendData("AT+CPMS = \"SM\",\"ME\",\"SM\" ", 1000, DEBUG);
    Serial.println("Trying");
  }

  // For Speaker
  msg = sendData("AT+SNFS=2", 2000, DEBUG);
  while ( msg.indexOf("OK") == -1 ) {
    msg = sendData("AT+SNFS=2", 1000, DEBUG);
    Serial.println("Trying");
  }

  msg = sendData("AT+CLVL=8", 2000, DEBUG);
  while ( msg.indexOf("OK") == -1 ) {
    msg = sendData("AT+CLVL=8", 1000, DEBUG);
    Serial.println("Trying");
  }

  A9G_Ready_msg(); // Sending Ready Msg to SOS Number

  digitalWrite(A9G_LOWP, HIGH); // Sleep Mode ON
}

void loop()
{
  // Listen from GSM Module
  if (Serial1.available())
  {
    char inChar = Serial1.read();

    if (inChar == '\n') {

      // Check if location request
      if (fromGSM == "SEND LOCATION\r" || fromGSM == "send location\r" || fromGSM == "Send Location\r")
      {
        Get_gmap_link(0);  // Send Location without Call
        digitalWrite(A9G_LOWP, HIGH);// Sleep Mode ON
      }
      // Check if battery status requested
      else if (fromGSM == "BATTERY?\r" || fromGSM == "battery?\r" || fromGSM == "Battery?\r")
      {
        digitalWrite(A9G_LOWP, LOW); // Sleep Mode OFF
        Serial.println("---------Battery Status-------");
        msg = sendData("AT+CBC?", 2000, DEBUG);
        while ( msg.indexOf("OK") == -1 ) {
          msg = sendData("AT+CBC?", 1000, DEBUG);
          Serial.println("Trying");
        }

        msg = msg.substring(19, 24);
        response = &msg[0];

        Serial.print("Received Data - "); Serial.println(response);
        custom_message = response;
        Send_SMS(custom_message);
      }
      // For Auto Call Receive
      else if (fromGSM == "RING\r")
      {
        digitalWrite(A9G_LOWP, LOW); // Sleep Mode OFF
        Serial.println("---------ITS RINGING-------");
        tone(Buzzer, 2000, 1000);
        Serial1.println("ATA");
      }
      else if (fromGSM == "NO CARRIER\r")
      {
        Serial.println("---------CALL ENDS-------");
        CALL_END = 1;
        digitalWrite(A9G_LOWP, HIGH);// Sleep Mode ON
      }

      // Print the actual response
      Serial.println(fromGSM);
      // Clear the buffer
      fromGSM = "";
    }
    else
    {
      fromGSM += inChar;
    }
    delay(20);
  }

  // Read from Serial and send to A9G
  if (Serial.available()) {
    int inByte = Serial.read();
    Serial1.write(inByte);
  }

  // When SOS button is pressed
  if (digitalRead(SOS_Button) == LOW && CALL_END == 1)
  {
    Serial.print("Calling In.."); // Waiting for 5 sec
    for (c = 0; c < SOS_Time; c++)
    {
      Serial.println((SOS_Time - c));
      delay(1000);
      if (digitalRead(SOS_Button) == HIGH)
        break;
    }

    if (c == 5)
    {
      Get_gmap_link(1);  // Send Location with Call
    }

    // Write a full message to the GSM module
    if (stringComplete)
    {
      Serial1.print(inputString);
      inputString = "";
      stringComplete = false;
    }
  }

  // When Spy button is pressed
  if (digitalRead(SPY_Button) == LOW && CALL_END == 1)
  {
    Serial.print("Spy Calling In.."); // Waiting for 5 sec
    for (c = 0; c < SOS_Time; c++)
    {
      Serial.println((SOS_Time - c));
      delay(1000);
      if (digitalRead(SPY_Button) == HIGH)
        break;
    }

    if (c == 5)
    {
      Serial1.println("ATD" + SOS_NUM);
      Serial.println("Calling Now for Spy Mode");
      CALL_END = 0;
    }

    // Write a full message to the GSM module
    if (stringComplete)
    {
      Serial1.print(inputString);
      inputString = "";
      stringComplete = false;
    }
  }
}

// Getting Location and making Google Maps link of it. Also making call if needed
void Get_gmap_link(bool makeCall)
{
  digitalWrite(A9G_LOWP, LOW);
  Serial.println("---------Getting Location-------");

  // Turn ON GPS
  msg = sendData("AT+GPS=1", 2000, DEBUG);
  while ( msg.indexOf("OK") == -1 ) {
    msg = sendData("AT+GPS=1", 1000, DEBUG);
    Serial.println("Trying");
  }

  delay(5000);

  // Turn OFF GPS
  msg = sendData("AT+GPS=0", 2000, DEBUG);
  while ( msg.indexOf("OK") == -1 ) {
    msg = sendData("AT+GPS=0", 1000, DEBUG);
    Serial.println("Trying");
  }

  msg = sendData("AT+CGNSINF", 2000, DEBUG);
  while ( msg.indexOf("OK") == -1 ) {
    msg = sendData("AT+CGNSINF", 1000, DEBUG);
    Serial.println("Trying");
  }

  Serial.println(msg);

  String latitude = msg.substring(msg.indexOf("LAT:") + 4, msg.indexOf(",", msg.indexOf("LAT:")));
  String longitude = msg.substring(msg.indexOf("LON:") + 4, msg.indexOf(",", msg.indexOf("LON:")));
  String googleMapsURL = "https://www.google.com/maps?q=" + latitude + "," + longitude;

  Serial.println(googleMapsURL);
  if (makeCall)
  {
    Serial1.println("ATD" + SOS_NUM);
    delay(5000);
    Serial1.println("AT+CSCA=\"" + SOS_NUM + "\"");
    Serial1.println("AT+CMGS=\"" + SOS_NUM + "\"");
    Serial1.println(googleMapsURL);
    Serial1.println((char)26);
  }
  else
  {
    Serial1.println("AT+CMGS=\"" + SOS_NUM + "\"");
    Serial1.println(googleMapsURL);
    Serial1.println((char)26);
  }

  delay(10000);
}

// Function to send data
String sendData(String command, int timeout, boolean debug)
{
  String response = "";
  Serial1.println(command);
  long int time = millis();
  while ((time + timeout) > millis())
  {
    while (Serial1.available())
    {
      char c = Serial1.read();
      response += c;
    }
  }
  if (debug)
  {
    Serial.print("Response: ");
    Serial.println(response);
  }
  return response;
}

// A9G Initialization
void A9G_Ready_msg()
{
  Serial1.println("AT+CMGS=\"" + SOS_NUM + "\"");
  Serial1.println("SOS Tracker Initialized");
  Serial1.println((char)26);
  delay(1000);
}
