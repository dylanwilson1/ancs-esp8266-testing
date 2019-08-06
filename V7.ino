#include <SoftwareSerial.h>

SoftwareSerial mySerial(5, 4); //rx tx   D1  D2
const int ledpin = 14;  //D5
char NotifUID[5];
char onUID[5];  //stores the uid of the notification that turned the light on
bool state = 0;

void setup() {
  pinMode(ledpin, OUTPUT);

  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  Serial.setTimeout(200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Goodnight moon!");

  // set the data rate for the SoftwareSerial port
  mySerial.begin(9600);
  mySerial.println("Hello, world?");
}

void printType(char type) //print what the notification is
{
  switch (type)
  {
    case '0':
      Serial.print("Notif Added\n");
      break;
    case '1':
      Serial.print("Notif Modded\n");
      break;
    case '2':
      Serial.print("Notif Removed\n");
      break;
  }
}

void printID(char ID)
{
  switch (ID)
  {
    case '0':
      Serial.print("Other\n");
      break;
    case '1':
      Serial.print("Incoming Call\n");
      break;
    case '2':
      Serial.print("Missed Call\n");
      break;
    case '3':
      Serial.print("Voicemail\n");
      break;
    case '4':
      Serial.print("Social\n");
      break;
    case '5':
      Serial.print("Schedule\n");
      break;
    case '6':
      Serial.print("Email\n");
      break;
    case '7':
      Serial.print("News\n");
      break;
    case '8':
      Serial.print("Health\n");
      break;
    case '9':
      Serial.print("Buss\n");
      break;
    case 'A':
      Serial.print("Location\n");
      break;
    case 'B':
      Serial.print("Entertainment\n");
      break;
  }
}

void reqInfo(char UID[]) {
  mySerial.write("AT+ANCS");
  for (int i = 0; i < 4; i++) {
    mySerial.write(UID[i]);
  }
  mySerial.write("116");   //1 means header, 20 matches the max bytes of the MTU
  Serial.println("Request sent to phone");
}


void loop() {
  if (mySerial.available() >= 7) { //if 7 bytes are available
    char ANCS8Buffer[8];
    mySerial.readBytes(ANCS8Buffer, 7);   //read the first 7 bytes   EITHER OK+ANCS8 OR OK+ANCSW  OR OK+1234   WONT READ 8, W
    Serial.print("ANCS8Buffer     ");
    //Serial.println(ANCS8Buffer); //print what comes in

    for (int i = 0; i < 7; i++) {
      Serial.print(ANCS8Buffer[i]);  //print the first 7 characters
    }


    if (ANCS8Buffer[3] == 'A' and ANCS8Buffer[4] == 'N'
        and ANCS8Buffer[5] == 'C' and ANCS8Buffer[6] == 'S') { //if its not a UID ,either OK+ANCSW or OK+ANCS8 or OK+ANCS:FF

      ANCS8Buffer[7] = mySerial.read(); //read the 8th character as it is either  OK+ANCS8 or OK+ANCSW
      Serial.println(ANCS8Buffer[7]);  //print the 8th character if there is one



      if (ANCS8Buffer[7] == '8') { //if it is Notification Source     OK+ANCS8
        Serial.println("Notification Source recieved");

        char ANCS16Buffer[8];
        mySerial.readBytes(ANCS16Buffer, 8); //read the next 8 bytes to the 16buffer  820020004
        Serial.print("ANCS16Buffer     ");
        //Serial.println(ANCS16Buffer);

        for (int i = 0; i < 8; i++) {   //print the next 8 characters
          Serial.print(ANCS16Buffer[i]);
        }
        Serial.print('\n');


        NotifUID[0] = ANCS16Buffer[4]; //get notifUID
        NotifUID[1] = ANCS16Buffer[5];
        NotifUID[2] = ANCS16Buffer[6];
        NotifUID[3] = ANCS16Buffer[7];
        Serial.print("UID     ");
        Serial.println(NotifUID);


        printType(ANCS16Buffer[0]); //print if it is added, modded or removed
        printID(ANCS16Buffer[1]); //print what category the notification is eg social, health


        if (ANCS16Buffer[0] == '0') { //if it is an added notification
          reqInfo(NotifUID);  //send a request for the header to the phone
        }
        Serial.print("onUID        ");
        Serial.println(onUID);


        if (ANCS16Buffer[0] == '2') { //if it is a removed notification
          Serial.println("checked notification, it is removed");
          if (strncmp(onUID, NotifUID, 4) == 0) {
            Serial.println("Turning the light off");
            digitalWrite(ledpin, LOW); //turn the light off
            state = 0;
          }
        }



      }
      else if (ANCS8Buffer[7] == 'W') {    //if it is a Get Notification Attributes Response  OK+ANCSW
        Serial.println("Notification Attributes Response recieved");

        char ANCSResponseBuffer[10];
        while (!mySerial.available()) {}
        mySerial.readBytes(ANCSResponseBuffer, 10);  //read the next 10 bytes  OK+ANCS:14

        //14 being the max length returned in hex (20)

        Serial.print("ANCSResponseBuffer      ");
        for (int i = 0; i < 10; i++) {
          Serial.print(ANCSResponseBuffer[i]);
        }
        Serial.print('\n');



        char AttributeLengthChar[2];    //make  AttributeLength how many characters need to be read next  two digit hex number
        AttributeLengthChar[0] = ANCSResponseBuffer[8];
        AttributeLengthChar[1] = ANCSResponseBuffer[9];
        Serial.print("Attributelengthchar     ");
        Serial.println(AttributeLengthChar);


        //make two digit hex number into an integer
        char * pEnd;
        long int AttributeLength = strtol(AttributeLengthChar, &pEnd, 16);

        Serial.print("Attribute Length     ");
        Serial.println(AttributeLength);



        char ANCSWBuffer[AttributeLength];  //creates a buffer to accept the information   the max is 0x14 in this situation(20)due to MTU limit
        mySerial.readBytes(ANCSWBuffer, AttributeLength); //reads the amount of bytes stated by OK+ANCS
        Serial.print("ANCSWBuffer     ");
        for (int i = 0; i < AttributeLength; i++) {
          Serial.write(ANCSWBuffer[i]);
        }
        Serial.print('\n');

        
        Serial.println("Checking if string is present");

        //----------------------------------------------------
        /// add your own string here to find in the header
        //----------------------------------------------------

        char stringToFind[] = "hello";



        for (int i = 0; i < AttributeLength - len(stringToFind); i++) { //checks every position in the buffer that the string could appear
          for (int j = 0; j < len; j++) {
            if (ANCSWBuffer[i + j] == stringToFind[j]) {
              if (state == 0) {
                Serial.println("Found, Turning the light on");
                strncpy(onUID, NotifUID, 4); //copy NotifUID to onUID
                digitalWrite(ledpin, HIGH);
                state = 1;
                break;
              }
            }
          }
          if (state == 1) {
            break;
          }
        }
      }

      else if (ANCS8Buffer[7] == ':') {  //OK+ANCS:FF     if the Notification attributes response is long
        Serial.println("Extended Notification Attributes Recieved");


        char ANCSWExtBuffer[20];
        mySerial.readBytes(ANCSWExtBuffer, 20);
        Serial.println(ANCSWExtBuffer);
      }
    }
    else {
      //if it is OK+1234 (UID) do nothing
      Serial.print('\n'); //new line for formatting purposes
    }
  }
  if (Serial.available()) {
    mySerial.write(Serial.read());
  }
  delay(1000);
  Serial.println("waiting");
}
