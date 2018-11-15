// LED Audio Spectrum Analyzer Display
// 
// Creates an impressive LED light show to music input
//   using Teensy 3.1 with the OctoWS2811 adaptor board
//   http://www.pjrc.com/store/teensy31.html
//   http://www.pjrc.com/store/octo28_adaptor.html
//
// Line Level Audio Input connects to analog pin A3
//   Recommended input circuit:
//   http://www.pjrc.com/teensy/gui/?info=AudioInputAnalog


#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include "FastLED.h"
// The display size and color to use
const unsigned int matrix_width = 60;
//const unsigned int matrix_height = 32;
const unsigned int max_height = 255;

// These parameters adjust the vertical thresholds
const float maxLevel = 0.5;      // 1.0 = max, lower is more "sensitive"
const float dynamicRange = 40.0; // total range to display, in decibels
const float linearBlend = 0.3;   // useful range is 0 to 0.7
#define NUM_LEDS 150
#define BIN_WIDTH 3
float decay = 0.95;
int HALF_LEDS = floor(NUM_LEDS/2);
CRGB leds[NUM_LEDS];
CHSV fleds[NUM_LEDS / 2];

/*
// OctoWS2811 objects
const int ledsPerPin = matrix_width * matrix_height / 8;
DMAMEM int displayMemory[ledsPerPin*6];
int drawingMemory[ledsPerPin*6];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 leds(ledsPerPin, displayMemory, drawingMemory, config);
*/

//PINS!
const int AUDIO_INPUT_PIN = 7;        // Input ADC pin for audio data.
const int POWER_LED_PIN = 13; 

//Adafruit Neopixel object
const int NEO_PIXEL_PIN = 12;           // Output pin for neo pixels.
const int NUM_BINS = floor(NUM_LEDS/(2*BIN_WIDTH));         // Number of neo pixels.  You should be able to increase this without
                                       // any other changes to the program.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_LEDS, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Audio library objects
AudioInputAnalog         adc1(AUDIO_INPUT_PIN);       //xy=99,55
AudioAnalyzeFFT1024      fft;            //xy=265,75
AudioConnection          patchCord1(adc1, fft);


// This array holds the volume level (0 to 1.0) for each
// vertical pixel to turn on.  Computed in setup() using
// the 3 parameters above.
float thresholdVertical[max_height];

// This array specifies how many of the FFT frequency bin
// to use for each horizontal pixel.  Because humans hear
// in octaves and FFT bins are linear, the low frequencies
// use a small number of bins, higher frequencies use more.
//int frequencyBinsHorizontal[NUM_BINS] = {
//   1,  1,  1,  1,  1,
//   1,  1,  1,  1,  1,
//   2,  2,  2,  2,  2,
//   2,  2,  2,  2,  3,
//   3,  3,  3,  3,  4,
//   4,  4,  4,  4,  5,
//   5,  5,  6,  6,  6,
//   7,  7,  7,  8,  8,
//   9,  9, 10, 10, 11,
//   12, 12, 13, 14, 15,
//   15, 16, 17, 18, 19,
//   20, 22, 23, 24, 25
//};

int frequencyBinsHorizontal[25] = {
   2,  2,  2,  2,  2,
   2,  2,  2,  2,  2,
   4,  4,  4,  4,  4,
   4,  4,  4,  4,  6,
   6,  6,  6,  6,  8,
};



// Run setup once
void setup() {
  // the audio library needs to be given memory to start working
  AudioMemory(12);
  Serial.begin(9600);
  Serial.println("setup");
//  writeFrequencyBinsHorizontal();
  // compute the vertical thresholds before starting
  computeVerticalLevels();
  color_spectrum_half_wrap();
  FastLED.addLeds<NEOPIXEL, NEO_PIXEL_PIN>(leds, NUM_LEDS);
  // turn on the display
//  pixels.begin();
//  pixels.show();
  FastLED.show();
}

