
const int LED_PIN       = 13;  // digital output


/*---------------------------------------------------------------------------------------------
Mission program setup

Initialize all sensors, wait for commit pin to signal start of the mission, write mission 
data header to the SD card
---------------------------------------------------------------------------------------------*/
void setup()
{
   delay(2000);  // wait for hardware to power on and settle down

   Serial.begin(9600);
   while (!Serial) {;}  // wait for serial port to connect; needed for Leonardo


   pinMode(LED_PIN, OUTPUT);

   
}


/*---------------------------------------------------------------------------------------------
Main program

Collect all sensor data and record it to the SD card <<frequency>>.

---------------------------------------------------------------------------------------------*/
void loop()
{
   // if statement
   int x = 5;
   if (x < 10)
   {
      Serial.print(x); Serial.println(" < 10");
   }
   else
   {
      Serial.print(x); Serial.println(" >= 10");
   }
   
   // while loop
   int i = 0;
   while (i < 10)
   {
      Serial.print("Loop "); Serial.println(i);
      
      digitalWrite(LED_PIN, HIGH);
      delay(250);
      digitalWrite(LED_PIN, LOW);
      delay(250);

      ++i;
   }


   // for loop
   for (i = 0; i < 10; ++i)
   {
      Serial.print("Loop "); Serial.println(i);
   }
 
   // function call
   float y; 
   y = function1(5, 6.0);
   Serial.print("y = "); Serial.println(y);
   
   
   // pointer
   
   int * p;
   
   p = &x;
   Serial.print("p = 0x"); Serial.println((size_t)p, HEX);  // the pointer itself is an address of a memory location
   Serial.print("*p= "); Serial.println(*p, DEC);           // the value at the location the pointer points to
}


// function definition
float function1 (int param1, float param2)
{
   Serial.println("Inside function1()");
   return (param1 * param2);
}
