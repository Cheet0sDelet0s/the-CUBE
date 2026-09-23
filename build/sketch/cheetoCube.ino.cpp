#include <Arduino.h>
#line 1 "/home/cheeto/Arduino/cheetoCube/cheetoCube.ino"
#include <FastLED.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string.h>

/* ===================== CONFIG ===================== */

#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

#define LEDS_PER_FACE 25
#define NUM_FACES 6

// Face pins
#define FACE1_PIN 33
#define FACE2_PIN 2
#define FACE3_PIN 13
#define FACE4_PIN 26
#define FACE5_PIN 25
#define FACE6_PIN 32

// Dock detect (top face removed = HIGH via pullup)
#define FACE6_DOCK_PIN 15

// Buttons (external pulldown, HIGH when pressed)
#define LEFT_BUTTON   37
#define RIGHT_BUTTON  38 
#define SELECT_BUTTON 36 

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

/* ===================== LED ARRAYS ===================== */

CRGB face1[LEDS_PER_FACE];
CRGB face2[LEDS_PER_FACE];
CRGB face3[LEDS_PER_FACE];
CRGB face4[LEDS_PER_FACE];
CRGB face5[LEDS_PER_FACE];
CRGB face6[LEDS_PER_FACE];

CRGB* faces[NUM_FACES] = {
  face1, face2, face3, face4, face5, face6
};

CRGB smiley[5][5] = {
  { CRGB::Yellow,    CRGB::Yellow,    CRGB::Yellow, CRGB::Yellow,    CRGB::Yellow },
  { CRGB::Yellow,    CRGB::Black,  CRGB::Yellow, CRGB::Black,  CRGB::Yellow },
  { CRGB::Yellow,  CRGB::Yellow,  CRGB::Yellow,  CRGB::Yellow,  CRGB::Yellow },
  { CRGB::Yellow,    CRGB::Black,  CRGB::Yellow, CRGB::Black,  CRGB::Yellow },
  { CRGB::Yellow,    CRGB::Black,    CRGB::Black, CRGB::Black,    CRGB::Yellow }
};


/* ===================== OLED ===================== */

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

static const unsigned char PROGMEM sun_icon[] = {0x80,0x20,0x4e,0x40,0x1f,0x00,0xdf,0x60,0x1f,0x00,0x4e,0x40,0x80,0x20};

/* ===================== MENU ===================== */

const char* mainMenu[] = {
  "effects",
  "brightness",
  "settings"
};

const char* effectNames[] = {
  "back",
  "solid blue",
  "rainbow",
  "pulse",
  "confetti",
  "theatre chase",
  "smiley face"
};

const uint8_t MAIN_MENU_LENGTH = sizeof(mainMenu) / sizeof(mainMenu[0]);
const uint8_t EFFECTS_MENU_LENGTH = sizeof(effectNames) / sizeof(effectNames[0]);

enum MenuState { MENU_MAIN, MENU_EFFECTS };
MenuState menuState = MENU_MAIN;
uint8_t menuIndex = 0;        // highlighted index within the active menu
uint8_t selectedEffect = 0;   // currently running effect (0..n-1)

/* ===================== STATE ===================== */

bool dockOpen = false;
unsigned long lastButtonTime = 0;
const unsigned long debounceMs = 150;

uint8_t menuScroll = 0;


int brightness = 100;

/* ===================== SETUP ===================== */

