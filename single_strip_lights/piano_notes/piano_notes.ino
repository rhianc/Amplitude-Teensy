//Serial LED Light Visualisations
//Inspired by the LED Audio Spectrum Analyzer Display

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include "FastLED.h"
#include <WS2812Serial.h>
#include <math.h>

#define NUM_LEDS 300 // per strip
#define BIN_WIDTH 1 // lights with the same frequency assignment

int state = 0; // used to determine which type of lights we currently want
float beat_threshold = 0.08;
int old_color;
// VARIABLES FROM BEFORE
const unsigned int max_height = 255;
const float maxLevel = 0.5;      // 1.0 = max, lower is more "sensitive"

const float dynamicRange = 60.0; // total range to display, in decibels (what??? power range??)
const float linearBlend = 0.4;   // useful range is 0 to 0.7
// This array holds the volume level (0 to 1.0) for each
// vertical pixel to turn on.  Computed in setup() using
// the 3 parameters above.
float thresholdVertical[max_height];

float decay = 0.9999;
const int colorRange = 80;
const int startColor = 0;
const int HALF_LEDS = floor(NUM_LEDS/2);
const int NUM_FLEDS = 2 * floor(ceil((255./colorRange) * NUM_LEDS) / 2);
CHSV hsv_leds[NUM_LEDS];
CHSV fleds[NUM_FLEDS];

// PINS!
const int AUDIO_INPUT_PIN = A7;        // Input ADC pin for audio data.
const int OUTPUT_PIN = 5;           // Output pin for neo pixels.
// Binning
const int NUM_BINS = floor(NUM_LEDS/(BIN_WIDTH)); //Get the number of bins based on NUM_LEDS and BIN_WIDTH
const int HALF_NUM_BINS = floor(NUM_LEDS/(2*BIN_WIDTH)); //Over two for half wrap

// Audio library objects
AudioInputAnalog         adc1(AUDIO_INPUT_PIN);       //xy=99,55
AudioAnalyzeFFT1024      fft;                         //xy=265,75
AudioConnection          patchCord1(adc1, fft);

//empty array for generating 
int genFrequencyBinsHorizontal[NUM_BINS];
float genFrequencyLabelsHorizontal[NUM_BINS];

int genFrequencyHalfBinsHorizontal[HALF_NUM_BINS];
float genFrequencyHalfLabelsHorizontal[HALF_NUM_BINS];
float logLevelsEq[HALF_NUM_BINS];

int startTimer = 0;
int timer = 0;
int counter = 0;

byte drawingMemory[NUM_LEDS*3];         //  3 bytes per LED
DMAMEM byte displayMemory[NUM_LEDS*12]; // 12 bytes per LED

WS2812Serial leds(NUM_LEDS, displayMemory, drawingMemory, OUTPUT_PIN, WS2812_GRB);

//----------------------------------------------------------------------
//----------------------------CORE PROGRAM------------------------------
//----------------------------------------------------------------------

// Run setup once
void setup() {
  pinMode(15, INPUT); // pin 14 used for button read
  // the audio library needs to be given memory to start working
  AudioMemory(12); // this is probably why the longer FFT wasn't working, didn't add more memory
  writeFrequencyBinsHorizontal();
  //creates spectrum array (fleds) for color reference later. First value is range (0-255) of spectrum to use, second is starting value. Negate range to flip order.
  //Note: (-255,0 will be solid since the starting value input only loops for positive values, all negative values are equiv to 0 so you would want -255,255 for a reverse spectrum)
  
  color_spectrum_half_wrap_setup();
  //color_spectrum_setup(255,0);

  //initialize strip objects
  leds.begin();
}

void loop() {
  // see if need to change state of LEDS to boring or na
  checkButtonChange();
  // state 0 is FFT, state 1 is boring
  if (state == 0){
      //check if Audio processing for that sample frame is compelte
    if (fft.available()) {
        color_spectrum_half_wrap(true);
        leds.show();
    }
    //choose any time based modifiers
    timer = millis();
    if(timer-startTimer > 100){
        moving_color_spectrum_half_wrap();    //modifies color mapping
        startTimer = millis();
    }
  }
  else if (state == 1){
    beatDetectorUpdate();
    leds.show();
  }
  else if (state == 2){
    merge_half_wrap_boring();
    leds.show();
    delay(100);
  }
  
}
//-----------------------------------------------------------------------
//------------------------Button for State Change------------------------
//-----------------------------------------------------------------------
void checkButtonChange(){
  if (digitalRead(15)==LOW){
    state = (state+1)%3;
    if (state == 0){
      writeFrequencyBinsHorizontal();
      color_spectrum_half_wrap_setup();
      leds.show();
    }
    else if (state == 1){
      color_spread(0);
      leds.show();
      
    }
    else{
      color_spectrum_half_wrap_boring();
      leds.show();
    }
    delay(200);
  }
}

