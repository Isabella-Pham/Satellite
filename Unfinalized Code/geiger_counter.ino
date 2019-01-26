/*The code below was tested on an Arduino Leonardo with the Geiger counter connected to Serial1
and the USB cable on Serial connected to the PC.  The data was output to the Serial Monitor.


Connections
-------------------------------------------

   Pin   Geiger Counter           Leonardo     Uno
   ----  -----------------------  --------     ______
   GND   GND on J6 marked "blk"   GND          GND
   TX    Pin5 on J6               RX1          *see note below
   
Note:  if you have an Uno or other Arduino that has only one serial port and you want to output
to a PC over USB or other serial device, you would need to use the SoftwareSerial library to read
the output from the Geigner counter on another digital pin.  Thus, you would use SoftwareSerial to 
read from the Geiger counter and Serial to write to the PC.  You would connect the Geiger
counter's TX pin to the RX pin specified when you unitialize SoftwareSerial.
------------------------------------------------------------------------------------------------*/


const int GEIGER_BUFF_SIZE = 64;
char szGeigerReading[GEIGER_BUFF_SIZE];

bool ReadGeiger(Stream &geiger, char * szBuff, int buffLen);
bool ReadGeiger(Stream &geiger, Stream &dest);

HardwareSerial &geigerCounter = Serial1;

void setup()
{
   while(!Serial) { ;}
   
   Serial.begin(9600);
   geigerCounter.begin(9600);
}


void loop()
{
   //unsigned long t1, t2; // timestamps for timing
   
   unsigned long runningTime;
   
   bool fResult;
  
   //t1 = millis();  // start timing
   
   runningTime = millis();
  
   // Simulate reading other sensors, 
   if (runningTime % 100 == 0)
   {
      Serial.print(runningTime); Serial.print(" ms,"); Serial.println("Light(lux),Temp(c)");
      delay(3);  // prevents loop wraparound within same millisecond causing two readings with the same timestamp.
   }
  
   if (runningTime % 1000 == 0)
   {
      // Option 1
   
      fResult = ReadGeiger(geigerCounter, szGeigerReading, GEIGER_BUFF_SIZE);
      Serial.print(runningTime); Serial.print(" ms,"); Serial.println(szGeigerReading);
      
      //Option 2
      //Serial.print(runningTime); Serial.print(" ms,");
      //fResult = ReadGeiger(geigerCounter, Serial);
   }
   
   //t2 = millis();  // end timing  
   //Serial.print("Time (ms) = "); Serial.println(t2 - t1, DEC);
   
}


/*------------------------------------------------------------------------------------------------
ReadGeiger (geiger, szBuff, buffLen)

Reads a geiger counter on a serial port and then writes it to a buffer named szBuff.


Test cases:

1. Didn't finish reading to the newline (\n) due to hardware error -- NULL terminate what we read
   but return false because whole string wasn't read.
2. Didn't finish reading to the newline (\n) due to buffer length too short -- NULL terminate what
   we read but return false because whole string wasn't read.
3. There is no data to read -- return false without doing anything.
4. Finished reading to the newline (\n) -- NULL terminate the string by overwriting the \n with a
   NULL and return true. This eliminates the trailing newline.  

------------------------------------------------------------------------------------------------*/
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


/*------------------------------------------------------------------------------------------------
ReadGeiger (geiger, dest)

Reads a Geiger counter on a serial port and writes it to a stream named dest.  Will also print
a trailing newline (\n).

Note that several different destinations implement the stream interface:  Serial, SD, etc.


Test cases:

1. Didn't finish reading to the newline (\n) due to hardware error -- NULL terminate what we read
   but return false because whole string wasn't read.
2. Didn't finish reading to the newline (\n) due to buffer length too short -- NULL terminate what
   we read but return false because whole string wasn't read.
3. There is no data to read -- return false without doing anything.
4. Finished reading to the newline (\n) -- return true because the whole reading was taken. 

------------------------------------------------------------------------------------------------*/
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