#line 103 "/home/cheeto/Arduino/cheetoCube/cheetoCube.ino"
void setup();
#line 128 "/home/cheeto/Arduino/cheetoCube/cheetoCube.ino"
void loop();
#line 145 "/home/cheeto/Arduino/cheetoCube/cheetoCube.ino"
void handleButtons();
#line 186 "/home/cheeto/Arduino/cheetoCube/cheetoCube.ino"
void handleOLED();
#line 263 "/home/cheeto/Arduino/cheetoCube/cheetoCube.ino"
void runEffect(uint8_t effect);
#line 280 "/home/cheeto/Arduino/cheetoCube/cheetoCube.ino"
void solidColor();
#line 284 "/home/cheeto/Arduino/cheetoCube/cheetoCube.ino"
void rainbow();
#line 292 "/home/cheeto/Arduino/cheetoCube/cheetoCube.ino"
void pulse();
#line 303 "/home/cheeto/Arduino/cheetoCube/cheetoCube.ino"
void confetti();
#line 311 "/home/cheeto/Arduino/cheetoCube/cheetoCube.ino"
void theatreChaseRGB();
#line 335 "/home/cheeto/Arduino/cheetoCube/cheetoCube.ino"
void smileyFace();
#line 340 "/home/cheeto/Arduino/cheetoCube/cheetoCube.ino"
template<typename F>void forEachFace(F func);
#line 348 "/home/cheeto/Arduino/cheetoCube/cheetoCube.ino"
void fillAll(const CRGB& color);
#line 354 "/home/cheeto/Arduino/cheetoCube/cheetoCube.ino"
void fadeAll(uint8_t amount);
#line 360 "/home/cheeto/Arduino/cheetoCube/cheetoCube.ino"
uint8_t xyToIndex(uint8_t x, uint8_t y);
#line 364 "/home/cheeto/Arduino/cheetoCube/cheetoCube.ino"
void imageEffect(CRGB image[5][5]);
#line 103 "/home/cheeto/Arduino/cheetoCube/cheetoCube.ino"
void setup() {
  // Buttons
  pinMode(LEFT_BUTTON, INPUT);
  pinMode(RIGHT_BUTTON, INPUT);
  pinMode(SELECT_BUTTON, INPUT);

  // Dock detect
  pinMode(FACE6_DOCK_PIN, INPUT_PULLUP);

  // LEDs
  FastLED.addLeds<LED_TYPE, FACE1_PIN, COLOR_ORDER>(face1, LEDS_PER_FACE);
  FastLED.addLeds<LED_TYPE, FACE2_PIN, COLOR_ORDER>(face2, LEDS_PER_FACE);
  FastLED.addLeds<LED_TYPE, FACE3_PIN, COLOR_ORDER>(face3, LEDS_PER_FACE);
  FastLED.addLeds<LED_TYPE, FACE4_PIN, COLOR_ORDER>(face4, LEDS_PER_FACE);
  FastLED.addLeds<LED_TYPE, FACE5_PIN, COLOR_ORDER>(face5, LEDS_PER_FACE);
  FastLED.addLeds<LED_TYPE, FACE6_PIN, COLOR_ORDER>(face6, LEDS_PER_FACE);
  FastLED.setBrightness(brightness);
  FastLED.clear(true);

  // OLED (init only once)
  Wire.begin();
}

/* ===================== LOOP ===================== */

void loop() {
  dockOpen = digitalRead(FACE6_DOCK_PIN) == HIGH;

  if (dockOpen) {
    handleOLED();
    handleButtons();
  } else {
    display.clearDisplay();
    display.display();
  }

  runEffect(selectedEffect);
  FastLED.show();
}

/* ===================== BUTTON HANDLING ===================== */

void handleButtons() {
  if (millis() - lastButtonTime < debounceMs) return;

  uint8_t menuLength = (menuState == MENU_MAIN) ? MAIN_MENU_LENGTH : EFFECTS_MENU_LENGTH;

  if (digitalRead(LEFT_BUTTON)) {
    menuIndex = (menuIndex == 0) ? menuLength - 1 : menuIndex - 1;
    lastButtonTime = millis();
  }

  if (digitalRead(RIGHT_BUTTON)) {
    menuIndex = (menuIndex + 1) % menuLength;
    lastButtonTime = millis();
  }

  if (!digitalRead(SELECT_BUTTON)) {
    if (menuState == MENU_MAIN) {
      if (strcmp(mainMenu[menuIndex], "effects") == 0) {
        menuState = MENU_EFFECTS;
        menuIndex = 0;
        menuScroll = 0;
      }
      // future: handle brightness/settings
    } else if (menuState == MENU_EFFECTS) {
      if (menuIndex == 0) {
        // "back"
        menuState = MENU_MAIN;
        menuIndex = 0;
        menuScroll = 0;
      } else {
        // select effect (effectNames[1] -> selectedEffect 0)
        selectedEffect = menuIndex - 1;
      }
    }

    lastButtonTime = millis();
  }
}

/* ===================== OLED MENU ===================== */

