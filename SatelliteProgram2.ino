
//#define DEBUG
#include <dbghelp.h>
#include <SPI.h>
#include <Wire.h>


const int GSCALE  =  2;     // forMMA8452 Accelerometer.  Sets full-scale range to +/-2, 4, or 8g. Used to calc real g values.

const int COMMIT_PIN          = 7;   // digital input
const int LED_PIN             = 13;  // digital output
const int RESET_OPENLOG_PIN   = 8;   // digital, output
const int PRESSURE_SS_PIN     = 10;  // digital, output ; this pin controls SPI BUS
const int HUMIDITY_SENSOR_PIN = 2;   // analog, input


HardwareSerial &openLog1 = Serial;   // Create a reference to Serial
bool fOpenLogRunning = false;


/*---------------------------------------------------------------------------------------------
Mission program setup

Initialize all sensors, wait for commit pin to signal start of the mission, write mission 
data header to OpenLog
---------------------------------------------------------------------------------------------*/
void setup()
{
  delay(2000);  // wait for hardware to power on and settle down
  
  openLog1.begin(9600);  // Set up communication with the OpenLog
  
  // Set default pin states
  pinMode(COMMIT_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RESET_OPENLOG_PIN, OUTPUT);
  pinMode(PRESSURE_SS_PIN, OUTPUT);
  
  // For OpenLog -- don't try to call it if it couldn't be initialized.
  fOpenLogRunning = OpenLog_Init(RESET_OPENLOG_PIN);
  if (fOpenLogRunning)
  {
    OpenLog_AppendFile("SENSOR.TXT");  
  }
  
  // For Pressure Sensor (SPI Interface)
  SPI.begin();
  SPI.setDataMode(SPI_MODE3);
  
  // For Accelerometer (I2C Interface)
  Wire.begin();
  initMMA8452();
  
  // Wait for commit pin
  while (digitalRead(COMMIT_PIN))
  {
    openLog1.println("Waiting for commit pin");
    delay(1000);
  }

  // Write mission data header
  openLog1.println("Start Mission Data");
  openLog1.println("Pressure(psi),Temp(C),Humidity(%),X(g),Y(g),Z(g)");
}

/*---------------------------------------------------------------------------------------------
Main mission program

Collect all sensor data and record to OpenLog once per 100ms.

---------------------------------------------------------------------------------------------*/
void loop()
{
  float psi;
  float tempC;
  float humidity;
  float x, y, z;
  
  if (GetPressureAndTemp (PRESSURE_SS_PIN, &psi, &tempC))
  {
    //Serial.print("Pressure = "); Serial.print(psi); Serial.println(" psi");
    //Serial.print("Temp     = "); Serial.print(tempC); Serial.println(" C");
  }
  
  if (GetHumidity (HUMIDITY_SENSOR_PIN, tempC, &humidity))
  {
    //Serial.print("Humidity = "); Serial.print(humidity); Serial.println("%");
  }
  
  if (GetAcceleration (&x, &y, &z))
  {
    //Serial.print(x);Serial.print(",");Serial.print(y);Serial.print(",");Serial.println(z);
  }  
  
  openLog1.print(psi); openLog1.print(","); openLog1.print(tempC); openLog1.print(","); openLog1.print(humidity); openLog1.print(",");
  openLog1.print(x); openLog1.print(","); openLog1.print(y); openLog1.print(","); openLog1.println(z);
  
  delay(100);               // wait for 100ms
}


/*---------------------------------------------------------------------------------------------
GetPressureAndTemp (pin, pPressure, pTempC)

Read Honeywell TruStability SSC pressure sensor and return both pressure in PSI and temperature
in degrees C.

Sensor Specs:
  Model # SSC-DNNN015PASA5
  8-pin DIP
  0-15 psi
  SPI Interface
  5v supply voltage

Connect the pressure sensor to the Arduino with these pins:
  
  SPI Pin     Arduino Uno    Arudino Mega  -> SSC presure Sensor
  -------     -----------    ------------     ------------------
  GND         GND            GND              Pin 1
  VCC         5v             5v               Pin 2
  MISO        12             50               Pin 3
  SCLK        13             52               Pin 4
  SS          10             53               Pin 5
  
References:
http://sensing.honeywell.com/index.cfm/ci_id/156989/la_id/1/document/1/re_id/0
---------------------------------------------------------------------------------------------*/

