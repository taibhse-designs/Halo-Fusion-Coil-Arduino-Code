#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

#define color_change_button_pin 2

#define pattern_change_button_pin 3


#define LED_PIN 6
#define LED_COUNT 22

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);

// -------------------- EEPROM ADDRESSES --------------------

#define EEPROM_COLOR_ADDR 0
#define EEPROM_MODE_ADDR 1

// -------------------- COLOR STRUCT --------------------

struct ColorOption {
  const char* command;
  const char* name;
  uint8_t colors[3][4];
};

struct animationPattern {
  const char* command;
  const char* name;
  const int pattern;
};

//colors are coded as Darkest colour, mid colour, Lightest colour
ColorOption colorOptions[] = {
  { "red", "Red scheme", { { 128, 0, 0, 0 }, { 255, 26, 26, 0 }, { 255, 179, 179, 0 } } },
  { "green", "Green scheme", { { 0, 77, 13, 0 }, { 0, 230, 38, 0 }, { 128, 255, 149, 0 } } },
  { "blue", "Blue scheme", { { 0, 55, 128, 0 }, { 26, 125, 255, 0 }, { 179, 212, 255, 0 } } },
  { "yellow", "Yellow scheme", { { 230, 198, 0, 0 }, { 255, 231, 77, 0 }, { 255, 245, 179, 0 } } },
  { "purple", "Purple scheme", { { 98, 0, 179, 0 }, { 151, 26, 255, 0 }, { 197, 128, 255, 0 } } },
  { "orange", "Orange scheme", { { 117, 63, 3, 0 }, { 188, 108, 5, 0 }, { 238, 192, 0, 0 } } },
};

animationPattern animationPatterns[] = {

  { "plasmashimmer", "Plasma Shimmer Animation Effect", 0 },
  { "heatfire", "Heat Fire Animation Effect", 1 },
  { "noiseflame", "Noise Flame Animation Effect", 2 },
  { "waveflame", "Wave Flame Animation Effect", 3 }
};

const int COLOR_COUNT = sizeof(colorOptions) / sizeof(colorOptions[0]);

const int ANIMATION_PATTERN_COUNT = sizeof(animationPatterns) / sizeof(animationPatterns[0]);

// -------------------- GLOBALS --------------------

String serialCommand = "";

int currentColor = 0;
int animationMode = 0;

#define MIN_BRIGHTNESS 25
#define MAX_BRIGHTNESS 255

unsigned long lastUpdate = 0;
const int updateInterval = 30;

uint8_t heat[LED_COUNT];

// -------------------- SETUP --------------------

void setup() {

  strip.begin();
  strip.show();
  Serial.begin(115200);

  // ---- LOAD EEPROM SETTINGS ----
  int savedColor = EEPROM.read(EEPROM_COLOR_ADDR);
  int savedMode = EEPROM.read(EEPROM_MODE_ADDR);

  if (savedColor >= 0 && savedColor < COLOR_COUNT)
    currentColor = savedColor;

  if (savedMode >= 0 && savedMode < ANIMATION_PATTERN_COUNT)
    animationMode = savedMode;


  pinMode(color_change_button_pin, INPUT_PULLUP);
  pinMode(pattern_change_button_pin, INPUT_PULLUP);

  Serial.println("Restored Settings:");
  Serial.print("Color: ");
  Serial.println(colorOptions[currentColor].name);
  Serial.print("Mode: ");
  Serial.println(animationPatterns[animationMode].name);

  printCommands();
}

// -------------------- LOOP --------------------