//-----------------------------------------------------------------------
//----------------------------SETUP FUNCTIONS----------------------------
//-----------------------------------------------------------------------

//Dynamically create frequency bin volume array for NUM_BINS
void writeFrequencyBinsHorizontal(){
  // why is there a 60 in the exponent, is M_E e? 
  int sum = 0;
  int binFreq = 43; // stated on website
  for (int i=0; i < NUM_BINS; i++){ // used when starting from end of strip
    genFrequencyBinsHorizontal[i] = ceil(60./NUM_BINS*0.7964*pow(M_E,0.0583*(i + 1)*(60./NUM_BINS)));
  }
  for (int i=0; i < HALF_NUM_BINS; i++){ // used when starting from center of strip
    genFrequencyHalfBinsHorizontal[i] = ceil(60./HALF_NUM_BINS*0.7964*pow(M_E,0.0583*(i + 1)*(60./HALF_NUM_BINS)));
  }
  for (int i=0; i<NUM_BINS;i++){ // has the value of the frequency for each bin
    genFrequencyLabelsHorizontal[i] = genFrequencyBinsHorizontal[i]*binFreq + sum;
    sum = genFrequencyLabelsHorizontal[i];
  }
  sum = 0;
  for (int i=0; i<HALF_NUM_BINS;i++){
    genFrequencyHalfLabelsHorizontal[i] = genFrequencyHalfBinsHorizontal[i]*binFreq + sum;
    if(genFrequencyHalfLabelsHorizontal[i]<4500){
      logLevelsEq[i] = (float(log10(genFrequencyHalfLabelsHorizontal[i])*0.01+70.))/85.;
    }
    else{
      logLevelsEq[i] = (float(-log10(genFrequencyHalfLabelsHorizontal[i])*0.01 + 140.))/85.;
    }
    sum = genFrequencyHalfLabelsHorizontal[i];
  }
}

//----------------------------------------------------------------------
//---------------------------RAINBOW VISUALS----------------------------
//----------------------------------------------------------------------

//NON REACTIVE----SPECTRUM BUILDERS

//creates full spectrum from red -> magenta that maps from 0->NUM_LEDS
void color_spectrum_setup(int colorRange, int startColor) {
  for (int i = 0;i < NUM_LEDS; i++){
    float number = i * colorRange;
    float number1 = number / NUM_LEDS;
    float number2 = floor(number1);
    CHSV fled = CHSV((number2 + startColor),255,255);
    CRGB led = fled;
    leds.setPixel(i,led.r,led.g,led.b);
    hsv_leds[i] = fled;
  }
}

//creates full spectrum from center out, input variables determine amount of full spectrum to use (i.e can limit to smaller color ranges)
void color_spectrum_half_wrap_setup() {
  for (int i = 0;i < NUM_FLEDS/2; i++){
    float number = i * 255;
    float number1 = number / (NUM_FLEDS/2);
    float number2 = floor(number1);
    fleds[i] = CHSV((number2 + startColor),255,0);
    fleds[NUM_FLEDS - i - 1] = CHSV((number2 + startColor),255,0);
  }
  for (int i = 0; i< HALF_LEDS; i++){
    CRGB fled = fleds[i];
    leds.setPixel(HALF_LEDS - i - 1,fled.r,fled.g,fled.b);
    leds.setPixel(HALF_LEDS + i,fled.r,fled.g,fled.b);
    hsv_leds[HALF_LEDS - i - 1] = fleds[i];
    hsv_leds[HALF_LEDS + i] = fleds[i];
  }
}

void color_spectrum_half_wrap(bool useEq){
  unsigned int x, freqBin;
  float level;
  int j_val;
  //int k_val;
  int j;
  int k;
  int right;
  int left;
  freqBin = 0;
  for (x=0; x < HALF_NUM_BINS; x++) {
      // get the volume for each horizontal pixel position
      level = fft.read(freqBin, freqBin + genFrequencyHalfBinsHorizontal[x] - 1);
       //using equal volume contours to create a liner approximation (lazy fit) and normalizing. took curve for 60Db. labels geerates freq in hz for bin
      //gradient value (0.00875) was calculated but using rlly aggressive 0.06 to account for bassy speaker, mic,  and room IR.Numbers seem way off though...
      if(useEq==true){
        level = level*logLevelsEq[x]*255*5; 
      }
      
      right = (HALF_NUM_BINS - x)*BIN_WIDTH;
      left = (HALF_NUM_BINS + x)*BIN_WIDTH;
      // uncomment to see the spectrum in Arduino's Serial Monitor
      //Serial.println(level);
      //uncomment for full spec
      //level = 255;

      if (level>55) {
          for(int i=0;i<BIN_WIDTH;i++){
            j = right - i - 1;
            k = left + i;
            color_spectrum_half_wrap_update(j,level);
            color_spectrum_half_wrap_update(k,level);
          }
          
        } else {
          for(int i=0;i<BIN_WIDTH;i++){
            j = right - i - 1;
            k = left + i;
            j_val = hsv_leds[j].val;
            if(j_val < 30){
              color_spectrum_half_wrap_update(j,0);
              color_spectrum_half_wrap_update(k,0);
            }else{
              color_spectrum_half_wrap_update(j,j_val * decay);
              color_spectrum_half_wrap_update(k,j_val * decay);
            }
          }
        }
      // increment the frequency bin count, so we display
      // low to higher frequency from left to right
      freqBin = freqBin + genFrequencyHalfBinsHorizontal[x];
    }
}