boolean GetPressureAndTemp (int pin, float * pPressure, float * pTempC)
{
  const int maxVal =  14745 ; // counts; 90% * 2^14 = 0.9 * 16,384
  const int minVal =  1638;   // counts; 10% * 2^14 = 0.1 * 16,384
  const int maxPress = 15;    // psi
  const int minPress = 0;     // psi
  
  byte b1, b2, b3, b4;
  
  digitalWrite (pin, LOW);  // SPI interface treates LOW as "selected slave"
  b1 = SPI.transfer(0);
  b2 = SPI.transfer(0);
  b3 = SPI.transfer(0);
  b4 = SPI.transfer(0);  
  digitalWrite (pin, HIGH);  // done so release SPI SS
  
  // Two high-order bits are sensor's status bits; if both are zero, no error occurred
  byte fResult = (b1 >> 6) == 0 ? true : false;
 
  if (fResult)
  {
    int val;
    
    // Compute Pressure
    val = ((int)b1 << 8 | b2) & 0x3FFF;  // Pressure output is 14-bit # of counts
    float pressure = ((float)(val - minVal) * (float)(maxPress - minPress)) / ((float)(maxVal - minVal)) + minPress;
    
    //Compute Temperature
    val = ((int)b3 << 8 | b4) >> 5;
    float tempC = ((float)val / 2047) * 200 - 50;
    
    // Write pressure and temp values to output parameters
    *pPressure = pressure;
    *pTempC = tempC;
  }
  
  return (fResult);
}


/*---------------------------------------------------------------------------------------------
GetHumidity (pin, tempC, pHumidity)

Read Honeywell HIH-4030 humidity sensor and return percent relative humidity.


References:
http://arduino.cc/forum/index.php/topic,19961.0.html
---------------------------------------------------------------------------------------------*/

boolean GetHumidity (int pin, float tempC, float * pHumidity)
{
  int val;
  
  val = analogRead(pin);
  
  // Compute relative humidity % (0-100%) not corrected for temperature
  float humidity = ((((val/1023.0)*5.0)-0.8)/3.1)*100;
  
  // Correct humidity for temperature
  humidity = (humidity / (105.46 - 0.216 * tempC)) * 100;
  
  // Write humidity value to output parameters
  *pHumidity = humidity;

  return (true);
}


/*---------------------------------------------------------------------------------------------
GetAcceleration (pX, pY, pZ)

Read MMA8452Q accelerometer and return one reading of X,Y,Z data and return in units of earth's 
gravitational pull (g)

---------------------------------------------------------------------------------------------*/

boolean GetAcceleration (float * pX, float * pY, float *pZ)
{
  int accelCount[3];  // Stores the 12-bit signed value
  readAccelData(accelCount);  // Read the x/y/z adc values

  /*
  // Now we'll calculate the accleration value into actual g's
  float accelG[3];  // Stores the real accel value in g's
  for (int i = 0 ; i < 3 ; i++)
  {
    accelG[i] = (float) accelCount[i] / ((1<<12)/(2*GSCALE));  // get actual g value, this depends on scale being set
  }

  // Print out values
  
  for (int i = 0 ; i < 3 ; i++)
  {
    Serial.print(accelG[i], 4);  // Print g values
    Serial.print("\t");  // tabs in between axes
  }
  Serial.println();
  */

  // Write humidity value to output parameters
  *pX = (float) accelCount[0] / ((1<<12)/(2*GSCALE));  // convert raw value to units of gravity 
  *pY = (float) accelCount[1] / ((1<<12)/(2*GSCALE));  // convert raw value to units of gravity
  *pZ = (float) accelCount[2] / ((1<<12)/(2*GSCALE));  // convert raw value to units of gravity

  return (true);
}


/*---------------------------------------------------------------------------------------------
OpenLog_Init (resetPin)

Prepares the OpenLog for use.  When this function returns true, the OpenLog will be in data
logging mode and ready for use.

  resetPin
    Set this to zero if you don't want to use the reset wire.  Otherwise, set this to the
    Arduino's digital output pin that's connected to the reset pin on the OpenLog.
---------------------------------------------------------------------------------------------*/

