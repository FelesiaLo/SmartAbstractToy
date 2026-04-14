#include <Arduino.h>
#include <Wire.h>
#include <SPIFFS.h>
#include <driver/i2s.h>
#include <FastLED.h>

// ================= MPR121 =================
#define SDA_PIN 21
#define SCL_PIN 22
#define MPR121_ADDR 0x5A

uint16_t lastTouched = 0;

// ================= I2S =================
#define I2S_BCLK 14
#define I2S_LRC 15
#define I2S_DOUT 25

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 512

File audioFile;
bool isPlaying = false;
int playCount = 0;
int maxLoops = 1;
const char *currentFile = "";

// ================= LED =================
#define LED_PIN 18
#define NUM_LEDS 24
CRGB leds[NUM_LEDS];

bool ledActive = false;
int currentMode = 0; // 1 = ocean, 2 = calm
unsigned long fadeStart = 0;

// ================= I2C =================
void writeRegister(uint8_t reg, uint8_t value)
{
  Wire.beginTransmission(MPR121_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

uint16_t readRegister16(uint8_t reg)
{
  Wire.beginTransmission(MPR121_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);

  Wire.requestFrom(MPR121_ADDR, 2);
  if (Wire.available() < 2)
    return 0xFFFF;

  uint16_t lsb = Wire.read();
  uint16_t msb = Wire.read();
  return (msb << 8) | lsb;
}

void initMPR121()
{
  writeRegister(0x80, 0x63);
  delay(10);

  writeRegister(0x5E, 0x00);

  for (uint8_t i = 0; i < 12; i++)
  {
    writeRegister(0x41 + i * 2, 6);
    writeRegister(0x42 + i * 2, 3);
  }

  writeRegister(0x5E, 0x8F);
}

// ================= I2S =================
void initI2S()
{
  i2s_config_t config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = 0,
      .dma_buf_count = 8,
      .dma_buf_len = 256,
      .use_apll = false};

  i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_BCLK,
      .ws_io_num = I2S_LRC,
      .data_out_num = I2S_DOUT,
      .data_in_num = I2S_PIN_NO_CHANGE};

  i2s_driver_install(I2S_NUM_0, &config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
}

// ================= AUDIO =================
void startPlaying(const char *path, int loops, int mode)
{
  if (isPlaying)
  {
    audioFile.close();
    isPlaying = false;
  }

  audioFile = SPIFFS.open(path);
  if (!audioFile)
  {
    Serial.println("❌ Failed to open file");
    return;
  }

  audioFile.seek(44);

  currentFile = path;
  playCount = 0;
  maxLoops = loops;
  isPlaying = true;

  // LED start
  currentMode = mode;
  ledActive = true;
  fadeStart = 0;

  Serial.print("▶ Playing: ");
  Serial.println(path);
}

void stopAudioClean()
{
  size_t bytesWritten;
  uint8_t silence[BUFFER_SIZE] = {0};

  for (int i = 0; i < 10; i++)
  {
    i2s_write(I2S_NUM_0, silence, BUFFER_SIZE, &bytesWritten, 0);
  }

  i2s_zero_dma_buffer(I2S_NUM_0);
}

void handleAudio()
{
  if (!isPlaying)
    return;

  static uint8_t buffer[BUFFER_SIZE];

  if (audioFile.available())
  {
    size_t bytesRead = audioFile.read(buffer, BUFFER_SIZE);

    size_t bytesWritten;
    i2s_write(I2S_NUM_0, buffer, bytesRead, &bytesWritten, 0);
  }
  else
  {
    audioFile.close();
    playCount++;

    if (playCount < maxLoops)
    {
      audioFile = SPIFFS.open(currentFile);
      audioFile.seek(44);
    }
    else
    {
      isPlaying = false;
      stopAudioClean();
      Serial.println("✅ Done (silent)");
    }
  }
}

// ================= LED =================
void updateLED()
{
  if (!ledActive)
    return;

  // 🌊 Ocean wave (blue flowing)
  if (currentMode == 1)
  {
    for (int i = 0; i < NUM_LEDS; i++)
    {
      uint8_t wave = sin(i * 10 + millis() * 0.2) * 127 + 128;
      leds[i] = CRGB(0, 0, wave);
    }
  }

  // 🧘 Calm (purple breathing)
  else if (currentMode == 2)
  {
    uint8_t b = (sin(millis() * 0.01) * 127) + 128;
    fill_solid(leds, NUM_LEDS, CRGB(b, 0, b));
  }

  // fade out after sound
  if (!isPlaying)
  {
    if (fadeStart == 0)
      fadeStart = millis();

    float progress = (millis() - fadeStart) / 3000.0;

    if (progress >= 1.0)
    {
      FastLED.clear();
      FastLED.show();
      ledActive = false;
      fadeStart = 0;
      return;
    }

    uint8_t fadeVal = 255 * (1.0 - progress);

    for (int i = 0; i < NUM_LEDS; i++)
    {
      leds[i].nscale8(fadeVal);
    }
  }

  FastLED.show();
}

// ================= SETUP =================
void setup()
{
  Serial.begin(115200);
  delay(1000);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  initMPR121();
  Serial.println("✅ MPR121 ready");

  if (!SPIFFS.begin(true))
  {
    Serial.println("❌ SPIFFS failed");
    while (1)
      ;
  }

  initI2S();

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();
}

// ================= LOOP =================
void loop()
{
  handleAudio();
  updateLED();

  uint16_t touched = 0;

  if (!isPlaying)
  {
    touched = readRegister16(0x00);

    if (touched == 0xFFFF)
    {
      Serial.println("❌ I2C ERROR");
      Wire.begin(SDA_PIN, SCL_PIN);
      return;
    }
  }

  if (!isPlaying && touched && touched != lastTouched)
  {
    // 🌊 Ocean
    if (touched & (1 << 0) || touched & (1 << 3))
    {
      startPlaying("/ocean_waves.wav", 2, 1);
    }
    // 🧘 Calm
    else if ((touched & (1 << 6)) || (touched & (1 << 9)))
    {
      startPlaying("/deep_calm.wav", 2, 2);
    }
  }

  lastTouched = touched;

  delay(2);
}
