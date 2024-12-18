#include <Wire.h>
#include <SPI.h>

#include <SD.h>
#include "RTClib.h"
#include <MFRC522.h>

#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

//Display Parameters
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32;

SSD1306AsciiWire display;

//RFID Reader Parameters
#define SS_PIN 8
#define RST_PIN 5

#define SD_CHIPSELECT 10

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key;

// Init array that will store new NUID
byte nuidPICC[4];

String tagId;
String currentTimestamp;

//RTC module
RTC_DS3231 rtc;
const char PROGMEM daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

File dataFile;


void setup(void)
{
  Serial.begin(9600);

  //Init RTC
  Wire.begin();
  Wire.setClock(400000L);
  display.begin(&Adafruit128x64, SCREEN_ADDRESS);
  display.setFont(Adafruit5x7);
  display.set2X();

  // Clear the buffer.
  display.clear();
  delay(1000);

  //display.setCursor(20, 0); //oled display
  //display.setTextSize(2);
  //display.setTextColor(WHITE);
  display.setCursor(20, 0);
  display.println("TAG ID");
  //display.display();

  if (!SD.begin(SD_CHIPSELECT)) {
    Serial.println(F("Card initialization failed!"));
    return;
  }
  Serial.println(F("Card initialized."));

  dataFile = SD.open("data.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.println("Timestamp, tagNumber, deviceNumber, comment"); //comment this statement after the first excecution
    dataFile.close();
  }



  if (! rtc.begin()) {
    Serial.println(F("Couldn't find RTC"));
    Serial.flush();
    while (1) delay(10);
  }

  /*
    if (rtc.lostPower()) {
    Serial.println(F("RTC lost power, let's set the time!"));
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    //rtc.adjust(DateTime(2024, 3, 28, 3, 0, 0));
    }
  */
  // Init MFRC522
  //SPI.begin(); // Init SPI bus
  rfid.PCD_Init();

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("This code scan the MIFARE Classsic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);
}

void loop(void)
{
  display.setCursor(20, 0);
  display.println("TAG ID");
  
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent()) {
    Serial.println(F("Waiting for Card"));
    return;
  }

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  int deviceNumber = 1000;


  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  Serial.println(F("A new card has been detected."));
  String currentTime = getTimeStamp();


  // Store NUID into nuidPICC array
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];
  }
  //display.setCursor(10, 25);
  Serial.println(F("The NUID tag is:"));
  Serial.print(F("In hex: "));
  printHex(rfid.uid.uidByte, rfid.uid.size);

  Serial.print(F("Time: "));
  Serial.print(currentTime);
  Serial.println();

  Serial.print("tagIdS:");
  Serial.println(tagId);

  display.setCursor(0, 25);
  display.println(tagId);
  display.setCursor(0, 50);
  display.println(currentTimestamp);
  delay(1000);


  writeDataToCSV((currentTime + "," + tagId + "," + String(deviceNumber) + "," + "add comment"));
  delay(1000);
  tagId = "";
  display.clear();


  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}

/**
   Helper routine to dump a byte array as hex values to Serial.
*/
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
    tagId = tagId + " " + String(buffer[i]);
    Serial.print(" 0x");
    Serial.print(buffer[i], HEX);
    //display.print(buffer[i], HEX);
  }
}


void writeDataToCSV(String data) {
  dataFile = SD.open("data.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.println(data);
    dataFile.close();
  }
}
String getTimeStamp() {
  // Implement your timestamp logic here
  DateTime now = rtc.now();

  /*
    //int year = now.year();
    //Serial.print(year, DEC);
    Serial.print("/");

    //int month = now.month();
    //Serial.print(month, DEC);
    Serial.print("/");

    //int day = now.day();
    //Serial.print(day, DEC);
    Serial.print("/");

    //String dayoftheweek = (daysOfTheWeek[now.dayOfTheWeek()]);
    //Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print("() ");

    //int hour = now.hour();
    //Serial.print(hour, DEC);
    //Serial.print(':');

    //int minute = now.minute();
    //Serial.print(minute, DEC);
    Serial.print(':');

    //int second = now.second();
    Serial.print(second, DEC);
    Serial.println();

    Serial.println();
  */

  String timestamp = String(now.year()) + "/" + String(now.month()) + "/ " + String(now.day()) + " time " +  String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
  currentTimestamp = String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());
  delay(1000);
  return timestamp;
}
