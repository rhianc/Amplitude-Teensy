//Serial LED Light Visualisations
//Inspired by the LED Audio Spectrum Analyzer Display

#include <Audio.h>
#include <Wire.h>
#include <SD.h>
#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include "FastLED.h"
#include <math.h>

#define NUM_LEDS 200
#define BIN_WIDTH 4 

// MATRIX VARIABLES FROM BEFORE
const unsigned int matrix_width = 60;
const unsigned int matrix_height = 32;
const unsigned int max_height = 255;
const float maxLevel = 0.5;      // 1.0 = max, lower is more "sensitive"
const float dynamicRange = 60.0; // total range to display, in decibels
const float linearBlend = 0.4;   // useful range is 0 to 0.7
// This array holds the volume level (0 to 1.0) for each
// vertical pixel to turn on.  Computed in setup() using
// the 3 parameters above.
float thresholdVertical[max_height];


float decay = 0.95;
int HALF_LEDS = floor(NUM_LEDS/2);
CRGB leds[NUM_LEDS];
CHSV fleds[NUM_LEDS / 2];
//PINS!
const int AUDIO_INPUT_PIN = 7;        // Input ADC pin for audio data.
const int POWER_LED_PIN = 13; 
//Adafruit Neopixel object
const int NEO_PIXEL_PIN = 12;           // Output pin for neo pixels.
const int NUM_BINS = floor(NUM_LEDS/(2*BIN_WIDTH)); //Over two for half wrap

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUM_LEDS, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Audio library objects
AudioInputAnalog         adc1(AUDIO_INPUT_PIN);       //xy=99,55
AudioAnalyzeFFT1024      fft;            //xy=265,75
AudioConnection          patchCord1(adc1, fft);


// This array specifies how many of the FFT frequency bin
// to use for each horizontal pixel.  Because humans hear
// in octaves and FFT bins are linear, the low frequencies
// use a small number of bins, higher frequencies use more.

//This is the original array, curve = 0.7964*e^(0.0583x)
//int frequencyBinsHorizontal[60] = {
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

//empty array for generating 
int genFrequencyBinsHorizontal[NUM_BINS];

//Array with some clipping to highest frequencies
int frequencyBinsHorizontal[60] = {
   1,  1,  1,  1,  1,
   1,  1,  1,  1,  1,
   2,  2,  2,  2,  2,
   2,  2,  2,  2,  3,
   3,  3,  3,  3,  4,
   4,  4,  4,  4,  5,
   5,  5,  6,  6,  6,
   7,  7,  7,  8,  8,
   9,  9, 10, 10, 11,
   12, 12, 13, 14, 15,
   15, 15, 15, 15, 15,
   15, 16, 17, 18, 20
};

//----------------------------------------------------------------------
//----------------------------CORE PROGRAM------------------------------
//----------------------------------------------------------------------


// Run setup once
void setup() {
  // the audio library needs to be given memory to start working
  AudioMemory(12);
  Serial.begin(9600);
  Serial.println("setup");
  writeFrequencyBinsHorizontal();
  //compute the vertical thresholds before starting. These are used for intensity graphing of frequencies
  computeVerticalLevels();
  //creates spectrum color array for use later
  color_spectrum_half_wrap_setup();

  FastLED.addLeds<NEOPIXEL, NEO_PIXEL_PIN>(leds, NUM_LEDS);

// turn on the strip
  FastLED.show();
}



void loop() {

  if (fft.available()) {
    //choose visuals
    color_spectrum_half_wrap();
    // after all pixels set, show them all at the same instant
    FastLED.show();

  }
}

//-----------------------------------------------------------------------
//----------------------------SETUP FUNCTIONS----------------------------
//-----------------------------------------------------------------------
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

//Dynamically create frequency bin volume array for NUM_BINS
void writeFrequencyBinsHorizontal(){
  for (int i=0; i < NUM_BINS; i++){
    genFrequencyBinsHorizontal[i] = ceil(0.7964*pow(M_E,0.0583*i));
    Serial.println(genFrequencyBinsHorizontal[i]);
  }
}
//-----------------------------------------------------------------------
//----------------------------COLOR MANAGEMENT---------------------------
//-----------------------------------------------------------------------



//Color conversions for Adafruit from 32bit to three rgb values
void unPackRGB(uint32_t color, uint32_t &r, uint32_t &g,  uint32_t &b){
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

//----------------------------------------------------------------------
//---------------------------RAINBOW VISUALS----------------------------
//----------------------------------------------------------------------



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

void color_spectrum_half_wrap_setup() {
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

void color_spectrum_half_wrap(){
  unsigned int x, freqBin;
  float level;
  freqBin = 0;
  for (x=0; x < NUM_BINS; x++) {
      // get the volume for each horizontal pixel position
      level = fft.read(freqBin, freqBin + frequencyBinsHorizontal[x] - 1);
      int right = NUM_BINS - x;
      int left = NUM_BINS + x;
      // uncomment to see the spectrum in Arduino's Serial Monitor
      //Serial.println(level);
      
      if (level>0.065) {
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
        
      // increment the frequency bin count, so we display
      // low to higher frequency from left to right
      freqBin = freqBin + frequencyBinsHorizontal[x];
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

//----------------------------------------------------------------------
//-------------------POTENTIALLY USEFUL LEFTOVERS-----------------------
//----------------------------------------------------------------------



//calculating peak levels for matrix, using later for ripples
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
      /*
// OctoWS2811 objects
const int ledsPerPin = matrix_width * matrix_height / 8;
DMAMEM int displayMemory[ledsPerPin*6];
int drawingMemory[ledsPerPin*6];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 leds(ledsPerPin, displayMemory, drawingMemory, config);
*/
