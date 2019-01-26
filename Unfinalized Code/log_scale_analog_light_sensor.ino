const int analogSensorPin = A2; //select the input pin for the potentiometer

void setup() {
  // put your setup code here, to run once:
  // analogReference(EXTERNAL); //s
  
  delay(2000); // wait for hardware to power on and settle down
  
  Serial.begin(9600);
  while (!Serial) {;} // wait for serial port to connect; needed for Leonardo
  
}

void loop() {
  // put your main code here, to run repeatedly:
  // read the raw value from the sensor:
  int rawValue = analogRead(analogSensorPin);
  
  Serial.print("Raw = ");
  Serial.print(rawValue);
  Serial.print(" - Lux = ");
  Serial.print(RawToLux(rawValue));
  delay(1000);
}

float RawToLux(int raw)
{
  const float rawRange = 1024; // 3.3v
  const float longRange = 5.0; // 3.3v = 10^5 lux

  float logLux = raw * longRange / rawRange;
  return pow(10, logLux);
}
