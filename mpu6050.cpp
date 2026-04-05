#include <Wire.h>
#include <Arduino.h>

#define MPU_ADDR 0x68

void setup()
{
  Serial.begin(115200);
  Wire.begin(21, 22);

  // Wake up MPU6050
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); // power register
  Wire.write(0);    // wake up
  Wire.endTransmission(true);

  Serial.println("MPU6050 Wake OK");
}

void loop()
{
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // accel register
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);

  int16_t ax = Wire.read() << 8 | Wire.read();
  int16_t ay = Wire.read() << 8 | Wire.read();
  int16_t az = Wire.read() << 8 | Wire.read();

  Serial.print("Accel: ");
  Serial.print(ax);
  Serial.print(", ");
  Serial.print(ay);
  Serial.print(", ");
  Serial.println(az);

  delay(300);
}
