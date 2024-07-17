#include <Arduino.h>
#include <EEPROM.h>
#include <FastLED.h>
#include "JC_Button.h"

#define BUTTON_1 2
#define BUTTON_2 3

#define LED_PIN 5
#define NUM_LEDS 126
#define LONG_PRESS_TIME 700 // ms

#define NUMBER_OF_EFFECTS 5

// Instantiations
void ShiftEffect();
void ReadButtons();

// Note: blocking function
void ButtonFeedback();

void Clean();
void Fire();
void PoliceLights();
void Rainbow();
void PureWhite();
void Ocean();

// Glabal variables

enum class LastButtonStatus
{
  RELEASED,
  LONG_PRESS,
};

Button G_BTN1(BUTTON_1, 25, true, false), G_BTN2(BUTTON_2, 25, true, false);
LastButtonStatus G_BTN1_S{LastButtonStatus::RELEASED}, G_BTN2_S{LastButtonStatus::RELEASED};
CRGB G_LEDS[NUM_LEDS];

void (*G_EFFECTS[NUMBER_OF_EFFECTS])() = {
    &PoliceLights,
    &Fire,
    &Rainbow,
    &Ocean,
    *PureWhite};

uint8_t G_CURRENT_EFFECT_IND = 0;
void (*GP_CURRENT_EFFECT)();
bool G_STATE_CHANCHED = true;
uint8_t G_BRIGHTNESS = 255;
bool G_SAVE_REQUIRED = false;

void InitParametersFromEEPROM()
{
  G_CURRENT_EFFECT_IND = EEPROM.read(128);
  G_BRIGHTNESS = EEPROM.read(129);

  G_CURRENT_EFFECT_IND = (G_CURRENT_EFFECT_IND) % NUMBER_OF_EFFECTS;
  GP_CURRENT_EFFECT = G_EFFECTS[G_CURRENT_EFFECT_IND];
}

void SaveParametersToEEPROM()
{
  EEPROM.write(128, G_CURRENT_EFFECT_IND);
  EEPROM.write(129, G_BRIGHTNESS);
}

void setup()
{
  delay(1000);
  G_BTN1.begin();
  G_BTN2.begin();
  FastLED.addLeds<WS2812, LED_PIN, GRB>(G_LEDS, NUM_LEDS);
  FastLED.setMaxPowerInMilliWatts(7500);

  InitParametersFromEEPROM();
  FastLED.setBrightness(G_BRIGHTNESS);

  delay(1000);
}

void loop()
{
  ReadButtons();

  (*GP_CURRENT_EFFECT)();

  FastLED.show();
}

void ShiftEffect()
{
  G_CURRENT_EFFECT_IND = (G_CURRENT_EFFECT_IND + 1) % NUMBER_OF_EFFECTS;
  GP_CURRENT_EFFECT = G_EFFECTS[G_CURRENT_EFFECT_IND];
}

uint8_t SuturationSubtruct(uint8_t i, uint8_t j, uint8_t min_value = 0)
{
  int t = i - j;
  if (t < min_value)
    t = min_value;
  return t;
}

void ReadButtons()
{
  static auto old_time = millis();
  auto new_time = millis();
  constexpr unsigned long update_time{100};
  if (new_time - old_time < update_time)
    return;

  G_BTN1.read();
  G_BTN2.read();

  if (G_BTN1.pressedFor(uint32_t(LONG_PRESS_TIME)))
  {
    G_BRIGHTNESS = SuturationSubtruct(G_BRIGHTNESS, 1, 10);
    FastLED.setBrightness(G_BRIGHTNESS);
    G_BTN1_S = LastButtonStatus::LONG_PRESS;
  }

  if (G_BTN2.pressedFor(uint32_t(LONG_PRESS_TIME)))
  {
    G_BRIGHTNESS = qadd8(G_BRIGHTNESS, 1);
    FastLED.setBrightness(G_BRIGHTNESS);
  }

  if (G_BTN1.wasReleased())
  {
    G_SAVE_REQUIRED = true;
    if (G_BTN1_S != LastButtonStatus::LONG_PRESS)
    {
      ShiftEffect();
      ButtonFeedback();
    }
    else
    {
      G_BTN1_S = LastButtonStatus::RELEASED;
    }
  }

  if (G_BTN2.wasReleased())
  {
    G_SAVE_REQUIRED = true;
    if (G_BTN2_S != LastButtonStatus::LONG_PRESS)
    {
      ButtonFeedback();
    }
    else
    {
      G_BTN2_S = LastButtonStatus::RELEASED;
    }
  }

  if (G_BTN1.releasedFor(5000) && G_BTN2.releasedFor(5000) && G_SAVE_REQUIRED)
  {
    G_SAVE_REQUIRED = false;
    SaveParametersToEEPROM();
    ButtonFeedback();
  }
}

void Clean()
{
  for (uint8_t i = 0; i < NUM_LEDS; ++i)
    G_LEDS[i] = CRGB(0, 0, 0);
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

    if (new_time - old_time >= 10)
    {
      G_LEDS[i] = color;
      G_LEDS[NUM_LEDS - 1 - i] = color;
      FastLED.show();
      ++i;
      old_time = new_time;
      ReadButtons();
    }
  }
}

// Efects functions
#define COOLING 100
#define SPARKING 220

void Fire()
{
  // Array of temperature readings at each simulation cell
  static uint8_t leds_in_column = NUM_LEDS / 2;
  // Serial.println(leds_in_column);
  static uint8_t heat[NUM_LEDS];

  static auto old_time = millis();
  auto new_time = millis();

  if (new_time - old_time < 20)
    return;
  old_time = new_time;

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
    G_LEDS[j] = HeatColor(heat_1[j]);
  for (uint8_t j = 0; j < leds_in_column; j++)
    G_LEDS[NUM_LEDS - 1 - j] = HeatColor(heat_2[j]);
}

void PoliceLights()
{
  static auto old_time = millis();
  auto new_time = millis();
  constexpr unsigned long update_time{30};
  if (new_time - old_time < update_time)
    return;

  for (uint8_t i = NUM_LEDS - 1; i > 0; --i)
  {
    G_LEDS[i] = G_LEDS[i - 1];
  }
  constexpr double period = update_time * (NUM_LEDS / 2) / 1000.;
  constexpr uint16_t bpm = 60 / period;
  constexpr uint8_t lowwer_value{10};
  constexpr uint8_t upper_value{255};
  const uint8_t beat = beatsin8(bpm, lowwer_value, upper_value);
  G_LEDS[0] = CRGB(upper_value - beat, 0, 0);
  G_LEDS[NUM_LEDS / 2] = CRGB(0, 0, beat);

  old_time = new_time;
}

void Rainbow()
{
  constexpr uint8_t thisdelay = 220, deltahue = 10;
  const uint8_t thishue = millis() * (255 - thisdelay) / 255; // To change the rate, add a beat or something to the result. 'thisdelay' must be a fixed value.

  fill_rainbow(G_LEDS, NUM_LEDS, thishue, deltahue); // Use FastLED's fill_rainbow routine.
}

void PureWhite()
{
  if (!G_STATE_CHANCHED)
    return;
  const CRGB white(255, 255, 255);
  fill_solid(G_LEDS, NUM_LEDS, white);
}

void Ocean()
{
}
