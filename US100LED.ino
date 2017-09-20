#include <Arduino.h>          // required before wiring_private.h
#include "wiring_private.h"   // pinPeripheral() function


#include <RTCZero.h>
#include <SPI.h>
#include <SD.h>


// Create Serial communication on pins 10(TX) and 11(RX)
Uart Serial2 (&sercom1, 11, 10, SERCOM_RX_PAD_0, UART_TX_PAD_2);
void SERCOM1_Handler() {
  Serial2.IrqHandler();
}

// Hardware Serial for Serial2
HardwareSerial *US100 = &Serial2;


// Initialize Variables
unsigned int FirstByte = 0;
unsigned int SecondByte = 0;
unsigned int cmDist = 0;
int temp = 0;

const int chipSelect = 4;

/* Create an rtc object */
RTCZero rtc;

/* Change these values to set the current initial time */
const byte seconds = 0;
const byte minutes = 47;
const byte hours = 14;

/* Change these values to set the current initial date */
const byte day = 20;
const byte month = 9;
const byte year = 17;

/* Change these values to set the alarm time */
const byte aSeconds = 0;
const byte aMinutes = 0;
const byte aHours = 9;

#define TAKE_READING_EACH_X_HOUR     1   //By default it will take reading each hour
#define TAKE_X_READINGS_AT_START             10

// Global Variables
int waitNumOfHour = 0;
#define VBATPIN A7    // Battery Voltage on Pin A7



void setup() {

  pinMode(8, OUTPUT); //
  pinMode(13, OUTPUT); //
  digitalWrite(13, LOW);

  Serial.begin(9600);
  US100->begin(9600);

  // Assign pins 10 & 11 SERCOM functionality
  pinPeripheral(10, PIO_SERCOM);
  pinPeripheral(11, PIO_SERCOM);

  rtc.begin();

  rtc.setTime(hours, minutes, seconds);
  rtc.setDate(day, month, year);

  rtc.setAlarmTime(aHours, aMinutes, aSeconds);
  rtc.enableAlarm(rtc.MATCH_MMSS); ///MATCH_HHMMSS-> Every day / MATCH_MMSS->EveryHour /MATCH_SS each minute

  // see if the card is present and can be initialized:
  // if (!SD.begin(chipSelect)) {
  // Serial.println("Card failed, or not present");
  // don't do anything more:
  //return;
  //}
  //Serial.println("card initialized.");
  for (int i = 1; i <= TAKE_X_READINGS_AT_START; i++)
  {


    digitalWrite(8, HIGH);
    // see if the card is present and can be initialized:
    if (!SD.begin(chipSelect)) {
      Serial.println("Card failed, or not present");
      return;
    }

    Serial.println("card initialized.");

    File dataFile = SD.open("test.csv", FILE_WRITE);

    String dataString = "";

    dataString =  BatteryVoltage ();
    dataString += ",";
    dataString += rtc.getEpoch();
    dataString += ",";
    dataString += takeDistance();
    dataString += ",";
    dataString += takeTemp();

    // if the file is available, write to it:
    if (dataFile) {
      dataFile.println(dataString);
      Serial.print("Reading Saved to SD card: "); Serial.println(dataString);
      dataFile.close();
      digitalWrite(8, LOW);
    }

    delay(10000);
  }


  rtc.attachInterrupt(alarmMatch);
  //rtc.standbyMode();
}

void loop() {
  //Serial.print("Temperature: "); Serial.println(i);
  rtc.standbyMode();
}

void alarmMatch() {
  waitNumOfHour += 1;
  if ( waitNumOfHour == TAKE_READING_EACH_X_HOUR) {
    String dataString = "";

    dataString =  BatteryVoltage ();
    dataString += ",";
    dataString += rtc.getDay();
    dataString += "/";
    dataString += rtc.getMonth();
    dataString += "/";
    dataString += rtc.getYear();
    dataString += " ";
    dataString += rtc.getHours();
    dataString += ":";
    dataString += rtc.getMinutes();
    dataString += ":";
    dataString += rtc.getSeconds();
    dataString += ",";
    dataString += rtc.getEpoch();
    dataString += ",";
    dataString += takeDistance();
    dataString += ",";
    dataString += takeTemp();



    digitalWrite(8, HIGH);
    // see if the card is present and can be initialized:
    if (!SD.begin(chipSelect)) {
      Serial.println("Card failed, or not present");
      return;
    }

    Serial.println("card initialized.");

    File dataFile = SD.open("datalog.csv", FILE_WRITE);

    // if the file is available, write to it:
    if (dataFile) {
      dataFile.println(dataString);
      Serial.print("Reading Saved to SD card: "); Serial.println(dataString);
      dataFile.close();
      digitalWrite(8, LOW);
    }

    waitNumOfHour = 0;
  }
}

double takeDistance() {
  US100->flush();         // Clear the buffer of the serial port
  US100->write(0x55);     // Request distance measurement

  digitalWrite(13, HIGH);
  delayMicroseconds(500000);
  // Initialize Variables
  unsigned int FirstByte = 0;
  unsigned int SecondByte = 0;
  double cmDist = 0;

  if (US100->available() >= 2) {    // Check if two bytes are available
    FirstByte = US100->read();      // Read both bytes
    SecondByte = US100->read();
    cmDist = (FirstByte * 256 + SecondByte) / 10; // Distance
  }

  if (cmDist > 420) {
    cmDist = 420;
  } else if ( cmDist <= 2) {
    cmDist = 2;
  }

  digitalWrite(13, LOW);
  return cmDist;
}

int takeTemp() {
  digitalWrite(13, HIGH);

  US100->flush();               // Clear the serial port buffer
  US100->write(0x50);           // Request temperature measurement
  int temp = 0;

  delayMicroseconds(500000);
  //delay(500);
  if (US100->available() >= 1) { // Check if one byte is available
    temp = US100->read();       // Read the byte
    if ((temp > 1) && (temp < 130)) { // Check the valid range
      temp -= 45;                     // Correct the offset of 45 degrees
    }
  }
  digitalWrite(13, LOW);
  return temp;
}

// Measure battery voltage using divider on Feather M0 - Only works on Feathers !!
float BatteryVoltage () {
  float measuredvbat;   // Variable for battery voltage
  float maxV = 4.2;
  float minV = 3.2;
  measuredvbat = analogRead(VBATPIN);   //Measure the battery voltage at pin A7
  measuredvbat *= 2;    // we divided by 2, so multiply back
  measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
  measuredvbat /= 1024; // convert to voltage
  float vbtPercentage = (measuredvbat - minV) / (maxV - minV) * 100;
  if (vbtPercentage > 100) {
    vbtPercentage = 100;
  } else if (vbtPercentage < 0 ) {
    vbtPercentage = 0;
  }
  return vbtPercentage;
}

