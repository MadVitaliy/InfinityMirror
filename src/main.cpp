#include <Arduino.h>
#include <EEPROM.h>
#include <FastLED.h>
#include "JC_Button.h"
#include <LowPower.h>

#define BUTTON_1 2
#define BUTTON_2 3

#define LED_PIN 5
#define NUM_LEDS 124
#define LONG_PRESS_TIME 700 // ms

#define NUMBER_OF_EFFECTS 6

// Instantiations
void NextEffect();
void ReadButtons();

// Note: blocking function
void ButtonFeedback();

void Clean();
void Fire();
void PoliceLights();
void Rainbow();
void ColdWhite();
void WarmWhite();
void Ocean();
void pacifica_one_layer(CRGBPalette16 &p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff);
void pacifica_add_whitecaps();
void pacifica_deepen_colors();
void EasterGame();
void WakeUp();
void WakeUpEffect();
void SleepDownEffect();
void Sleep();

void InitParametersFromEEPROM();
void SaveParametersToEEPROM();

enum class LastButtonStatus
{
  RELEASED,
  LONG_PRESS,
  SLEEP_PRESS,
};

// Glabal variables
Button G_BTN1(BUTTON_1, 25, true, false), G_BTN2(BUTTON_2, 25, true, false);
LastButtonStatus G_BTN1_S{LastButtonStatus::RELEASED}, G_BTN2_S{LastButtonStatus::RELEASED};
CRGB G_LEDS[NUM_LEDS];

void (*G_EFFECTS[NUMBER_OF_EFFECTS])() = {
    &PoliceLights,
    &Fire,
    &Rainbow,
    &Ocean,
    &ColdWhite,
    &WarmWhite};

volatile uint8_t G_CURRENT_EFFECT_IND = 0;
void (*GP_CURRENT_EFFECT)();
volatile bool G_STATE_CHANCHED = true;
volatile uint8_t G_BRIGHTNESS = 255;
volatile bool G_SAVE_REQUIRED = false;

void setup()
{
  delay(100);
  G_BTN1.begin();
  G_BTN2.begin();
  FastLED.addLeds<WS2812, LED_PIN, GRB>(G_LEDS, NUM_LEDS);
  FastLED.setMaxPowerInMilliWatts(7500);
  Clean();
  InitParametersFromEEPROM();
  // GP_CURRENT_EFFECT = &EasterGame;
  FastLED.setBrightness(G_BRIGHTNESS);
  delay(100);
}

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

void loop()
{
  ReadButtons();
  (*GP_CURRENT_EFFECT)();
  FastLED.show();
}

void NextEffect()
{
  G_CURRENT_EFFECT_IND = (G_CURRENT_EFFECT_IND + 1) % NUMBER_OF_EFFECTS;
  GP_CURRENT_EFFECT = G_EFFECTS[G_CURRENT_EFFECT_IND];
}

void PrevEffect()
{
  G_CURRENT_EFFECT_IND = (G_CURRENT_EFFECT_IND + NUMBER_OF_EFFECTS - 1) % NUMBER_OF_EFFECTS;
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
  // auto new_time = millis();
  // static auto old_time = new_time;
  // constexpr unsigned long update_time{100};
  // if (new_time - old_time < update_time)
  //   return;
  // old_time = new_time;

  G_BTN1.read();
  G_BTN2.read();

  if (G_BTN1.pressedFor(uint32_t(3500)))
  {
    G_BTN1_S = LastButtonStatus::SLEEP_PRESS;
    Sleep();
    InitParametersFromEEPROM();
    return;
  }
  else if (G_BTN1.pressedFor(uint32_t(LONG_PRESS_TIME)))
  {
    G_BRIGHTNESS = SuturationSubtruct(G_BRIGHTNESS, 1, 20);
    FastLED.setBrightness(G_BRIGHTNESS);
    G_BTN1_S = LastButtonStatus::LONG_PRESS;
  }

  if (G_BTN2.pressedFor(uint32_t(6000)))
  {
    GP_CURRENT_EFFECT = &EasterGame;

    // FastLED.setMaxPowerInMilliWatts(10000);
  }
  else if (G_BTN2.pressedFor(uint32_t(LONG_PRESS_TIME)))
  {
    G_BRIGHTNESS = qadd8(G_BRIGHTNESS, 1);
    FastLED.setBrightness(G_BRIGHTNESS);
    G_BTN2_S = LastButtonStatus::LONG_PRESS;
  }

  if (G_BTN1.wasReleased())
  {
    G_SAVE_REQUIRED = true;
    if (G_BTN1_S != LastButtonStatus::LONG_PRESS)
    {
      G_STATE_CHANCHED = true;
      PrevEffect();
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
      G_STATE_CHANCHED = true;
      NextEffect();
      ButtonFeedback();
    }
    else
    {
      G_BTN2_S = LastButtonStatus::RELEASED;
    }
  }

  if (G_BTN1.releasedFor(4000) && G_BTN2.releasedFor(4000) && G_SAVE_REQUIRED)
  {
    G_SAVE_REQUIRED = false;
    SaveParametersToEEPROM();
  }
}

