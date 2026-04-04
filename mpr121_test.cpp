#include <Wire.h>
#include <Adafruit_MPR121.h>

Adafruit_MPR121 cap = Adafruit_MPR121();

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA, SCL

  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 NOT found");
    while (1);
  }

  Serial.println("MPR121 detected!");
  cap.setThreshholds(5, 2);
}

void loop() {
  uint16_t touched = cap.touched();

  for (int i = 0; i < 12; i++) {
    if (touched & (1 << i)) {
      Serial.print("Touched: E");
      Serial.println(i);
    }
  }

  delay(100);
}
