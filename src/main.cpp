#include <Arduino.h>

#include <FastLED.h>
#include "JC_Button.h"

#define BUTTON_1 2
#define BUTTON_2 3

#define LED_PIN 5
#define NUM_LEDS 126

// Instantiations
void Clean();
// Note: blocking function
void ButtonFeedback();

// Efects functions
#define COOLING 100
#define SPARKING 220
void Fire();
void PoliceLights();
void Ocean();

// Glabal variables
CRGB leds[NUM_LEDS];
Button btn1(BUTTON_1), btn2(BUTTON_2);
void (*current_effect)() = &Fire;
void setup()
{
  // Serial.begin(9600);
  // attachInterrupt(digitalPinToInterrupt(BUTTON_1), ButtonFeedback, RISING);
  // attachInterrupt(digitalPinToInterrupt(BUTTON_2), ButtonFeedback, RISING);
  btn1.begin();
  btn2.begin();
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setMaxPowerInMilliWatts(7500);

  delay(1000);
}

void ShiftEffect()
{
  static void (*effects[3])() = {
      &Fire,
      &PoliceLights,
      &Ocean};
  static uint8_t i = 0;
  i = (i + 1) % sizeof(effects);
  current_effect = effects[i];
}

void ReadBottons()
{
  btn1.read();
  btn2.read();
  if (btn1.wasPressed())
    ButtonFeedback();
  if (btn2.wasPressed())
  {
    ButtonFeedback();
    ShiftEffect();
  }
}

void loop()
{
  ReadBottons();

  (*current_effect)();
  FastLED.show();
}

void Clean()
{
  for (uint8_t i = 0; i < NUM_LEDS; ++i)
    leds[i] = CRGB(0, 0, 0);
  FastLED.show();
}

void ButtonFeedback()
{
  static uint8_t global_counter = 0;
  ++global_counter;
  uint8_t current_interruption = global_counter;

  static uint8_t shift = 0;
  ++shift;
  const CRGB color(80 * (shift + 0) % 3, 30 * (shift + 3) % 7, 30 * (shift + 5) % 8);
  uint8_t i = 0;

  Clean();
  auto old_time = millis();
  while (i < NUM_LEDS / 2)
  {
    if (current_interruption != global_counter)
      break;

    auto new_time = millis();

    if (new_time - old_time < 40)
      return;
    leds[i] = color;
    leds[NUM_LEDS - 1 - i] = color;
    FastLED.show();
    ++i;
    old_time = new_time;
    
    ReadBottons();
  }
}

void Fire()
{
  // Array of temperature readings at each simulation cell
  static uint8_t leds_in_column = NUM_LEDS / 2;
  // Serial.println(leds_in_column);
  static uint8_t heat[NUM_LEDS];

  static auto old_time = millis();
  auto new_time = millis();

  if (new_time - old_time < 100)
    return;

  uint8_t *heat_1 = heat;
  uint8_t *heat_2 = heat + leds_in_column;

  // Step 1.  Cool down every cell a little
  for (uint8_t i = 0; i < NUM_LEDS; i++)
    heat[i] = qsub8(heat[i], random8(0, ((COOLING * 10) / NUM_LEDS) + 2));

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for (uint8_t k = leds_in_column - 1; k >= 2; k--)
  {
    heat_1[k] = (heat_1[k - 1] + heat_1[k - 2] + heat_1[k - 2]) / 3;
    heat_2[k] = (heat_2[k - 1] + heat_2[k - 2] + heat_2[k - 2]) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if (random8() < SPARKING)
  {
    uint8_t y = random8(7);
    heat_1[y] = qadd8(heat_1[y], random8(160, 255));
    y = random8(7);
    heat_2[y] = qadd8(heat_2[y], random8(160, 255));
  }

  // Step 4.  Map from heat cells to LED colors
  for (uint8_t j = 0; j < leds_in_column; j++)
    leds[j] = HeatColor(heat_1[j]);
  for (uint8_t j = 0; j < leds_in_column; j++)
    leds[NUM_LEDS - 1 - j] = HeatColor(heat_2[j]);
}

void PoliceLights()
{
  static auto old_time = millis();
  static bool rename_me = true;
  auto new_time = millis();
  if (new_time - old_time < 1000)
    return;

  CRGB c1 = rename_me ? CRGB::Red : CRGB::Blue;
  CRGB c2 = !rename_me ? CRGB::Red : CRGB::Blue;

  for (uint8_t i = 0; i < NUM_LEDS / 2; i++)
  {
    leds[i] = c1;
    leds[NUM_LEDS - 1 - i] = c2;
  }
  old_time = new_time;
  rename_me = !rename_me;
}

void Ocean() {}
