#include <SPI.h>

#include <SD.h>

File data_file; 

const int GEIGER_BUFF_SIZE = 64+1;
const int TEMPERATURE_PIN = A4; 
const int LIGHT_PIN = A3; 
const int SD_PIN = 10;

float VoltToTempC (int reading);
float RawToLux(int raw);

unsigned int t1;
unsigned int t2;

int i = 0;

char      geigerData[sizeof(char) * GEIGER_BUFF_SIZE];

HardwareSerial & geiger = Serial;


bool ReadGeiger (Stream &geiger, char * dest, int destLength);
bool match (const char * s1, const char * s2, int len);

void setup()
{
  delay (2000);
  Serial.begin(9600);
  pinMode (SD_PIN, OUTPUT); 
  SD.begin (SD_PIN);
  //pinMode(TEMPERATURE_PIN, OUTPUT);
  //pinMode (LIGHT_PIN, OUTPUT); 
  
  data_file = SD.open ("Data.txt", FILE_WRITE);
  t1 = millis();
}



void loop ()
{
  int tempC;
  int light; 
  
   t2 = millis();
  //Serial.print(t2); Serial.print(" - "); Serial.print(t1); Serial.print(" = "); Serial.println(t2-t1);
  if (t2 - t1 >= 1000)  // NOTE: t2 >= t1 + 1000 and t2- t1 >= 1000 are NOT THE SAME; t2 >= t1 + 1000 does not handle timer overflow: when t1 = 64536, t2 == 64536 + 1000 == 1, now t2 < t1
  {
     t1 = t2; // reset start timer for next loop iteration 
    
    // Read sensors every 100ms
    //tempC = VoltToTempC (analogRead (TEMPERATURE_PIN));
    tempC = analogRead (TEMPERATURE_PIN);
    light = analogRead(LIGHT_PIN);
    //Serial.print(" Lux = ");
    //Serial.print(light);
    //Serial.print("DegC #1="); 
    //Serial.println (tempC);
    data_file.print ("DegC #1=");
    data_file.println (tempC);
    data_file.print ( " Lux = ");
    data_file.println (light);
  }


  // Read Geiger Counter and print data if it is ready
  
  if (ReadGeiger (geiger, geigerData, GEIGER_BUFF_SIZE))
  {
    //data_file.print ("Read:   "); 
    data_file.println (geigerData);
  }
  
   if (i % 20 == 0)
    {
      data_file.flush();
    }
  
    ++i;

}

float VoltToTempC (int reading)
{
  //int tempReading = analogRead(TEMPERATURE_PIN);
  float tempVolts = reading * 5.0 / 1023.0;
  float tempC = (tempVolts - 0.5) * 100.0;
  return tempC;
}

float RawToLux(int raw)
{
  float logLux = raw * 7.58 / 1023.0;
  return pow(10, logLux);
}

bool ReadGeiger (Stream &geiger, char * dest, int destLength)
{
  const char MSG_START[] = "CPS";                                     // message start token
  const int  MSG_START_LEN = (sizeof(MSG_START) -1) / sizeof(char);   // don't want trailing NULL
  const int  MSG_START_LAST = (sizeof(MSG_START) -2) / sizeof(char);  // don't want trailing NULL
  const int STOP_COMMA   = 6;
  
  char ch;
  static int state   = 0;
  static int i       = 0;
  static int commas  = 0;
  
  while (geiger.available() > 0)
  {
    // Check for possible caller buffer overflow - reset to S0
    if (i > destLength -1)
    {
      state   = 0;
      i       = 0;
      commas  = 0;
      dest[0] = '\0';
      //Serial.print ("ReadGeiger error:  i = "); Serial.print (i); Serial.print(" > "); Serial.print(destLength - 1);
      return false;
    }
    
    ch = geiger.read();
    
    switch (state)
    {
    // Try to match the start of a Geiger counter message ("CPS").
    case 0:
    case 1:
    case 2:
      if (MSG_START[state] == ch)   // Found a matching char, save it to output buffer, advance state
      {
        dest[i++] = ch;
        ++state;
      }
      /* Failed to match the message start, so first check whether the non-matching character is the beginning of 
         another message.  This might happen if the serial buffer was overwritten when the calling program reads 
         data more slowly than the Geiger counter produces output (once per second).  If the non-matching character 
         is the first character of the next message, reset to the beginning of S1 because this is equivalent to 
         having read the first character of the message. The output buffer will already contain the starting character, 
         so we don't need to copy it.
      */
      else if (ch == MSG_START[0])
      {
        state = 1;
        i = 1;
        commas = 0;
      }
      //  Found non-matching character that isn't the beginning of another message, so reset state to S0 to begin again
      else   
      {
        dest[0] = '\0';
        state = 0;
        i = 0;
        commas = 0;
      }
      break;
      
    // Found the start of the Geiger counter message, now copy the rest of it character by character until we get
    // to the final 6th comma.  Once we find that, stop, write a trailing NULL and be done!
    case 3: 
#ifdef COUNT_COMMAS
      if (',' == ch)
      {
        ++commas;
        
        dest[i++] = ch;
        
        //If found the ending comma, write a trailing null and return the data
        if (commas == STOP_COMMA)
        {
          dest[--i] = '\0';
          state = 0;
          i = 0;
          commas = 0;
 
          return (true);
        }
      }
#else       // alternate version looks for final newline.
      if ('\n' == ch)
      {
        dest[i++] = ch;
        
        dest[--i] = '\0';
        state = 0;
        i = 0;
        commas = 0;
 
        return (true);
      }
#endif
      else
      {
        // Character is part of the message--save it
        dest[i++] = ch;
        
        /*
           Check for MSG_START pattern here--a new message may start before the end of the previous message.  This
           might occur in a buffer overflow if caller calls us more slowly than the Geiger counter produces output.
           If we encounter the last character in the start of the message (CPS), see if the rest of the message start
           matches.  If so, then reset the curent state to the beginning of S3 because we just found the message
           start.  The output buffer will already contain the message start, so we don't need to copy it.
        */
        if ((ch == MSG_START[MSG_START_LAST]) && match (&dest[i-MSG_START_LEN], MSG_START, MSG_START_LEN)) 
        {
          state = 3;
          i = MSG_START_LEN;
          commas = 0; 
        }
      }
      break;
    }
  }  
  return (false);  // not enough input available - tell caller to call again later
}

// By design, does not add trailing NULL
bool match (const char * s1, const char * s2, int len)
{
  int i = 0;
  //Serial.print ("i = "); Serial.print (i); Serial.print(" len = "); Serial.println(len);
  while (i < len && (*s1++ == *s2++))  { ++i; }
  //Serial.print ("i = "); Serial.print (i); Serial.print(" len = "); Serial.println(len);  
  return (i == len);
}


