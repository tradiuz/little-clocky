/*******************************************************************************
 * Start of Arduino_GFX setting
 *
 * Arduino_GFX try to find the settings depends on selected board in Arduino IDE
 * Or you can define the display dev kit not in the board list
 * Defalult pin list for non display dev kit:
 * Arduino Nano, Micro and more: CS:  9, DC:  8, RST:  7, BL:  6, SCK: 13, MOSI: 11, MISO: 12
 * ESP32 various dev board     : CS:  5, DC: 27, RST: 33, BL: 22, SCK: 18, MOSI: 23, MISO: nil
 * ESP32-C2/3 various dev board: CS:  7, DC:  2, RST:  1, BL:  3, SCK:  4, MOSI:  6, MISO: nil
 * ESP32-C5 various dev board  : CS: 23, DC: 24, RST: 25, BL: 26, SCK: 10, MOSI:  8, MISO: nil
 * ESP32-C6 various dev board  : CS: 18, DC: 22, RST: 23, BL: 15, SCK: 21, MOSI: 19, MISO: nil
 * ESP32-H2 various dev board  : CS:  0, DC: 12, RST:  8, BL: 22, SCK: 10, MOSI: 25, MISO: nil
 * ESP32-P4 various dev board  : CS: 26, DC: 27, RST: 25, BL: 24, SCK: 36, MOSI: 32, MISO: nil
 * ESP32-S2 various dev board  : CS: 34, DC: 38, RST: 33, BL: 21, SCK: 36, MOSI: 35, MISO: nil
 * ESP32-S3 various dev board  : CS: 40, DC: 41, RST: 42, BL: 48, SCK: 36, MOSI: 35, MISO: nil
 * ESP8266 various dev board   : CS: 15, DC:  4, RST:  2, BL:  5, SCK: 14, MOSI: 13, MISO: 12
 * Raspberry Pi Pico dev board : CS: 17, DC: 27, RST: 26, BL: 28, SCK: 18, MOSI: 19, MISO: 16
 * RTL8720 BW16 old patch core : CS: 18, DC: 17, RST:  2, BL: 23, SCK: 19, MOSI: 21, MISO: 20
 * RTL8720_BW16 Official core  : CS:  9, DC:  8, RST:  6, BL:  3, SCK: 10, MOSI: 12, MISO: 11
 * RTL8722 dev board           : CS: 18, DC: 17, RST: 22, BL: 23, SCK: 13, MOSI: 11, MISO: 12
 * RTL8722_mini dev board      : CS: 12, DC: 14, RST: 15, BL: 13, SCK: 11, MOSI:  9, MISO: 10
 * Seeeduino XIAO dev board    : CS:  3, DC:  2, RST:  1, BL:  0, SCK:  8, MOSI: 10, MISO:  9
 * Teensy 4.1 dev board        : CS: 39, DC: 41, RST: 40, BL: 22, SCK: 13, MOSI: 11, MISO: 12
 ******************************************************************************/
#include <Arduino_GFX_Library.h>
#include <string>
#include <Arduino.h>
#include <FastLED.h>
#include <RotaryEncoder.h>
#include <uClock.h>

/*******************************************************************************
 *  Pin definitions
 ******************************************************************************/
#define NUM_LEDS 1

#define PIN_LCD_DC  16
#define PIN_LCD_CS  17
#define PIN_CLK0    18
#define PIN_MOSI    19
#define PIN_LCD_RST 20  
#define GFX_BL      21  // PIN_LCD_BL
#define RGBLED_PIN  22



//#define PIN_MISO 4

#define PIN_OUT1 6
#define PIN_OUT2 7
#define PIN_OUT3 8
#define PIN_OUT4 9

#define CHAR_M 2
#define CHAR_H 12
#define CHAR_W 16

#define BOX_H 16


// Define the inputs

#define ENC1_S 5
#define ENC1_A 4
#define ENC1_B 3

#define DEBOUNCE_MS 50

volatile uint_fast32_t enc1_db;
volatile bool enc1_armed = false;
volatile bool enc1_led= false;

void checkEnc1S(){
  if (!enc1_armed){
    enc1_db = millis();
    enc1_armed = true;
  }

}

RotaryEncoder *enc1 = nullptr;

void checkPosition(){
  enc1->tick();
}

// Pulse stuff

#define PPQN 24
#define PULSE_TIME 12-
#define BPM_MAX 200
#define BPM_MIN 10


// Define the array of leds
CRGB leds[NUM_LEDS];