void color_spectrum_half_wrap_update(int index, float level) {
  if(index >= NUM_LEDS || index < 0){
    ;
  }else if(index >= HALF_LEDS){
    int f_index = index - HALF_LEDS;
    CRGB fled = CHSV(fleds[f_index].hue,255,level);
    leds.setPixel(index,fled.r,fled.g,fled.b);
    hsv_leds[index] = CHSV(fleds[f_index].hue,255,level);
  }else{
    int f_index = abs(index - HALF_LEDS +1);
    CRGB fled = CHSV(fleds[f_index].hue,255,level);
    leds.setPixel(index,fled.r,fled.g,fled.b);
    hsv_leds[index] = CHSV(fleds[f_index].hue,255,level);
  }
}

void moving_color_spectrum_half_wrap(){
  CHSV fled = fleds[0];
  for(int i=0;i<(NUM_FLEDS-1);i++){
    fleds[i] = fleds[i+1];
  }
  fleds[NUM_FLEDS-1] = fled;
}

//----------------------------------------------------------------------
//-------------------------For Beat Detection---------------------------
//----------------------------------------------------------------------
float getBassPower(int maxBin){
  float power = 0;
  for (int i = 1; i < maxBin + 1; i++){
    power += pow(fft.read(i),2);
  }
  return power;
}

float prevBassPower = 0;

bool beatDetector(){
  // return true if beat detected
  float newBassPower = getBassPower(256);
  //Serial.println(newBassPower-prevBassPower); 
  if ((newBassPower - prevBassPower) > beat_threshold){
    // beat detected!
    prevBassPower = newBassPower;
    return true;
  }
  else{
    prevBassPower = newBassPower;
    return false;
  }
}

int tick = 0;
int beatTimer = 15;

void beatDetectorUpdate(){
  // already checked if new FFT available
  
  if (beatDetector() && beatTimer > 20){
    int new_color = choose_random_color(old_color);
    color_spread(new_color);
    old_color = new_color;
    beatTimer = 0;
  }
  beatTimer += 1;
}

void color_spread(int startColor) {
  CRGB colorRef = CHSV(startColor,255,255);
  for (int i = 0;i < NUM_LEDS; i++){
//    Serial.println(i);
    leds.setPixel(i,colorRef.r,colorRef.g,colorRef.b);
  }
}

int choose_random_color(int old_c){
  int new_color = rand() % (255 + 1 - 0) + 0;
  if (abs(new_color-old_c) < 70){
    return choose_random_color(old_c);
  }
  else{
    return new_color;
  }
}

//----------------------------------------------------------------------
//------------------------------For Ripple------------------------------
//----------------------------------------------------------------------

// need to map frequencies to colors (512 different colors)
// need to input next round of frequencies at beginning 

void timeEvolve() {
  

}

//----------------------------------------------------------------------
//---------------------------For NON-Reactive---------------------------
//----------------------------------------------------------------------

void color_spectrum_half_wrap_boring() {
        for (int i = 0;i < NUM_LEDS; i++){
          float number = i * 255;
          float number1 = number / NUM_LEDS;
          float number2 = floor(number1);
          fleds[i] = CHSV(number2,255,255); 
          fleds[2*NUM_LEDS - i - 1] = CHSV(number2,255,255); 
        }
        for (int i = 0; i< NUM_LEDS/2; i++){
          CRGB fled = fleds[i];
          leds.setPixel(NUM_LEDS/2 - i - 1,fled.r,fled.g,fled.b);
          leds.setPixel(NUM_LEDS/2 + i,fled.r,fled.g,fled.b);
        }
      }

void merge_half_wrap_boring() {
      CHSV jawnski = fleds[0];
      for (int j = 0; j< 2*NUM_LEDS - 1; j++){
          fleds[j] = fleds[j+1];
        }
      fleds[2*NUM_LEDS - 1] = jawnski;
      for (int i = 0; i< NUM_LEDS/2; i++){
          CRGB fled = fleds[i];
          leds.setPixel(NUM_LEDS/2 - i - 1,fled.r,fled.g,fled.b);
          leds.setPixel(NUM_LEDS/2 + i,fled.r,fled.g,fled.b);
        }
    }

void turn_off() {
  for(int i = 0; i<NUM_LEDS; i++){
    leds.setPixel(i,0,0,0);
  }
}