boolean OpenLog_Init(int resetPin)
{
  enum state_t {INITIALIZED, ERROR, TIMEOUT};
  const int MAX_TRIES = 2;
  int     i;
  char    ch;
  
  DebugSprintf ("Enter Init\r");
  
  // Wait for OpenLog to respond with '<' to indicate it is awake and in logging mode.
  // If we don't get '<' after a reasonable time, reset the device and wait again.
  state_t state = TIMEOUT; // assume timeout
  
  for (int n = 0; n < MAX_TRIES && INITIALIZED != state; ++n)
  {
    // Wait for '<' for up to 3 seconds
    for (i = 0; i < 1000 && INITIALIZED != state; ++i)
    {
      delay (3);
      if(openLog1.available())
      {
        ch = openLog1.read();
        DebugWriteChar(ch);
        if ('<' == ch) { state = INITIALIZED; }
      }
    }

    DebugSprintf ("\ri = %d\r", i);
    DebugSprintfIf (INITIALIZED != state, "\rDid not get '12<'\r"); 

    if (INITIALIZED != state)
    {
      OpenLog_HardReset (resetPin);
      DebugSprintf ("OpenLog Hard Reset\r");
    }
  }

  DebugSprintf ("Leave Init\r");
  
  return (INITIALIZED == state) ? true : false;
}


/*---------------------------------------------------------------------------------------------
OpenLog_AppendFile (szFileName)

---------------------------------------------------------------------------------------------*/

boolean OpenLog_AppendFile (const char * szFileName)
{
  boolean fResult;
  char szCmd[50];    // Holds OpenLog commands
  
  DebugSprintf ("Enter AppendFile\r");

  fResult = OpenLog_EnterCommandMode(); 
  if (!fResult) 
    goto EXIT;
 
  DebugSprintf ("AppendFile: Entered Command Mode\r");

  // Check if the file exists by seeing if the size command succeeds
  sprintf(szCmd, "size %s\r", szFileName);
  openLog1.print(szCmd);
  fResult = OpenLog_WaitForCommandReply();
  
  DebugSprintf ("AppendFile: %s %s\r", szFileName, fResult ? "exists" : "doesn't exist");

  // If the file doesn't exist, create a new one.
  if (!fResult)
  {
    sprintf(szCmd, "new %s\r", szFileName);
    openLog1.print(szCmd);
    fResult = OpenLog_WaitForCommandReply();
    if (!fResult) 
      goto EXIT;
       
    DebugSprintf ("AppendFile: Created %s\r", szFileName);   
  }

  // Open the file for append mode.
  sprintf(szCmd, "append %s\r", szFileName);
  openLog1.print(szCmd);
  fResult = OpenLog_WaitForCommandReply();
  
  DebugSprintf ("AppendFile: Append mode = %s\r", fResult ? "true" : "false");

EXIT:
  DebugSprintf ("Leave AppendFile\r");

  return (fResult);
}


/*---------------------------------------------------------------------------------------------
OpenLog_EnterCommandMode (szFileName)

---------------------------------------------------------------------------------------------*/

boolean OpenLog_EnterCommandMode()
{
  DebugSprintf ("Enter CommandMode\r"); 

  // Send three Ctrl+Z characters to enter OpenLog command mode, then
  // wait for OpenLog to respond that the command finished.
  openLog1.write(26); openLog1.write(26); openLog1.write(26);
  
  DebugSprintf ("Leave CommandMode\r");
  
  return (OpenLog_WaitForCommandReply());
}


/*---------------------------------------------------------------------------------------------
OpenLog_WaitForCommandReply (szFileName)

make this private if we create a class
---------------------------------------------------------------------------------------------*/

boolean OpenLog_WaitForCommandReply()
{     
  DebugSprintf ("Enter WaitForCommandReply\r");
  
  // Wait for OpenLog to respond with '>' or '<' to indicate the command finished.
  
  // What happens if there are no characters such as calling this twice in succession?
  // The second call must not hang.  It needs to time out.
  char ch;
  boolean fResult = true;  // assume the command completed successfully
  
  while(true) 
  {
    if(openLog1.available())
    {
      ch = openLog1.read();
      DebugWriteChar(ch);
    
      // If we get !> an error occurred.  On ! just note the error,
      // then continue through the loop to drain the rest of the
      // characters including the final >
      if ('!' == ch || ':' == ch) { fResult = false; }  // append command will return error : XXXX on failure
      if ('>' == ch || '<' == ch) { break; }            // append command will return < on success
    }
  }
    
  DebugSprintf ("Leave WaitForCommandReply\r");
  
  return (fResult);
}


/*---------------------------------------------------------------------------------------------
OpenLog_HardReset (szFileName)

---------------------------------------------------------------------------------------------*/

boolean OpenLog_HardReset(int resetPin)
{
  // Setting the reset pin to 0v resets the OpenLog; be sure to set it back to high
  // Short delay after resetting the OpenLog ensures it's ready to receive data or commands.
  digitalWrite(resetPin, LOW);
  delay(100);
  digitalWrite(resetPin, HIGH);
  delay(100);
  return (true);
}