Arduino_DataBus *bus = new Arduino_RPiPicoSPI(PIN_LCD_DC /* DC */, PIN_LCD_CS /* CS */, PIN_CLK0 /* SCK */, PIN_MOSI /* MOSI */, -1 /* MISO */, spi0 /* spi */);  
Arduino_GFX *gfx = new Arduino_ST7789(bus, PIN_LCD_RST, 1 /* rotation */, true /* IPS */,172 /* width */ , 320 /* height */, 34, 0,34,0);

std::string box1_text = "180 bpm";
std::string box2_text = "24 ppq";
std::string box3_text = "10";
std::string box4_text = "15";

struct ColorTheme {
  uint32_t background = RGB565_BLACK;
  uint32_t line = RGB565_DARKCYAN;
  uint32_t text = RGB565_ORANGERED;
};

ColorTheme theme;


void renderbox(int box, const char *content) {
  int row = (box / 3) * BOX_H;
  char buf[9];
  if (box > 2) { row = gfx->height() - BOX_H; };
  int currentCursorX = gfx->getCursorX();
  int currentCursorY = gfx->getCursorY();
  gfx->setCursor(int(gfx->width() * box / 3), row);
  sprintf(buf, "%8s", content);
  gfx->printf(buf);
  gfx->setCursor(currentCursorX, currentCursorY);
};

volatile bool clock_state=false;
volatile uint_fast8_t slice=0;
void onStepCallback(uint32_t step){
  if (slice >= 4){ slice = 0;
    digitalWriteFast(PIN_OUT1, clock_state );
    clock_state = !clock_state;
  }
  slice++;
}

void onQN(uint32_t tick){
}

void setup() {
  pinMode(ENC1_S, INPUT_PULLUP);
  enc1 = new RotaryEncoder(ENC1_A,ENC1_B, RotaryEncoder::LatchMode::FOUR3);
  attachInterrupt(digitalPinToInterrupt(ENC1_A), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC1_B), checkPosition, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC1_S), checkEnc1S, FALLING);

  uClock.setOutputPPQN(uClock.PPQN_96);
  uClock.setTempo(120.0);
  uClock.init();
  uClock.setOnStep(onStepCallback);
  uClock.setOnSync(uClock.PPQN_1, onQN);

  pinMode(PIN_OUT1, OUTPUT);
  pinMode(PIN_OUT2, OUTPUT);
  pinMode(PIN_OUT3, OUTPUT);
  pinMode(PIN_OUT4, OUTPUT);
  digitalWriteFast(PIN_OUT1, HIGH);
#ifdef DEV_DEVICE_INIT
  DEV_DEVICE_INIT();
#endif
#ifdef RGBLED_PIN
 FastLED.addLeds<WS2812B, RGBLED_PIN, RGB>(leds, NUM_LEDS);

  leds[0] = CRGB::Red;
  FastLED.show();
  delay(500);

#endif

  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  // while(!Serial);
  Serial.println("Arduino_GFX Hello World example");

  // Init Display
  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(RGB565_BLACK);

#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif

  gfx->setTextSize(CHAR_M);

  gfx->setTextColor(theme.text,theme.background);
  gfx->setCursor(0, 0);

  gfx->startWrite();
  gfx->writeFastHLine(0, BOX_H, gfx->width(), theme.line );
  gfx->writeFastHLine(0, gfx->height() - BOX_H, gfx->width(), theme.line );
  gfx->writeFastVLine(int(gfx->width() / 3), 0, BOX_H, theme.line );
  gfx->writeFastVLine(int(gfx->width() * 2/3), 0, BOX_H, theme.line );
  gfx->endWrite();


  renderbox(0, &box1_text[0]);
  renderbox(1, &box2_text[0]);
  renderbox(2, &box3_text[0]);
  renderbox(3, &box4_text[0]);

  delay(100);

#ifdef RGBLED_PIN
  leds[0] = CRGB::Green;
  FastLED.show();
#endif
digitalWriteFast(PIN_OUT1, LOW);
uClock.start();
}
int bpm = 120;
int i = 1;

  static int pos=0;

  std::string bpmtext;
void loop() {
  //enc1->tick();
  uint_fast32_t currentTime = millis();
  RotaryEncoder::Direction dir = enc1->getDirection();
  if (dir != RotaryEncoder::Direction::NOROTATION ) {
    bpm += dir == RotaryEncoder::Direction::CLOCKWISE ? -1 : 1;
    if (bpm > BPM_MAX){
      bpm = BPM_MAX;
    } else if (bpm < BPM_MIN){
      bpm = BPM_MIN;
    }
    uClock.setTempo(bpm);
  }
  renderbox(1, "test");
  bpmtext = std::to_string(bpm);
  renderbox(0, bpmtext.c_str());
  if (enc1_armed){
    enc1_armed = false;
    enc1_led = !enc1_led;
    digitalWriteFast(PIN_OUT1, enc1_led);
  }
//  delay(1);
}
