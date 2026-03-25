/*******************************************************************************
 *  Libraries
 ******************************************************************************/

#include <Arduino_GFX_Library.h>
#include <string>
#include <Arduino.h>
#define FASTLED_INTERNAL
#include <FastLED.h>
#include <RotaryEncoder.h>
#include <uClock.h>


/*******************************************************************************
 *  ATOMIC shim
 ******************************************************************************/
#ifndef ATOMIC
#define ATOMIC(X) { uint32_t __interrupt_mask = save_and_disable_interrupts(); X; restore_interrupts(__interrupt_mask); }
#endif


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

#define PIN_OUT1 6
#define PIN_OUT2 7
#define PIN_OUT3 8
#define PIN_OUT4 9

// Define the inputs
#define ENC1_S 5
#define ENC1_A 4
#define ENC1_B 3

#define DEBOUNCE_MS 50

#define CHAR_M 8 // Character multiplier
#define CHAR_H 12 // Character height
#define CHAR_W 16 // Character width


#define BOX_H CHAR_M * CHAR_H + 4

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

#define BPM_MAX 240
#define BPM_MIN 10


// Define the array of leds
CRGB leds[NUM_LEDS];


Arduino_DataBus *bus = new Arduino_RPiPicoSPI(PIN_LCD_DC /* DC */, PIN_LCD_CS /* CS */, PIN_CLK0 /* SCK */, PIN_MOSI /* MOSI */, -1 /* MISO */, spi0 /* spi */);  
Arduino_GFX *gfx = new Arduino_ST7789(bus, PIN_LCD_RST, 0 /* rotation */, true /* IPS */,172 /* width */ , 320 /* height */, 34, 0,34,0);

struct ColorTheme {
  uint32_t background = RGB565_BLACK;
  uint32_t line = RGB565_DARKCYAN;
  uint32_t text = RGB565_ORANGERED;
};

ColorTheme theme;


void renderbox(int box, const char *content) {
  int row = box * BOX_H + 4;
  char buf[4];
  int currentCursorX = gfx->getCursorX();
  int currentCursorY = gfx->getCursorY();
  gfx->setCursor(8, row);
  sprintf(buf, "%3s", content);
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


volatile uint_fast8_t steps; 
volatile uint_fast8_t bpm = 120;
void onQN(uint32_t tick){
    steps++;
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
  
  gfx->writeFastHLine(0, BOX_H*2, gfx->width(), theme.line );
  
  gfx->writeFastHLine(0, BOX_H*3, gfx->width(), theme.line );
  gfx->endWrite();
  delay(100);
    renderbox(0,std::to_string(bpm).c_str());

#ifdef RGBLED_PIN
  leds[0] = CRGB::Green;
  FastLED.show();
#endif
digitalWriteFast(PIN_OUT1, LOW);
uClock.start();
}
int i = 1;

void loop() {
  uint_fast8_t currentStep;
  ATOMIC(
    currentStep = steps;
  )
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
    renderbox(0,std::to_string(bpm).c_str());
  }

  renderbox(1, std::to_string(currentStep).c_str());

/*
  if (enc1_armed){
    enc1_armed = false;
    enc1_led = !enc1_led;
    digitalWriteFast(PIN_OUT1, enc1_led);
  }*/
//  delay(1);
}
