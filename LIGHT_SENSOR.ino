

const int LIGHT_PIN = A3; //light


void setup()
{
  delay (2000);
  Serial.begin (9600); 
  pinMode(LIGHT_PIN, OUTPUT);
}

void loop ()
{
  int light = analogRead(LIGHT_PIN);
  Serial.print(" - Lux = ");
  Serial.print(light);
  delay(1000);
}

float RawToLux(int raw)
{
  float logLux = raw * 5.0 / 1024.0;
  return pow(10, logLux);
}