void loop() {

  int button_state_color_change = digitalRead(color_change_button_pin); //get state of color change button

  int button_state_pattern_change = digitalRead(pattern_change_button_pin); //get state of pattern change button


  if (Serial.available()) {
    serialCommand = Serial.readStringUntil('\n');
    serialCommand.trim();

    if (serialCommand.equalsIgnoreCase("next-color")) {

      currentColor = (currentColor + 1) % COLOR_COUNT;
      saveColorToEEPROM();
      Serial.println(colorOptions[currentColor].name);

    } else if (serialCommand.equalsIgnoreCase("next-pattern")) {

      animationMode = (animationMode + 1) % ANIMATION_PATTERN_COUNT;
      saveModeToEEPROM();
      Serial.print("Animation mode: ");
      Serial.println(animationPatterns[animationMode].name);

    } else {

      processCommand(serialCommand);

    }
  }

  //check button states
  if(button_state_color_change == LOW)
  {
      Serial.println("Change Color Button Pressed");

      //loop to prevent actions until button released
      do {
          Serial.println("Waiting for Change Color Button to be Released...");
          
          delay(100);
      } while (digitalRead(color_change_button_pin) == LOW);
      
      Serial.println("Change Color Button Release");

      currentColor = (currentColor + 1) % COLOR_COUNT;
      saveColorToEEPROM();

      Serial.print("Colour Changed to ");
      Serial.print(colorOptions[currentColor].name);
      Serial.println("");

      delay(150);

  }

  //check button states
  if(button_state_pattern_change == LOW)
  {
      Serial.println("Change Pattern Button Pressed");
      
      //loop to prevent actions until button released
      do {
          Serial.println("Waiting for Change Pattern Button to be Released...");
          
          delay(100);
      } while (digitalRead(pattern_change_button_pin) == LOW);
      
      Serial.println("Change Pattern Button Release");
      animationMode = (animationMode + 1) % ANIMATION_PATTERN_COUNT;
      saveModeToEEPROM();

      Serial.print("Animation Pattern Changed to ");
      Serial.print(animationPatterns[animationMode].name);
      Serial.println("");

      delay(150);
  }

  //update code for animation effect
  if (millis() - lastUpdate > updateInterval) {

    lastUpdate = millis();
    
    //test to check which annimation effect is active to know which one to update
    switch (animationMode) {
      case 0: plasmaShimmer(); break;
      case 1: heatFire(); break;
      case 2: noiseFlame(); break;
      case 3: waveFlame(); break;
    }
  }
}

// -------------------- EEPROM SAVE --------------------

void saveColorToEEPROM() {
  if (EEPROM.read(EEPROM_COLOR_ADDR) != currentColor)
    EEPROM.update(EEPROM_COLOR_ADDR, currentColor);
}

void saveModeToEEPROM() {
  if (EEPROM.read(EEPROM_MODE_ADDR) != animationMode)
    EEPROM.update(EEPROM_MODE_ADDR, animationMode);
}

// -------------------- COMMAND PROCESS --------------------

void processCommand(String cmd) {

  boolean commandProcessed = false;
  for (int i = 0; i < COLOR_COUNT; i++) {
    if (cmd.equalsIgnoreCase(colorOptions[i].command)) {
      currentColor = i;
      saveColorToEEPROM();
      Serial.println(colorOptions[i].name);

      commandProcessed = true;
      return;
    }
  }

  for (int i = 0; i < ANIMATION_PATTERN_COUNT; i++) {
    if (cmd.equalsIgnoreCase(animationPatterns[i].command)) {
      animationMode = animationPatterns[i].pattern;
      saveModeToEEPROM();
      Serial.println(animationPatterns[i].name);

      commandProcessed = true;
      return;
    }
  }

  if(commandProcessed == false)
  {
    Serial.print(cmd);
    Serial.print(": This command does not exist...");
    Serial.println();
  }
}

void printCommands() {
  Serial.println("---------------------------------------------------------------------------------------------");
  Serial.println("Available Serial Commands:");
  Serial.println("next-color -> next color");
  Serial.println("next-pattern -> next animation mode");
  for (int i = 0; i < COLOR_COUNT; i++) {
    Serial.print(colorOptions[i].command);
    Serial.print(" -> Sets the LED Colors to ");
    Serial.print(colorOptions[i].name);
    Serial.println();
  }

  for (int i = 0; i < ANIMATION_PATTERN_COUNT; i++) {
    Serial.print(animationPatterns[i].command);
    Serial.print(" -> Sets the Animation Pattern to ");
    Serial.print(animationPatterns[i].name);
    Serial.println();
  }
  Serial.println("---------------------------------------------------------------------------------------------");
}