void Clean()
{
  fill_solid(G_LEDS, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

void ButtonFeedback()
{
  static uint8_t global_counter = 0;
  ++global_counter;
  uint8_t current_interruption = global_counter;

  static uint8_t shift = 0;
  ++shift;
  const CRGB color(random8(50, 240), random8(50, 240), random8(50, 240));

  Clean();
  auto old_time = millis();
  for (size_t i = 0; i < NUM_LEDS / 2;)
  {
    if (current_interruption != global_counter)
      break;

    auto new_time = millis();

    if (new_time - old_time >= 8)
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

void WakeUp()
{
}

void WakeUpEffect()
{
  Clean();
  for (uint8_t i = 0; i < NUM_LEDS / 2; ++i)
  {
    G_LEDS[i] = G_LEDS[NUM_LEDS - i - 1] = CRGB::WhiteSmoke;
    blur1d(G_LEDS, NUM_LEDS, 3);
    FastLED.show();
    delay(15);
  }
}

void SleepDownEffect()
{
  //fill_solid(G_LEDS, NUM_LEDS, CRGB::WhiteSmoke);
  // FastLED.show();
  constexpr uint8_t half_of_pixels = NUM_LEDS / 2;
  for (uint8_t i = 0; i < half_of_pixels; ++i)
  {
    G_LEDS[half_of_pixels - 1 - i] = G_LEDS[half_of_pixels + i] = CRGB::Black;
    blur1d(G_LEDS, NUM_LEDS, 3);
    FastLED.show();
    delay(15);
  }
}

void Sleep()
{
  SleepDownEffect();

  delay(500);
  static auto pin1 = digitalPinToInterrupt(BUTTON_1);
  attachInterrupt(pin1, WakeUp, RISING);
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  detachInterrupt(pin1);
  delay(500);

  InitParametersFromEEPROM();
  FastLED.setBrightness(G_BRIGHTNESS);
  G_BTN1_S = LastButtonStatus::LONG_PRESS;
  G_STATE_CHANCHED = true;
  WakeUpEffect();
  G_BTN1.begin();
  G_BTN2.begin();
}

// Efects functions
#define COOLING 120
#define SPARKING 80

void Fire()
{
  // Array of temperature readings at each simulation cell
  static uint8_t leds_in_column = NUM_LEDS / 2;
  // Serial.println(leds_in_column);
  static uint8_t heat[NUM_LEDS];

  static auto old_time = millis();
  auto new_time = millis();

  if (new_time - old_time < 30)
    return;
  old_time = new_time;

  uint8_t *heat_1 = heat;
  uint8_t *heat_2 = heat + leds_in_column;

  constexpr uint8_t num_bottom_vertical_leds = 12;
  for (uint8_t i = 0; i < num_bottom_vertical_leds; ++i)
  {
    heat_1[i] = random8(160, 220);
    heat_2[i] = random8(160, 220);
  }

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
    uint8_t y = random8(7) + num_bottom_vertical_leds;
    heat_1[y] = qadd8(heat_1[y], random8(120, 220));
    y = random8(7);
    heat_2[y] = qadd8(heat_2[y], random8(120, 220));
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
  old_time = new_time;

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
}

void Rainbow()
{
  constexpr uint8_t thisdelay = 220, deltahue = 10;
  const uint8_t thishue = millis() * (255 - thisdelay) / 255; // To change the rate, add a beat or something to the result. 'thisdelay' must be a fixed value.

  fill_rainbow(G_LEDS, NUM_LEDS, thishue, deltahue); // Use FastLED's fill_rainbow routine.
}

void ColdWhite()
{
  if (!G_STATE_CHANCHED)
    return;
  G_STATE_CHANCHED = false;
  fill_solid(G_LEDS, NUM_LEDS, CRGB::WhiteSmoke);
}

void WarmWhite()
{
  if (!G_STATE_CHANCHED)
    return;
  G_STATE_CHANCHED = false;
  const CRGB warm_white(252, 113, 25);
  fill_solid(G_LEDS, NUM_LEDS, warm_white);
}

// big complecated effect I have just copied from examples
CRGBPalette16 pacifica_palette_1 =
    {0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117,
     0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x14554B, 0x28AA50};
CRGBPalette16 pacifica_palette_2 =
    {0x000507, 0x000409, 0x00030B, 0x00030D, 0x000210, 0x000212, 0x000114, 0x000117,
     0x000019, 0x00001C, 0x000026, 0x000031, 0x00003B, 0x000046, 0x0C5F52, 0x19BE5F};
CRGBPalette16 pacifica_palette_3 =
    {0x000208, 0x00030E, 0x000514, 0x00061A, 0x000820, 0x000927, 0x000B2D, 0x000C33,
     0x000E39, 0x001040, 0x001450, 0x001860, 0x001C70, 0x002080, 0x1040BF, 0x2060FF};

void Ocean()
{
  // workaround; minimal brightness seems not enough for this effect
  // change once when switch to it
  if (G_STATE_CHANCHED)
  {
    G_BRIGHTNESS = SuturationSubtruct(G_BRIGHTNESS, 1, 70);
    FastLED.setBrightness(G_BRIGHTNESS);
    G_STATE_CHANCHED = false;
  }

  static auto old_time = millis();
  auto new_time = millis();
  constexpr unsigned long update_time{30};
  if (new_time - old_time < update_time)
    return;
  old_time = new_time;
  // Increment the four "color index start" counters, one for each wave layer.
  // Each is incremented at a different speed, and the speeds vary over time.
  static uint16_t sCIStart1, sCIStart2, sCIStart3, sCIStart4;
  static uint32_t sLastms = 0;
  uint32_t ms = GET_MILLIS();
  uint32_t deltams = ms - sLastms;
  sLastms = ms;
  uint16_t speedfactor1 = beatsin16(3, 179, 269);
  uint16_t speedfactor2 = beatsin16(4, 179, 269);
  uint32_t deltams1 = (deltams * speedfactor1) / 256;
  uint32_t deltams2 = (deltams * speedfactor2) / 256;
  uint32_t deltams21 = (deltams1 + deltams2) / 2;
  sCIStart1 += (deltams1 * beatsin88(1011, 10, 13));
  sCIStart2 -= (deltams21 * beatsin88(777, 8, 11));
  sCIStart3 -= (deltams1 * beatsin88(501, 5, 7));
  sCIStart4 -= (deltams2 * beatsin88(257, 4, 6));

  // Clear out the LED array to a dim background blue-green
  fill_solid(G_LEDS, NUM_LEDS, CRGB(2, 6, 10));

  // Render each of four layers, with different scales and speeds, that vary over time
  pacifica_one_layer(pacifica_palette_1, sCIStart1, beatsin16(3, 11 * 256, 14 * 256), beatsin8(10, 70, 130), 0 - beat16(301));
  pacifica_one_layer(pacifica_palette_2, sCIStart2, beatsin16(4, 6 * 256, 9 * 256), beatsin8(17, 40, 80), beat16(401));
  pacifica_one_layer(pacifica_palette_3, sCIStart3, 6 * 256, beatsin8(9, 10, 38), 0 - beat16(503));
  pacifica_one_layer(pacifica_palette_3, sCIStart4, 5 * 256, beatsin8(8, 10, 28), beat16(601));

  // Add brighter 'whitecaps' where the waves lines up more
  pacifica_add_whitecaps();

  // Deepen the blues and greens a bit
  pacifica_deepen_colors();
}

// Add one layer of waves into the led array
void pacifica_one_layer(CRGBPalette16 &p, uint16_t cistart, uint16_t wavescale, uint8_t bri, uint16_t ioff)
{
  uint16_t ci = cistart;
  uint16_t waveangle = ioff;
  uint16_t wavescale_half = (wavescale / 2) + 20;
  for (uint16_t i = 0; i < NUM_LEDS; i++)
  {
    waveangle += 250;
    uint16_t s16 = sin16(waveangle) + 32768;
    uint16_t cs = scale16(s16, wavescale_half) + wavescale_half;
    ci += cs;
    uint16_t sindex16 = sin16(ci) + 32768;
    uint8_t sindex8 = scale16(sindex16, 240);
    CRGB c = ColorFromPalette(p, sindex8, bri, LINEARBLEND);
    G_LEDS[i] += c;
  }
}

// Add extra 'white' to areas where the four layers of light have lined up brightly
void pacifica_add_whitecaps()
{
  uint8_t basethreshold = beatsin8(9, 55, 65);
  uint8_t wave = beat8(7);

  for (uint16_t i = 0; i < NUM_LEDS; i++)
  {
    uint8_t threshold = scale8(sin8(wave), 20) + basethreshold;
    wave += 7;
    uint8_t l = G_LEDS[i].getAverageLight();
    if (l > threshold)
    {
      uint8_t overage = l - threshold;
      uint8_t overage2 = qadd8(overage, overage);
      G_LEDS[i] += CRGB(overage, overage2, qadd8(overage2, overage2));
    }
  }
}

// Deepen the blues and greens
void pacifica_deepen_colors()
{
  for (uint16_t i = 0; i < NUM_LEDS; i++)
  {
    G_LEDS[i].blue = scale8(G_LEDS[i].blue, 145);
    G_LEDS[i].green = scale8(G_LEDS[i].green, 200);
    G_LEDS[i] |= CRGB(2, 5, 7);
  }
}

struct Pair
{
  uint8_t beg;
  uint8_t end;
};

uint8_t TrainLength(const Pair i_train)
{
  return i_train.end - i_train.beg;
}

Pair DefinePlatformPosition(const uint8_t i_train_size)
{
  Pair platform{(NUM_LEDS - i_train_size) / 2, i_train_size};
  platform.end += platform.beg;
  return platform;
}

bool IsTrainOnPlatform(const Pair train, const Pair platform)
{
  return train.end > platform.beg && train.beg < platform.end;
}

Pair TrainOnPlatform(const Pair train, const Pair platform)
{
  if (!IsTrainOnPlatform(train, platform))
    return {0, 0};
  return {max(train.beg, platform.beg), min(train.end, platform.end)};
}

void BlinkLostPixels(Pair firsts, Pair seconds)
{
  constexpr uint16_t del = 50;
  for (size_t k = 0; k < 10; k++)
  {
    for (uint8_t i = firsts.beg; i < firsts.end; ++i)
      G_LEDS[i] = CRGB::DarkRed;
    for (uint8_t i = seconds.beg; i < seconds.end; ++i)
      G_LEDS[i] = CRGB::DarkRed;
    FastLED.show();
    delay(del);

    for (uint8_t i = firsts.beg; i < firsts.end; ++i)
      G_LEDS[i] = CRGB::Black;
    for (uint8_t i = seconds.beg; i < seconds.end; ++i)
      G_LEDS[i] = CRGB::Black;
    FastLED.show();
    delay(del);
  }
}

void BlinkTrain(Pair firsts)
{
  constexpr uint16_t del = 50;
  for (size_t k = 0; k < 10; k++)
  {
    for (uint8_t i = firsts.beg; i < firsts.end; ++i)
      G_LEDS[i] = CRGB::DarkGreen;
    FastLED.show();
    delay(del);

    for (uint8_t i = firsts.beg; i < firsts.end; ++i)
      G_LEDS[i] = CRGB::Black;
    FastLED.show();
    delay(del);
  }
}

void ShowScore(const float i_score)
{
  const CRGB colors[2] = {CRGB::DarkBlue, CRGB::OrangeRed};
  const uint16_t score = static_cast<uint16_t>(i_score);
  const uint8_t loops = score / NUM_LEDS;
  const uint8_t last_score = score % NUM_LEDS;
  CRGB current_collor = colors[0];

  uint16_t del = 50;
  delay(1000);

  for (uint8_t l = 0; l < loops; l++)
  {
    for (uint16_t i = 0; i < NUM_LEDS; i++)
    {
      G_LEDS[i] = current_collor;
      FastLED.show();
      delay(del);
    }
    current_collor = colors[(l + 1u) % 2];
  }

  for (uint16_t i = 0; i < last_score; i++)
  {
    if (last_score - i == 10)
      del = 75;
    else if (last_score - i == 5)
      del = 125;
    else if (last_score - i == 2)
      del = 200;
    G_LEDS[i] = current_collor;
    FastLED.show();
    delay(del);
  }
  delay(3000);
  Clean();
}

void EasterGame()
{
  bool was_user_input = true;
  bool game_over = false;
  Pair train{0, 10};
  Pair platform = DefinePlatformPosition(TrainLength(train));
  int8_t direction = 1;
  constexpr float train_speed = 6; // leds per second
  float speed_scale = 1.f;
  float speed_increase = 0.6f;
  uint16_t frame_time = 1000 / train_speed;
  float score = 0.f;
  constexpr uint16_t button_downtime = 800;

  static auto old_time = millis();
  static auto last_button_pres_time = old_time;
  while (!game_over)
  {

    const auto new_time = millis();
    if (new_time - old_time > frame_time)
    {
      old_time = new_time;

      // bounce from the ends of the led strip
      if (train.beg == 0)
      {
        direction = 1; // move forward when begin has been readhed
        if (!was_user_input)
        {
          // if the user has misses platform twice
          // and he train has returned to begin, the train looses a carrage
          train.end -= 1;
          auto new_train_length = TrainLength(train);
          platform = DefinePlatformPosition(new_train_length);
          if (new_train_length == 0)
          {
            game_over = true;
            break;
          }
        }
        was_user_input = false;
      }
      if (train.end == NUM_LEDS)
        direction = -1; // move backward when end has been readhed

      train.beg += direction;
      train.end += direction;

      // draw background
      fill_solid(G_LEDS, NUM_LEDS, CRGB(30, 30, 30));
      // draw platform
      for (uint8_t k = platform.beg; k < platform.end; ++k)
        G_LEDS[k] = CRGB::OrangeRed;
      // draw train
      for (uint8_t i = train.beg; i < train.end; ++i)
        G_LEDS[i] = CRGB::DarkBlue;

      FastLED.show();
    }

    G_BTN1.read();

    if (G_BTN1.wasPressed() && new_time - last_button_pres_time > button_downtime)
    {
      was_user_input = true;
      if (IsTrainOnPlatform(train, platform))
      {
        Pair lost_pixels_1 = {min(train.beg, platform.beg), max(train.beg, platform.beg)};
        Pair lost_pixels_2 = {min(train.end, platform.end), max(train.end, platform.end)};
        if (TrainLength(lost_pixels_1) || TrainLength(lost_pixels_2))
          BlinkLostPixels(lost_pixels_1, lost_pixels_2);
        else
          BlinkTrain(train);

        train = TrainOnPlatform(train, platform);
        auto new_train_length = TrainLength(train);
        platform = DefinePlatformPosition(new_train_length);
        score += (speed_scale * new_train_length) / 1.3;

        speed_scale += speed_increase;
        frame_time = 1000 / (train_speed * speed_scale);
      }
      else
      {
        BlinkLostPixels(train, platform);
        game_over = true;
      }
      last_button_pres_time = millis();
    }
  }
  Clean();
  if (score > 0.)
  {
    ShowScore(score);
    delay(2000);
  }
  GP_CURRENT_EFFECT = G_EFFECTS[G_CURRENT_EFFECT_IND];
  G_BTN1.begin();
  G_BTN2.begin();
}
