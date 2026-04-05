#include <FastLED.h>
#include <driver/i2s.h>
#include <SPIFFS.h>
#include <Arduino.h>
#include "WAVFileReader.h"
#include "SinWaveGenerator.h"
#include "I2SOutput.h"

// ================= LED =================
#define LED_PIN 18
#define NUM_LEDS 8
CRGB leds[NUM_LEDS];

// ================= TOUCH =================
#define TOUCH_PIN 4
#define THRESHOLD 30

// ================= I2S =================
#define I2S_BCLK 14
#define I2S_LRC 15
#define I2S_DOUT 22
#define SAMPLE_RATE 44100

// ================= GLOBAL AUDIO BUFFER =================
#define AUDIO_CHUNK 256
int16_t audioBuffer[AUDIO_CHUNK * 2]; // stereo buffer

i2s_pin_config_t i2sPins = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE};

I2SOutput *output;
SampleSource *sampleSource;

// ================= LED EFFECT =================
void googleHomeEffect()
{
  static uint8_t hue = 0;
  static float brightnessPhase = 0;

  brightnessPhase += 0.05;
  float brightness = (sin(brightnessPhase) + 1.0) / 2.0; // 0–1 smooth

  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CHSV(hue + i * 10, 200, brightness * 150);
  }

  hue++;
  FastLED.show();
}

// ================= SETUP =================
void setup()
{
  Serial.begin(115200);

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();


  Serial.println("System ready. Touch sensor test...");
}

// ================= LOOP =================
void loop()
{
  int touchValue = touchRead(TOUCH_PIN);

  if (touchValue < THRESHOLD)
  {
    // TOUCHED
    googleHomeEffect(); // LED animation
    SPIFFS.begin();
    sampleSource = new WAVFileReader("/sample.wav");

    Serial.println("Starting I2S Output");
    output = new I2SOutput();
    output->start(I2S_NUM_1, i2sPins, sampleSource);
  }
  else
  {
    // NOT TOUCHED
    FastLED.clear();
    FastLED.show();

    // send silence
    for (int i = 0; i < AUDIO_CHUNK; i++)
    {
      audioBuffer[i * 2] = 0;
      audioBuffer[i * 2 + 1] = 0;
    }

    delay(20);
  }
}
