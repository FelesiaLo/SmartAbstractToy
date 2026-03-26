#include <Wire.h>
#include <Adafruit_MPR121.h>
#include <FastLED.h>

// ===================== PIN CONFIG =====================
#define SDA_PIN 21
#define SCL_PIN 22

#define LED_PIN 9
#define NUM_LEDS 8

// ===================== OBJECTS =====================
Adafruit_MPR121 cap = Adafruit_MPR121();
CRGB leds[NUM_LEDS];

// ===================== BRIGHTNESS CONTROL =====================
uint8_t brightness = 0;
uint8_t targetBrightness = 0;
const uint8_t MAX_BRIGHTNESS = 120; // safe for babies

// animation
int offset = 0;

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);

  Wire.begin(SDA_PIN, SCL_PIN);

  // detect touch sensors
  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found!");
    while (1);
  }

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(brightness);
}

// ===================== LOOP =====================
void loop() {
  bool isTouched = readTouch();

  // set target brightness
  if (isTouched) {
    targetBrightness = MAX_BRIGHTNESS;
  } else {
    targetBrightness = 0;
  }

  updateBrightness();
  updateLED();

  delay(20); // smooth animation
}

// ===================== TOUCH =====================
bool readTouch() {
  uint16_t touched = cap.touched();

  bool left  = touched & (1 << 0);
  bool right = touched & (1 << 1);
  bool front = touched & (1 << 2);
  bool back  = touched & (1 << 3);

  return (left || right || front || back);
}

// ===================== SMOOTH BRIGHTNESS =====================
void updateBrightness() {
  if (brightness < targetBrightness) {
    brightness++;   // fade IN
  } 
  else if (brightness > targetBrightness) {
    brightness--;   // fade OUT (~5 sec)
  }

  FastLED.setBrightness(brightness);
}

// ===================== LED =====================
void updateLED() {
  offset++;

  // Colors + low saturation
  for (int i = 0; i < NUM_LEDS; i++) {

    uint8_t hue = (i * 20 + offset) % 255;

    leds[i] = CHSV(
      hue,        // rainbow movement
      60,         // low saturation = pastel
      255         // actual brightness controlled globally
    );
  }

  FastLED.show();
}
