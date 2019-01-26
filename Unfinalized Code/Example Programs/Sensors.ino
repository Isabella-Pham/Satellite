
#include <SPI.h>

const int PRESSURE_PIN  = 7;   // digital output & input
const int LED_PIN       = 13;  // digital output

const int TMP36_PIN     = A2;  // analog input
const int HIH4030_PIN   = A3;  // analog input

const float ANALOG_REF_VOLTAGE = 5.0f;

/*---------------------------------------------------------------------------------------------
Initialize all sensors
---------------------------------------------------------------------------------------------*/
void setup()
{
   delay(2000);  // wait for hardware to power on and settle down

   Serial.begin(9600);
   while (!Serial) {;}  // wait for serial port to connect; needed for Leonardo when connecting to Serial Monitor


   delay(2000);  // make it easy to open serial monitor to see output

   pinMode(LED_PIN, OUTPUT);
   pinMode(PRESSURE_PIN, OUTPUT);

}


/*---------------------------------------------------------------------------------------------
Main program

Collect all sensor data and write it to the serial port.

---------------------------------------------------------------------------------------------*/
void loop()
{
   boolean fResult;
   float degC_1;
   float degC_2;
   float pressure;
   float humidity;
  
   
   // Read sensors once per second
   if (millis() % 1000 == 0)
   { 
      // Read Temp
      degC_1 = ToCelcius_Tmp36(ReadAnalogSensor(TMP36_PIN));
      Serial.print ("DegC #1= "); Serial.println(degC_1);
      
      // Read Humidity
      humidity = ToHumidity_HIH4030 (ReadAnalogSensor(HIH4030_PIN), degC_1);
      Serial.print ("RH % = "); Serial.println(humidity);
      
      // Read pressure & temp
      fResult = GetPressureAndTemp(7, &pressure, &degC_2);
      if (fResult)
      {
         Serial.print ("PSI = "); Serial.println(pressure);
         Serial.print ("DegC #2 = "); Serial.println(degC_2);
      }
   }


}


/*---------------------------------------------------------------------------------------------
ReadAnalogSensor (pin)

Reads an analog sensor connected to the Arduino's built-in 10-bit DAC and returns a voltage 
relative to the analog reference voltage specified by the constant ANALOG_REF_VOLTAGE.

Returns a voltage between 0 and ANALOG_REF_VOLTAGE (3.3v or 5v, depending on Arduino board),
given an analog input between 0 and 1023.

TODO:  does Arduino DUE use 10-bit DAC?  If not, special case for that; use default args


See references below for explanation of impedance when reading multiple analog sensors at
the same time.

References:

http://www.adafruit.com/blog/2010/01/29/how-to-multiplex-analog-readings-what-can-go-wrong-with-high-impedance-sensors-and-how-to-fix-it/
http://forums.adafruit.com/viewtopic.php?f=25&t=11597


---------------------------------------------------------------------------------------------*/
float ReadAnalogSensor (int pin)
{
   int rawVal = analogRead(pin);
   delay (5);
   rawVal = analogRead(pin);
   delay(5);
   return (rawVal/1024.0 * ANALOG_REF_VOLTAGE);
}



/*---------------------------------------------------------------------------------------------
ToCelcius_Tmp36 (volts)

Converts Analog Devices TMP 36 temperature sensor output voltage to temperature in degrees C.

Sensor Specs:
  Model #TMP36
  Analog Interface
  5v supply voltage  (note: will get more precision if run @ 3.3v and use 3.3v as analog reference
                      because sensor produces voltages between 0.1 - 1.75v)

Connect the sensor to the Arduino with these pins:
  
  Connection   Arduino Uno    Arudino Mega  -> TMP 36 Sensor
  ----------   -----------    ------------     ------------------
  VCC          5v             5v               Pin 1
  Signal       A3             A3               Pin 2
  GND          GND            GND              Pin 3

References:
http://dlnmh9ip6v2uc.cloudfront.net/datasheets/Sensors/Temp/TMP35_36_37.pdf

Conversion to degrees C:
------------------------
The datasheet says the sensor has an output voltage scale factor of 10mV/1ºC and produces a 
value of 750mV @ 25ºC.  This leads to the linear equation:

   750v = 10mV/1ºC * 25ºC + 0.5v
   
If we let V be the output voltage and C the measured degrees celcius, and convert the scale
factor from mV to V, we get the equation:

   V = 10mV/1ºC * C + 0.5v
     = 0.010v/1ºC * C + 0.5v
     = 0.010 * C + 0.5
   
Since V is produced by the sensor and we want to know C, solve for C:

   C = (V - 0.5) / 0.010
     = (V - 0.5) * 100
     = 100*V - 50

Verify this with known outputs from figure 6 on page 5 of the data sheet:
0.1v @ -40ºC, 1v @ 50ºC, 1.75v @ 125ºC.

To convert the raw reading to a voltage, use the equation:
   V = (reading/1024) * vRef  
   
where vRef is the reference voltage (5v or 3.3v) used to operate the sensor.
---------------------------------------------------------------------------------------------*/
float ToCelcius_Tmp36 (float volts)
{
   return (100 * volts - 50);
}


/*---------------------------------------------------------------------------------------------
ToHumidity_HIH4030 (rawV, degC)

Converts Honeywell HIH-4030 humidity sensor output voltage to relative humidity %.  Note that
relative humidity is dependent on temperature in degrees C.

Sensor Specs:
  Model #HIH-4030
  Analog Interface
  5v supply voltage

Connect the sensor to the Arduino with these pins:
  
  Connection   Arduino Uno    Arudino Mega  -> HIH-4030 Sensor
  ----------   -----------    ------------     ------------------
  VCC          5v             5v               Pin 1
  Signal       A4             A4               Pin 2
  GND          GND            GND              Pin 3

References:
http://sensing.honeywell.com/honeywell-sensing-hih4030-4031%20series-product-sheet-009021-4-en.pdf?name=HIH-4030-001
https://www.sparkfun.com/products/9569


Conversion to % relative humidity:
----------------------------------


---------------------------------------------------------------------------------------------*/
float ToHumidity_HIH4030 (float volts, float degC)
{
   return (volts);
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
http://sensing.honeywell.com/spi%20comms%20digital%20ouptu%20pressure%20sensors_tn_008202-3-en_final_30may12.pdf 
http://sensing.honeywell.com/index.php?ci_id=49918&la_id=1&
http://sensing.honeywell.com/honeywell-sensing-ssc-digital-silicon-pressure-sensors-product-sheet-008213-2-en.pdf


Conversion to PSI
-----------------
The datasheet says the sensor returns # of counts and that the conversion function is:

   C% = 80% / (Pmax - Pmin) * (P - Pmin) + 10%  (% of 2^14 counts; 80% comes from 90% @ max - 10% @ min)
   C = 13108 /(Pmax - Pmin) * (P - Pmin) + 1638  (13108 = 14746 - 1638)

With Output Values:

    %     Digital counts
  ---     --------------
    0         0
   10      1638
   50      8192
   90     14746
  100     16383  (2^14 - 1)

Solving for P:

   P = (C% - 10%) * (Pmax - Pmin) / 80% + Pmin    | P = (C% - 10%) * (Pmax - Pmin) / (90% - 10%) + Pmin 
   P = (C - 1638) * (Pmax - Pmin) / 13108 + Pmin  | P = (C - 1638) * (Pmax - Pmin) / (14746-1638) + Pmin


Conversion to Temperature
-------------------------
The datasheet says the temperature equation is:

   DegC = (output / 2047 * 200)  - 50


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
