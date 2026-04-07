#include <Arduino.h>
#include <FastLED.h>
#include <driver/i2s.h>
#include <math.h>
#include <Wire.h>

// ================= MPR121 =================
#define MPR121_ADDR 0x5A

void writeRegister(uint8_t reg, uint8_t value)
{
  Wire.beginTransmission(MPR121_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

uint16_t readTouchStatus()
{
  Wire.beginTransmission(MPR121_ADDR);
  Wire.write(0x00);
  Wire.endTransmission(false);
  Wire.requestFrom(MPR121_ADDR, 2);

  uint16_t L = Wire.read();
  uint16_t H = Wire.read();

  return (H << 8) | L;
}

// ================= LED =================
#define LED_PIN 18
#define NUM_LEDS 8
CRGB leds[NUM_LEDS];

// ================= I2S =================
#define I2S_BCLK 14
#define I2S_LRC 15
#define I2S_DOUT 25
#define SAMPLE_RATE 44100

// ================= MELODY =================
float melody[] = {261.63, 293.66, 329.63, 293.66, 261.63, 220.00};
int melodyLength = 6;
int melodyIndex = 0;
unsigned long lastNoteTime = 0;
float currentFreq = melody[0];

// ================= AUDIO =================
#define AUDIO_CHUNK 256
int16_t audioBuffer[AUDIO_CHUNK * 2];

// ================= SETUP I2S =================
void setupI2S()
{
  i2s_config_t config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = 0,
      .dma_buf_count = 8,
      .dma_buf_len = 64};

  i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_BCLK,
      .ws_io_num = I2S_LRC,
      .data_out_num = I2S_DOUT,
      .data_in_num = I2S_PIN_NO_CHANGE};

  i2s_driver_install(I2S_NUM_0, &config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
}

// ================= LED EFFECT =================
void googleHomeEffect()
{
  static uint8_t hue = 0;
  static float brightnessPhase = 0;

  brightnessPhase += 0.05;
  float brightness = (sin(brightnessPhase) + 1.0) / 2.0;

  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CHSV(hue + i * 10, 200, brightness * 150);
  }

  hue++;
  FastLED.show();
}

// ================= MELODY =================
void updateMelody()
{
  if (millis() - lastNoteTime > 400)
  {
    melodyIndex++;
    if (melodyIndex >= melodyLength)
      melodyIndex = 0;
    currentFreq = melody[melodyIndex];
    lastNoteTime = millis();
  }
}

// ================= AUDIO =================
void fillAudioBuffer(float freq)
{
  static float t = 0;
  float dt = 1.0 / SAMPLE_RATE;

  for (int i = 0; i < AUDIO_CHUNK; i++)
  {
    float wave = sin(2 * PI * freq * t) * 0.5;
    int16_t sample = wave * 4000;

    audioBuffer[i * 2] = sample;
    audioBuffer[i * 2 + 1] = sample;

    t += dt;
  }
}

void playAudio()
{
  size_t bytes_written;
  i2s_write(I2S_NUM_0, audioBuffer, sizeof(audioBuffer), &bytes_written, portMAX_DELAY);
}

// ================= SETUP =================
void setup()
{
  Serial.begin(115200);

  Wire.begin(21, 22);
  Wire.setClock(100000);

  // 🔴 Init MPR121
  writeRegister(0x5E, 0x00);
  for (int i = 0; i < 12; i++)
  {
    writeRegister(0x41 + i * 2, 10);
    writeRegister(0x42 + i * 2, 5);
  }
  writeRegister(0x2B, 0x01);
  writeRegister(0x2C, 0x01);
  writeRegister(0x2D, 0x00);
  writeRegister(0x2E, 0x00);
  writeRegister(0x5E, 0x8F);

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();

  setupI2S();

  Serial.println("System ready (MPR121)");
}

// ================= LOOP =================
void loop()
{
  uint16_t touch = readTouchStatus();

  // 🎯 Only electrodes 0,3,7,11
  bool isTouched =
      (touch & (1 << 0)) ||
      (touch & (1 << 3)) ||
      (touch & (1 << 7)) ||
      (touch & (1 << 11));

  Serial.print("Touch mask: ");
  Serial.println(touch, BIN);

  if (isTouched)
  {
    // TOUCHED
    googleHomeEffect();
    updateMelody();
    fillAudioBuffer(currentFreq);
    playAudio();
  }
  else
  {
    // NOT TOUCHED
    FastLED.clear();
    FastLED.show();

    for (int i = 0; i < AUDIO_CHUNK; i++)
    {
      audioBuffer[i * 2] = 0;
      audioBuffer[i * 2 + 1] = 0;
    }
    playAudio();

    delay(20);
  }
}
