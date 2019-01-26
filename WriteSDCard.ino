
/*

*/


#include <SPI.h>
#include <SD.h>

const int HARDWARE_SETTLE_TIME = 2000;
const int SD_SELECT    = 10;
const int SDLIB_SELECT = 10;  // need to raise this to output for SD Library

File dataFile;
File geigerFile;

bool sd_init = false;
int dataFile_errors = 0;
int geigerFile_errors = 0;


int i = 1;

unsigned int t1;
unsigned int t2;



void setup() 
{
  
  delay(HARDWARE_SETTLE_TIME);
  
  pinMode (SD_SELECT, OUTPUT);
  pinMode (SDLIB_SELECT, OUTPUT);  // required by SD Library (lame)
  
  Serial.begin (9600);
  
  
  Serial.println ("Starting");
  
  sd_init = SD.begin(SD_SELECT);
  if (!sd_init)
  {
    Serial.println ("SD Card not initialized");
  }
  else
  {
    Serial.println ("SD Card ready");
  }  
  
  
  if (sd_init)
  {
    if (SD.exists ("data.txt"))
    {
      SD.remove ("data.txd");
      Serial.println ("previous data.txt removed");
    }
    if (SD.exists ("geiger.txt"))
    {
      SD.remove ("geiger.txd");
      Serial.println ("previous geiger.txt removed");
    }
    
    
    dataFile = SD.open("data.txt", FILE_WRITE);
    
    if (dataFile)
    {
      Serial.println ("data.txt opened");
    }
    
    geigerFile = SD.open("geiger.txt", FILE_WRITE);
    
    if (geigerFile)
    {
      Serial.println ("geiger.txt opened");
    }
  }
  
  t1 = millis();
}

void loop() 
{
  
  t2 = millis();
  //Serial.print(t2); Serial.print(" - "); Serial.print(t1); Serial.print(" = "); Serial.println(t2-t1);
  if (t2 - t1 >= 1000)  // NOTE: t2 >= t1 + 1000 and t2- t1 >= 1000 are NOT THE SAME; t2 >= t1 + 1000 does not handle timer overflow: when t1 = 64536, t2 == 64536 + 1000 == 1, now t2 < t1
  {
    t1 = t2; // reset start timer for next loop iteration 
  
    
    Serial.println(i);
    
    if (!dataFile.print(i) || !dataFile.println(" Hello, World"))
    {
      // handle write error
      ++dataFile_errors;
      Serial.println("dataFile write error");
    }
    
    if (!geigerFile.print(i) || !geigerFile.println(" Hello, Geiger"))
    {
      // handle write error
      ++geigerFile_errors;
      Serial.println("geigerFile write error");
    }
    
    // Flush file after every so often to ensure data is written to disk in case power is lost
    if (i % 20 == 0)
    {
      dataFile.flush();
      geigerFile.flush();
    }
  
  
    //delay (100);
  
    ++i;

  }
}