void handleOLED() {
  static bool oledInit = false;

  if (!oledInit) {
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) return;
    oledInit = true;
  }

  const uint8_t ROW_HEIGHT = 10;
  const uint8_t HEADER_HEIGHT = 16;
  const uint8_t VISIBLE_ROWS = (SCREEN_HEIGHT - HEADER_HEIGHT) / ROW_HEIGHT;

  // Clamp scroll window using the active menu index/length
  uint8_t menuLength = (menuState == MENU_MAIN) ? MAIN_MENU_LENGTH : EFFECTS_MENU_LENGTH;
  if (menuIndex < menuScroll) {
    menuScroll = menuIndex;
  } else if (menuIndex >= menuScroll + VISIBLE_ROWS) {
    menuScroll = menuIndex - VISIBLE_ROWS + 1;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Header
  display.setCursor(0, 0);
  display.println("cheetoCube");
  display.drawRect(115, 0, 12, 7, 1); // battery box
  display.drawLine(127, 2, 127, 4, 1); // battery anode
  display.fillRect(116, 1, 8, 5, 1); // battery fill
  
  // brightness display
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);
  display.setCursor(86, 0);
  uint8_t percent = (brightness * 100) / 255;
  display.print(String(percent) + "%");

  display.drawBitmap(73, 0, sun_icon, 11, 7, 1);
  
  // divider between brightness and battery display
  display.drawRect(111, 0, 2, 7, 1);

  // Draw visible menu entries for the active menu
  for (uint8_t i = 0; i < VISIBLE_ROWS; i++) {
    uint8_t idx = menuScroll + i;
    if (idx >= menuLength) break;

    display.setCursor(0, HEADER_HEIGHT + (i * ROW_HEIGHT));

    if (idx == menuIndex) {
      display.print("> ");
    } else {
      display.print("  ");
    }

    if (menuState == MENU_MAIN) {
      display.println(mainMenu[idx]);
    } else {
      display.println(effectNames[idx]);
    }
  }

  // Scroll indicator
  if (menuScroll > 0) {
    display.fillTriangle(120, 18, 124, 18, 122, 14, SSD1306_WHITE);
  }
  if (menuScroll + VISIBLE_ROWS < menuLength) {
    display.fillTriangle(120, 60, 124, 60, 122, 64, SSD1306_WHITE);
  }

  display.display();
}


/* ===================== EFFECTS ===================== */

void runEffect(uint8_t effect) {
  switch (effect) {
    case 0: solidColor();
      break;
    case 1: rainbow();
      break;
    case 2: pulse();
      break;
    case 3: confetti();
      break;
    case 4: theatreChaseRGB();
      break;
    case 5: smileyFace();
      break;
  }
}

void solidColor() {
  fillAll(CRGB::Blue);
}

void rainbow() {
  static uint8_t hue = 0;
  forEachFace([&](CRGB* face) {
    fill_rainbow(face, LEDS_PER_FACE, hue, 5);
  });
  hue++;
}

void pulse() {
  static uint8_t bri = 0;
  static int8_t dir = 1;
  bri += dir * 4;
  if (bri == 0 || bri == 255) dir = -dir;

  forEachFace([&](CRGB* face) {
    fill_solid(face, LEDS_PER_FACE, CHSV(0, 0, bri));
  });
}

void confetti() {
  fadeAll(20);
  forEachFace([&](CRGB* face) {
    int pos = random16(LEDS_PER_FACE);
    face[pos] += CHSV(random8(), 200, 255);
  });
}

void theatreChaseRGB() {
  static uint8_t offset = 0;
  static uint8_t hue = 0;
  static unsigned long lastUpdate = 0;

  const uint16_t intervalMs = 80;

  if (millis() - lastUpdate < intervalMs) return;
  lastUpdate = millis();

  forEachFace([&](CRGB* face) {
    for (uint8_t i = 0; i < LEDS_PER_FACE; i++) {
      if ((i + offset) % 3 == 0) {
        face[i] = CHSV(hue + (i * 5), 255, 255);
      } else {
        face[i].fadeToBlackBy(200);
      }
    }
  });

  offset = (offset + 1) % 3;
  hue += 4;
}

void smileyFace() {
  imageEffect(smiley);
}

/* ===================== HELPERS ===================== */

template<typename F>
void forEachFace(F func) {
  for (uint8_t i = 0; i < NUM_FACES; i++) {
    func(faces[i]);
  }
}

void fillAll(const CRGB& color) {
  forEachFace([&](CRGB* face) {
    fill_solid(face, LEDS_PER_FACE, color);
  });
}

void fadeAll(uint8_t amount) {
  forEachFace([&](CRGB* face) {
    fadeToBlackBy(face, LEDS_PER_FACE, amount);
  });
}

uint8_t xyToIndex(uint8_t x, uint8_t y) {
  return y * 5 + x;  // 0–24
}

void imageEffect(CRGB image[5][5]) {
  forEachFace([&](CRGB* face) {
    for (uint8_t y = 0; y < 5; y++) {
      for (uint8_t x = 0; x < 5; x++) {
        uint8_t idx = xyToIndex(x, y);
        face[idx] = image[y][x];
      }
    }
  });
}