/*---------------------------------------------------------------------------------------------
 The code for the MMA8452 Accelerometer was copied from:
 
 MMA8452Q Basic Example Code
 Nathan Seidle
 SparkFun Electronics
 November 5, 2012

 ---------------------------------------------------------------------------------------------*/

// The SparkFun breakout board defaults to 1, set to 0 if SA0 jumper on the bottom of the board is set
#define MMA8452_ADDRESS 0x1D  // 0x1D if SA0 is high, 0x1C if low

//Define a few of the registers that we will be accessing on the MMA8452
#define OUT_X_MSB 0x01
#define XYZ_DATA_CFG  0x0E
#define WHO_AM_I   0x0D
#define CTRL_REG1  0x2A


void readAccelData(int *destination)
{
  byte rawData[6];  // x/y/z accel register data stored here

  readRegisters(OUT_X_MSB, 6, rawData);  // Read the six raw data registers into data array

  // Loop to calculate 12-bit ADC and g value for each axis
  for(int i = 0; i < 3 ; i++)
  {
    int gCount = (rawData[i*2] << 8) | rawData[(i*2)+1];  //Combine the two 8 bit registers into one 12-bit number
    gCount >>= 4; //The registers are left align, here we right align the 12-bit integer

    // If the number is negative, we have to make it so manually (no 12-bit data type)
    if (rawData[i*2] > 0x7F)
    {  
      gCount = ~gCount + 1;
      gCount *= -1;  // Transform into negative 2's complement #
    }

    destination[i] = gCount; //Record this gCount into the 3 int array
  }
}

// Initialize the MMA8452 registers 
// See the many application notes for more info on setting all of these registers:
// http://www.freescale.com/webapp/sps/site/prod_summary.jsp?code=MMA8452Q
void initMMA8452()
{
  byte c = readRegister(WHO_AM_I);  // Read WHO_AM_I register
  if (c == 0x2A) // WHO_AM_I should always be 0x2A
  {  
    //Serial.println("MMA8452Q is online...");
  }
  else
  {
    Serial.print("Could not connect to MMA8452Q: 0x");
    Serial.println(c, HEX);
    //while(1) ; // Loop forever if communication doesn't happen
  }

  MMA8452Standby();  // Must be in standby to change registers

  // Set up the full scale range to 2, 4, or 8g.
  byte fsr = GSCALE;
  if(fsr > 8) fsr = 8; //Easy error check
  fsr >>= 2; // Neat trick, see page 22. 00 = 2G, 01 = 4A, 10 = 8G
  writeRegister(XYZ_DATA_CFG, fsr);

  //The default data rate is 800Hz and we don't modify it in this example code

  MMA8452Active();  // Set to active to start reading
}

// Sets the MMA8452 to standby mode. It must be in standby to change most register settings
void MMA8452Standby()
{
  byte c = readRegister(CTRL_REG1);
  writeRegister(CTRL_REG1, c & ~(0x01)); //Clear the active bit to go into standby
}

// Sets the MMA8452 to active mode. Needs to be in this mode to output data
void MMA8452Active()
{
  byte c = readRegister(CTRL_REG1);
  writeRegister(CTRL_REG1, c | 0x01); //Set the active bit to begin detection
}

// Read bytesToRead sequentially, starting at addressToRead into the dest byte array
void readRegisters(byte addressToRead, int bytesToRead, byte * dest)
{
  Wire.beginTransmission(MMA8452_ADDRESS);
  Wire.write(addressToRead);
  Wire.endTransmission(false); //endTransmission but keep the connection active

  Wire.requestFrom(MMA8452_ADDRESS, bytesToRead); //Ask for bytes, once done, bus is released by default

  while(Wire.available() < bytesToRead); //Hang out until we get the # of bytes we expect

  for(int x = 0 ; x < bytesToRead ; x++)
    dest[x] = Wire.read();    
}

// Read a single byte from addressToRead and return it as a byte
byte readRegister(byte addressToRead)
{
  Wire.beginTransmission(MMA8452_ADDRESS);
  Wire.write(addressToRead);
  Wire.endTransmission(false); //endTransmission but keep the connection active

  Wire.requestFrom(MMA8452_ADDRESS, 1); //Ask for 1 byte, once done, bus is released by default

  while(!Wire.available()) ; //Wait for the data to come back
  return Wire.read(); //Return this one byte
}

// Writes a single byte (dataToWrite) into addressToWrite
void writeRegister(byte addressToWrite, byte dataToWrite)
{
  Wire.beginTransmission(MMA8452_ADDRESS);
  Wire.write(addressToWrite);
  Wire.write(dataToWrite);
  Wire.endTransmission(); //Stop transmitting
}
