// Digital sensor green is ground, red is SCL, black is SDA, white is VCC
#include <SD.h>
#include <SPI.h>
#include <stdio.h>
#include <Wire.h>
#include <TSL2561.h>

const int SD_CARD_PIN    = 8;   // Sparkfun SD Shield SPI SS pin
const int SD_LIBRARY_PIN = 10;  // digital output
const int COMMIT_PIN    = 4;   // digital input
const int LED_PIN       = 13;  // digital output
const int analogLight = A2; //select the input pin for the potentiometer
const int GEIGER_BUFF_SIZE = 64;

bool ReadGeiger(Stream &geiger, char * szBuff, int buffLen);
bool ReadGeiger(Stream &geiger, Stream &dest);

char szGeigerReading[GEIGER_BUFF_SIZE];
File dataFile;
File geigerFile;
unsigned long runningTime;
HardwareSerial &geigerCounter = Serial1;
TSL2561 tsl(TSL2561_ADDR_FLOAT); 

void setup() {
  // put your setup code here, to run once:
  // analogReference(EXTERNAL); //s
  
  delay(2000); // wait for hardware to power on and settle down
  
  //while (!Serial) {;} // wait for serial port to connect; needed for Leonardo
  
  // SD Card initialization
  pinMode(SD_CARD_PIN, OUTPUT);
  pinMode(SD_LIBRARY_PIN, OUTPUT);
   
  if(!SD.begin(10, 11, 12, 13))
  {
    //Serial.println("SD card not present; please ensure FAT/FAT32-formatted card is inserted");
      
    // what error-handling strategy should we take?  switch to storing data in EEPROM?  flash warning light?
  }
   
  //TODO
  //if (SD.exists("data.txt"))
    //SD.remove("data.txt");
      
  dataFile = SD.open("data.txt", FILE_WRITE);
  if (!dataFile)
  {
    while (1) {
      delay(250);
      digitalWrite(LED_PIN, HIGH);
      delay(250);
      digitalWrite(LED_PIN, LOW);
    }
    //what error-handling?
    //Serial.println("could not create data.txt");
    //return;
  }
   
  geigerFile = SD.open("geiger.txt", FILE_WRITE);
  if (!geigerFile)
  {
      while (1) {
      delay(500);
      digitalWrite(LED_PIN, HIGH);
      delay(500);
      digitalWrite(LED_PIN, LOW);
    }
    //what error-handling?
    //Serial.println("could not create data.txt");
    //return;
  }
  
  geigerFile.println("Starting geiger counter.");
  //Serial.begin(9600);
  geigerCounter.begin(9600);
  
  pinMode(LED_PIN, OUTPUT); 
  pinMode(COMMIT_PIN, INPUT);
  
  dataFile.println("Starting digital light sensor.");
  
  if (!tsl.begin()) {
    // TODO handle error better than this -- in satellite, try to find again, and if not, skip this sensor
  }
    
  // You can change the gain on the fly, to adapt to brighter/dimmer light situations
  tsl.setGain(TSL2561_GAIN_0X);         // set no gain (for bright situtations)
   
  // Changing the integration time gives you a longer time over which to sense light
  // longer timelines are slower, but are good in very low light situtations!
  tsl.setTiming(TSL2561_INTEGRATIONTIME_13MS);  // shortest integration time (bright light)
  
  // read commit pin here
  
  dataFile.println("Reading commit pin.");
  
  while (LOW == digitalRead(COMMIT_PIN))
  {
    //Serial.println("Commit Pin in place");
    delay(500);
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
  }
  
  dataFile.println("Commit pin pulled.");
  
  // write the data file headers
  
  dataFile.println ("Mission Data 03/29/2014");
  dataFile.println ("Running time (ms), Analog Light (lux), Digital Light (lux), Digital Light (ir), Digital Light (full), Digital Light (visible)");
  
  geigerFile.println("Mission Data 03/29/2014");
  geigerFile.println("Running time (ms), Geiger");

}

void loop() {
  // put your main code here, to run repeatedly:
  
  bool fResult;
  
  runningTime = millis();
  
  // Simulate reading other sensors, 
  if (runningTime % 100 == 0)
  {
    // read digital light sensor
    uint32_t lum = tsl.getFullLuminosity();
    uint16_t ir, full;
    ir = lum >> 16;
    full = lum & 0xFFFF;
     
    // read the raw value from the analog light sensor
    int rawValue = analogRead(analogLight);
     
    dataFile.print(runningTime); dataFile.print(",");
    dataFile.print(RawToLux(rawValue)); dataFile.print(",");
    dataFile.print(tsl.calculateLux(full, ir)); dataFile.print(",");
    dataFile.print(ir); dataFile.print(",");
    dataFile.print(full); dataFile.print(",");
    dataFile.println(full - ir);
      
    // flush every 20th reading so we lose little should there be a power failure or other problem. 
    if (runningTime % 2000 == 0)
    {
      dataFile.flush();
    }
     
    delay(3);  // prevents loop wraparound within same millisecond causing two readings with the same timestamp.
  }
  
  if (runningTime % 1000 == 0)
  {
    fResult = ReadGeiger(geigerCounter, szGeigerReading, GEIGER_BUFF_SIZE);
        
    geigerFile.print(runningTime); geigerFile.print(",");
    geigerFile.println(szGeigerReading);
      
    // flush every 20th reading so we lose little should there be a power failure or other problem. 
    if (runningTime % 20000 == 0)
    {
      geigerFile.flush();
    }
  }
}

float RawToLux(int raw)
{
  const float rawRange = 1024; // 3.3v
  const float longRange = 5.0; // 3.3v = 10^5 lux

  float logLux = raw * longRange / rawRange;
  return pow(10, logLux);
}

bool ReadGeiger(Stream &geiger, char * szBuff, int buffLen)
{
   bool fResult = false;  // assume we won't get a full reading
   
   if (geiger.available())
   {
      int i = 0;
      
      for (i = 0; geiger.available(), i < (buffLen-1); ++i)  // buffLen - 1 so we have room to add trailing NULL
      {
         szBuff[i] = (char)geiger.read();
         if (szBuff[i] == '\n')
         {
            // last character copied to buffer was \n so we got a full reading.  drop out of the loop, return success, and
            //  then overwrite the trailing \n with NULL
            fResult = true;
            break;
         }
      }
      szBuff[i] = '\0';  // always NULL terminate the string
   }

   return (fResult);   
}

bool ReadGeiger(Stream &geiger, Stream &dest)
{
   bool fResult = false;
   char ch;
   
   if (geiger.available())
   {
      
      while(geiger.available())
      {
         ch = (char)geiger.read();
         dest.write(ch);
         
         if (ch == '\n')
         {
            fResult = true;
           break;
         }
      }
   }
 
   return (fResult);  
}