// A simple xy() function to turn display matrix coordinates
// into the index numbers OctoWS2811 requires.  If your LEDs
// are arranged differently, edit this code...
/*
unsigned int xy(unsigned int x, unsigned int y) {
  if ((y & 1) == 0) {
    // even numbered rows (0, 2, 4...) are left to right
    return y * matrix_width + x;
  } else {
    // odd numbered rows (1, 3, 5...) are right to left
    return y * matrix_width + matrix_width - 1 - x;
  }
}
*/
// Run repetitively
void loop() {
  unsigned int x, y, freqBin;
  float level;

  if (fft.available()) {
    // freqBin counts which FFT frequency data has been used,
    // starting at low frequency
    freqBin = 0;
    for (x=0; x < NUM_BINS; x++) {
      // get the volume for each horizontal pixel position
      level = fft.read(freqBin, freqBin + frequencyBinsHorizontal[x] - 1);
      int right = NUM_BINS - x;
      int left = NUM_BINS + x;
      // uncomment to see the spectrum in Arduino's Serial Monitor
      Serial.println(level);
      if (level>0.075) {
          for(int i=0;i<BIN_WIDTH;i++){
            int j = BIN_WIDTH*right - i - 1;
            int k = BIN_WIDTH*left + i;
            color_spectrum_half_wrap_update(j,level);
            color_spectrum_half_wrap_update(k,level);
          }

          
        } else {
          for(int i=0;i<BIN_WIDTH;i++){
            int j = BIN_WIDTH*right - i - 1;
            int k = BIN_WIDTH*left + i;
            leds[j] = CRGB(leds[j].r *decay,leds[j].g *decay,leds[j].b *decay);
            leds[k] = CRGB(leds[k].r *decay,leds[k].g *decay,leds[k].b *decay);
          }
          
         
        }
        /*
      for (y=0; y < max_height; y++) {
        // for each vertical pixel, check if above the threshold
        // and turn the LED on or off
        if (level >= thresholdVertical[y]) {
          pixels.setPixelColor(x,level*pixels.Color(myColor);
        } else {
          pixels.setPixelColor(x,pixels.Color(0,0,0) );
        }
      }*/
      // increment the frequency bin count, so we display
      // low to higher frequency from left to right
      freqBin = freqBin + frequencyBinsHorizontal[x];
    }
    // after all pixels set, show them all at the same instant
    FastLED.show();
//    pixels.show();
    // Serial.println();
  }
}


// Run once from setup, the compute the vertical levels
void computeVerticalLevels() {
  unsigned int y;
  float n, logLevel, linearLevel;

  for (y=0; y < max_height; y++) {
    n = (float)y / (float)(max_height - 1);
    logLevel = pow10f(n * -1.0 * (dynamicRange / 20.0));
    linearLevel = 1.0 - n;
    linearLevel = linearLevel * linearBlend;
    logLevel = logLevel * (1.0 - linearBlend);
    thresholdVertical[y] = (logLevel + linearLevel) * maxLevel;
  }
}

void writeFrequencyBinsHorizontal(){
  for (int i=0; i < NUM_BINS; i++){
    frequencyBinsHorizontal[i] = 0;
  }
}

void unPackRGB(uint32_t color, int &r, int &g,  int &b){
  r = (color >>16)&0xff;
  g = (color >> 8)&0xff;
  b = (color)&0xff;
}
uint8_t getRedValueFromColor(uint32_t c) {
    return (c >> 16)&0xff;
}

uint8_t getGreenValueFromColor(uint32_t c) {
    return (c >> 8)&0xff;
}

uint8_t getBlueValueFromColor(uint32_t c) {
    return c&0xff;
}

void color_spectrum() {
  for (int i = 0;i < NUM_LEDS; i++){
    float number = i * 255;
    float number1 = number / NUM_LEDS;
    float number2 = floor(number1);
    leds[i] = CHSV(number2,255,0);
  }
}

void color_spectrum_update(int index, float level) {
  float number = index * 255;
  float number1 = number / NUM_LEDS;
  float number2 = floor(number1);
  leds[index] = CHSV(number2,255,255*level*5);
}

void color_spectrum_half_wrap() {
  for (int i = 0;i < HALF_LEDS; i++){
    float number = i * 255;
    float number1 = number / HALF_LEDS;
    float number2 = floor(number1);
    fleds[i] = CHSV(number2,255,255);
  }
  for (int i = 0; i< HALF_LEDS; i++){
    leds[HALF_LEDS - i - 1] = fleds[i];
    leds[HALF_LEDS + i] = fleds[i];
  }
}

void color_spectrum_half_wrap_update(int index, float level) {
  if(index >= NUM_LEDS || index < 0){
    ;
  }else if(index >= HALF_LEDS){
    int f_index = index - HALF_LEDS;
    CHSV fled = fleds[f_index];
    leds[index] = CHSV(fled.hue,255,255*level*5);
  }else{
    int f_index = abs(index - HALF_LEDS +1);
    CHSV fled = fleds[f_index];
    leds[index] = CHSV(fled.hue,255,255*level*5);
  }
}
