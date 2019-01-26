/*-------------------------------------------------------------------------------------------
This program controls an Arduino UNO R3 or Leonardo with Sparkfun MicroSD Shield.


Note: to get the SD library to work with Leonardo, follow these steps:

http://forum.arduino.cc/index.php/topic,113058.msg887287.html

-------------------------------------------------------------------------------------------*/

#include <SD.h>
#include <SPI.h>
#include <stdio.h>


//const int COMMIT_PIN     = 7;   // digital input
const int LED_PIN        = 13;  // digital output
const int SD_CARD_PIN    = 8;   // Sparkfun SD Shield SPI SS pin
const int SD_LIBRARY_PIN = 10;  // digital output




/*---------------------------------------------------------------------------------------------
Mission program setup

Initialize all sensors, wait for commit pin to signal start of the mission, write mission 
data header to the SD card
---------------------------------------------------------------------------------------------*/
void setup()
{
   delay(2000);  // wait for hardware to power on and settle down

   Serial.begin(9600);
   while (!Serial) {;}  // wait for serial port to connect. Needed for Leonardo only

   pinMode(LED_PIN, OUTPUT);
   
   
   Serial.println("****  Initializing SD Card ******");
   
   // SD Card initialization
   pinMode(SD_CARD_PIN, OUTPUT);
   pinMode(SD_LIBRARY_PIN, OUTPUT);
   
   if(!SD.begin(10, 11, 12, 13))
   {
      Serial.println("SD card not present; please ensure FAT/FAT32-formatted card is inserted");
      
      // what error-handling strategy should we take?  switch to storing data in EEPROM?  flash warning light?
   }
   
   if (SD.exists("data.txt"))
      SD.remove("data.txt");
      
   
   File dataFile = SD.open("data.txt", FILE_WRITE);
   if (!dataFile)
   {
      //what error-handling?
      Serial.println("could not create data.txt");
      return;
   }
   
   Serial.println("****  Writing data.txt ******");
   
   dataFile.println ("Mission Data 02/15/2014");
   dataFile.println ("Reading,Temp,Pressure,Light");
   
   for (int i = 0; i < 100; ++i)
   {
      
      dataFile.print(i);dataFile.print(","); dataFile.print(i*1.25); dataFile.print(","); dataFile.println(i*1.5);
     
      // flush every 10th reading so we lose little should there be a power failure or other problem. 
      if (i % 10 == 0)
         dataFile.flush();
   }
   
   dataFile.close();
   
   Serial.println("****  Reading data.txt ******");
   
   dataFile = SD.open("data.txt", FILE_READ);
   
   while (dataFile.available())
   {
      Serial.write(dataFile.read());
   }
   
   dataFile.close();
   
   
   
   
}


/*---------------------------------------------------------------------------------------------
Main mission program

Collect all sensor data and record it to the SD card <<frequency>>.

---------------------------------------------------------------------------------------------*/
void loop()
{

   delay(1000);
   digitalWrite(LED_PIN, HIGH);
   delay(1000);
   digitalWrite(LED_PIN, LOW);
   
}

