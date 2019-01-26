

/*---------------------------------------------------------------------------------------------
Example program showing how to use the TSL 2561 Digital Light Sensor (AdaFruit breakout board 
part #439).  This sensor uses the i2c bus.


Connect the sensor to the Arduino with these pins:
  
  Connection   Arduino Uno    Arudino Leonardo  -> Arudino Mega  -> TSL 2561 Sensor
  ----------   -----------    ----------------     ------------     ------------------
  VCC          3.3v           3.3v                 3.3v             VCC
  GND          GND            GND                  GND              GND
  SCL          A5             SCL or digital 3     21               SCL
  SCA          A4             SDA or digital 2     20               SCA

References:
   http://learn.adafruit.com/tsl2561/overview
   http://www.adafruit.com/products/439


---------------------------------------------------------------------------------------------*/


#include <Wire.h>
#include "TSL2561.h"

// Example for demonstrating the TSL2561 library - public domain!

// connect SCL to analog 5
// connect SDA to analog 4
// connect VDD to 3.3V DC
// connect GROUND to common ground
// ADDR can be connected to ground, or vdd or left floating to change the i2c address

// The address will be different depending on whether you let
// the ADDR pin float (addr 0x39), or tie it to ground or vcc. In those cases
// use TSL2561_ADDR_LOW (0x29) or TSL2561_ADDR_HIGH (0x49) respectively
TSL2561 tsl(TSL2561_ADDR_FLOAT); 

void setup(void) {
   
  Serial.begin(9600);
  
  while (!Serial) { ; }  // Leonardo
  
  Serial.println("Press any key to begin");
  while (Serial.read() <= 0) {;}
  
  Serial.println("Attempting to initialize TSL2561 Light Sensor");
  if (tsl.begin()) {
    Serial.println("TSL2561 sensor found");
  } else {
    Serial.println("TSL2561 sensor not found");
    while (1);  // TODO handle error better than this -- in satellite, try to find again, and if not, skip this sensor
  }
    
  // You can change the gain on the fly, to adapt to brighter/dimmer light situations
  tsl.setGain(TSL2561_GAIN_0X);         // set no gain (for bright situtations)
  //tsl.setGain(TSL2561_GAIN_16X);      // set 16x gain (for dim situations)
  
  // Changing the integration time gives you a longer time over which to sense light
  // longer timelines are slower, but are good in very low light situtations!
  tsl.setTiming(TSL2561_INTEGRATIONTIME_13MS);  // shortest integration time (bright light)
  //tsl.setTiming(TSL2561_INTEGRATIONTIME_101MS);  // medium integration time (medium light)
  //tsl.setTiming(TSL2561_INTEGRATIONTIME_402MS);  // longest integration time (dim light)
  
  // Now we're ready to get readings!
}

void loop(void) {
  // Simple data read example. Just read the infrared, fullspecrtrum diode 
  // or 'visible' (difference between the two) channels.
  // This can take 13-402 milliseconds! Uncomment whichever of the following you want to read
  //uint16_t x = tsl.getLuminosity(TSL2561_VISIBLE);     
  //uint16_t x = tsl.getLuminosity(TSL2561_FULLSPECTRUM);
  //uint16_t x = tsl.getLuminosity(TSL2561_INFRARED);
  
  //Serial.println(x, DEC);

  // More advanced data read example. Read 32 bits with top 16 bits IR, bottom 16 bits full spectrum
  // That way you can do whatever math and comparisons you want!
  uint32_t lum = tsl.getFullLuminosity();
  uint16_t ir, full;
  ir = lum >> 16;
  full = lum & 0xFFFF;
  Serial.print("IR: "); Serial.print(ir);   Serial.print("\t\t");
  Serial.print("Full: "); Serial.print(full);   Serial.print("\t");
  Serial.print("Visible: "); Serial.print(full - ir);   Serial.print("\t");
  
  Serial.print("Lux: "); Serial.println(tsl.calculateLux(full, ir));
  
  delay(100); 
}