// -------------------- ANIMATIONS --------------------

// 0 - Plasma shimmer animation effect
void plasmaShimmer() {

  for (int i = 0; i < LED_COUNT; i++) {

    float flicker = random(85, 160) / 100.0;
    uint8_t* base = colorOptions[currentColor].colors[random(0, 3)];

    uint8_t r = constrain(base[0] * flicker, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
    uint8_t g = constrain(base[1] * flicker, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
    uint8_t b = constrain(base[2] * flicker, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
    uint8_t w = constrain(base[3] * flicker, MIN_BRIGHTNESS, MAX_BRIGHTNESS);

    strip.setPixelColor(i, strip.Color(r, g, b, w));
  }
  strip.show();
}

// 1 - Heat diffusion fire
void heatFire() {

  for (int i = 0; i < LED_COUNT; i++)
    heat[i] = qsub8(heat[i], random(0, 20));

  for (int k = LED_COUNT - 1; k >= 2; k--)
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;

  if (random(255) < 120)
    heat[random(7)] = random(160, 255);

  for (int j = 0; j < LED_COUNT; j++) {

    uint8_t level = heat[j];
    uint8_t* dark = colorOptions[currentColor].colors[0];
    uint8_t* light = colorOptions[currentColor].colors[2];

    uint8_t r = map(level, 0, 255, dark[0], light[0]);
    uint8_t g = map(level, 0, 255, dark[1], light[1]);
    uint8_t b = map(level, 0, 255, dark[2], light[2]);
    uint8_t w = map(level, 0, 255, dark[3], light[3]);

    r = max(r, MIN_BRIGHTNESS);
    g = max(g, MIN_BRIGHTNESS);
    b = max(b, MIN_BRIGHTNESS);

    strip.setPixelColor(j, strip.Color(r, g, b, w));
  }
  strip.show();
}

// 2 - Flowing noise flame animation effect
void noiseFlame() {

  static uint16_t t = 0;
  t += 20;

  for (int i = 0; i < LED_COUNT; i++) {

    float noise = (sin((i * 15 + t) * 0.01) + 1) / 2;
    uint8_t* base = colorOptions[currentColor].colors[1];

    uint8_t r = constrain(base[0] * noise * 1.3, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
    uint8_t g = constrain(base[1] * noise * 1.3, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
    uint8_t b = constrain(base[2] * noise * 1.3, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
    uint8_t w = constrain(base[3] * noise * 1.3, MIN_BRIGHTNESS, MAX_BRIGHTNESS);

    strip.setPixelColor(i, strip.Color(r, g, b, w));
  }
  strip.show();
}

// 3 - Moving wave flame animation effect
void waveFlame() {

  static uint16_t offset = 0;
  offset += 3;

  for (int i = 0; i < LED_COUNT; i++) {

    float wave = (sin((i * 10 + offset) * 0.02) + 1) / 2;
    uint8_t* dark = colorOptions[currentColor].colors[0];
    uint8_t* light = colorOptions[currentColor].colors[2];

    uint8_t r = map(wave * 255, 0, 255, dark[0], light[0]);
    uint8_t g = map(wave * 255, 0, 255, dark[1], light[1]);
    uint8_t b = map(wave * 255, 0, 255, dark[2], light[2]);
    uint8_t w = map(wave * 255, 0, 255, dark[3], light[3]);

    r = max(r, MIN_BRIGHTNESS);
    g = max(g, MIN_BRIGHTNESS);
    b = max(b, MIN_BRIGHTNESS);

    strip.setPixelColor(i, strip.Color(r, g, b, w));
  }
  strip.show();
}

// -------------------- HELPERS --------------------

uint8_t qsub8(uint8_t i, uint8_t j) {
  int t = i - j;
  if (t < 0) return 0;
  return t;
}


