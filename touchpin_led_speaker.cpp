#include <FastLED.h>
#include <driver/i2s.h>
#include <math.h>

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

// ================= MELODY =================
float melody[] = {261.63, 293.66, 329.63, 293.66, 261.63, 220.00};
int melodyLength = 6;
int melodyIndex = 0;
unsigned long lastNoteTime = 0;
float currentFreq = melody[0];

// ================= GLOBAL AUDIO BUFFER =================
#define AUDIO_CHUNK 256
int16_t audioBuffer[AUDIO_CHUNK * 2]; // stereo buffer

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
  float brightness = (sin(brightnessPhase) + 1.0) / 2.0; // 0–1 smooth

  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CHSV(hue + i * 10, 200, brightness * 150);
  }

  hue++;
  FastLED.show();
}

// ================= UPDATE MELODY =================
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

// ================= FILL AUDIO BUFFER =================
void fillAudioBuffer(float freq)
{
  static float t = 0;
  float dt = 1.0 / SAMPLE_RATE;

  for (int i = 0; i < AUDIO_CHUNK; i++)
  {
    float wave = sin(2 * PI * freq * t) * 0.5; // low volume
    int16_t sample = wave * 4000;              // baby-safe
    audioBuffer[i * 2] = sample;               // left
    audioBuffer[i * 2 + 1] = sample;           // right
    t += dt;
  }
}

// ================= PLAY AUDIO =================
void playAudio()
{
  size_t bytes_written;
  i2s_write(I2S_NUM_0, audioBuffer, sizeof(audioBuffer), &bytes_written, portMAX_DELAY);
}

// ================= SETUP =================
void setup()
{
  Serial.begin(115200);

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();

  setupI2S();

  Serial.println("System ready. Touch sensor test...");
}

// ================= LOOP =================
void loop()
{
  int touchValue = touchRead(TOUCH_PIN);
  Serial.print("Touch value: ");
  Serial.println(touchValue);

  if (touchValue < THRESHOLD)
  {
    // TOUCHED
    googleHomeEffect(); // LED animation
    updateMelody();     // next note
    fillAudioBuffer(currentFreq);
    playAudio(); // safe, global buffer
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
    playAudio();

    delay(20);
  }
}
