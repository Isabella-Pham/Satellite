


const int COMMIT_PIN    = 4;   // digital input
const int LED_PIN       = 13;  // digital output

void setup()
{
   delay(2000);  // wait for hardware to power on and settle down

   Serial.begin(9600);
   while (!Serial) {;}  // wait for serial port to connect; needed for Leonardo
   
   
   pinMode(LED_PIN, OUTPUT); 
   pinMode(COMMIT_PIN, INPUT);
   
   // read commit pin here
   
   while (LOW == digitalRead(COMMIT_PIN))
   {
      Serial.println("Commit Pin in place");
      delay(500);
      digitalWrite(LED_PIN, HIGH);
      delay(500);
      digitalWrite(LED_PIN, LOW);
   }
   
   
}



void loop()
{
   
   if (LOW == digitalRead(COMMIT_PIN))
   {
      Serial.println("Commit Pin in place");
   }
   else
   {
      Serial.println("Commit Pin removed");
   }
   
   delay(1000);
   
}
