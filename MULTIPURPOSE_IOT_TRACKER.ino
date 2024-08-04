#include "SoftwareSerial.h"

#define SOS_Button 3
#define Call_Button 4
#define Buzzer 5

SoftwareSerial A9G_Serial(0, 1); // RX, TX

String SOS_NUM = "+91xxxxxxxxxx";  // SOS number
String CALL_NUM = "+91yyyyyyyyyy"; // Call number
bool CALL_END = true;

void setup() {
  Serial.begin(115200);
  A9G_Serial.begin(115200);

  pinMode(SOS_Button, INPUT_PULLUP);
  pinMode(Call_Button, INPUT_PULLUP);
  pinMode(Buzzer, OUTPUT);

  setupA9G();

  delay(20000); // Wait for A9G setup
}

void loop() {
  if (digitalRead(SOS_Button) == LOW) {
    delay(5000); // Debounce for 5 seconds
    if (digitalRead(SOS_Button) == LOW) {
      sendLocation(SOS_NUM);
      makeCall(SOS_NUM);
    }
  }

  if (digitalRead(Call_Button) == LOW) {
    delay(5000); // Debounce for 5 seconds
    if (digitalRead(Call_Button) == LOW) {
      makeCall(CALL_NUM);
    }
  }

  if (A9G_Serial.available()) {
    String response = A9G_Serial.readString();
    handleA9GResponse(response);
  }

  // Check if the call has ended
  if (!CALL_END) {
    if (A9G_Serial.available()) {
      String response = A9G_Serial.readString();
      if (response.indexOf("NO CARRIER") != -1) {
        Serial.println("---------CALL ENDS-------");
        CALL_END = true;
      }
    }
  }
}

void setupA9G() {
  sendCommand("AT");
  sendCommand("AT+GPS=1");
  sendCommand("AT+GPSLP=2");
  sendCommand("AT+SLEEP=1");
  sendCommand("AT+CMGF=1");
  sendCommand("AT+CSMP=17,167,0,0");
  sendCommand("AT+CPMS=\"SM\",\"ME\",\"SM\"");
  sendCommand("AT+SNFS=2"); // Enable speaker functionality
  sendCommand("AT+CLVL=8"); // Set speaker volume level to 8 (adjust as needed)
  sendCommand("AT+CLAC"); // To list available AT commands
}

void sendLocation(String number) {
  sendCommand("AT+LOCATION=2");
  String response = readResponse();
  if (response.indexOf("GPS NOT") != -1) {
    sendSMS(number, "Unable to fetch location. Please try again");
  } else {
    int commaIndex = response.indexOf(',');
    String lat = response.substring(17, commaIndex);
    String lon = response.substring(commaIndex + 1, 38);
    String gmapLink = "http://maps.google.com/maps?q=" + lat + "," + lon;
    sendSMS(number, "I'm here: " + gmapLink);
  }
}

void makeCall(String number) {
  sendCommand("ATD" + number + ";");
  CALL_END = false;
}

void sendSMS(String number, String message) {
  sendCommand("AT+CMGF=1");
  sendCommand("AT+CMGS=\"" + number + "\"");
  A9G_Serial.print(message);
  A9G_Serial.write(26); // Send SMS
  delay(1000);
  sendCommand("AT+CMGD=1,4"); // Delete all SMS from memory
}

void handleA9GResponse(String response) {
  if (response.indexOf("RING") != -1) {
    digitalWrite(Buzzer, HIGH);
    delay(3000); // Buzzer on for 3 seconds
    digitalWrite(Buzzer, LOW);
    sendCommand("ATA"); // Answer the call
    // Audio will be routed to the speaker connected to A9G
  } else if (response.indexOf("NO CARRIER") != -1) {
    Serial.println("---------CALL ENDS-------");
    CALL_END = true;
  }
}

void sendCommand(String command) {
  A9G_Serial.println(command);
  delay(1000);
}

String readResponse() {
  String response = "";
  long int time = millis();
  while ((time + 10000) > millis()) {
    while (A9G_Serial.available()) {
      char c = A9G_Serial.read();
      response += c;
    }
  }
  return response;
}
