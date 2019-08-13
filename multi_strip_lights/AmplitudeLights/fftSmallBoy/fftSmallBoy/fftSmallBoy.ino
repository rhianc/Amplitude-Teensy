#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <math.h>
#include <SD.h>
#include <SerialFlash.h>
#include "FastLED.h"

// Set-Up for audioShield communication
///////////////////////////////////////////////

// Line In
const int lineIn = AUDIO_INPUT_LINEIN;
const float auxInputVolume = 1;

// Mic In
const int micIn = AUDIO_INPUT_MIC;

/*
 * sums both left and right aux input and sends to FFT
 */
AudioInputI2S            i2s2;           
AudioMixer4              mixer;         
AudioAnalyzeFFT1024      fft;     
AudioConnection          patchCord1(i2s2, 0, mixer, 0);
AudioConnection          patchCord2(i2s2, 1, mixer, 1);
AudioConnection          patchCord3(mixer, fft);
AudioControlSGTL5000     audioShield;

// Set-Up for led strip communication
///////////////////////////////////////////////

// Important Constants
#define NUM_LEDS 60           // LEDs per strip
#define BIN_WIDTH 1           // lights with the same frequency assignment

// LED library initiation
#include <OctoWS2811.h>
DMAMEM int displayMemory[NUM_LEDS*6];
int drawingMemory[NUM_LEDS*6];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 leds(NUM_LEDS, displayMemory, drawingMemory, config);

float beat_threshold = .96;
int old_color;

float decay = 0.98;
const int colorRange = 85;
const int startColor = 0;
const int HALF_LEDS = floor(NUM_LEDS/2);
const int NUM_FLEDS = ceil((255./colorRange) * NUM_LEDS);
CHSV hsv_leds[NUM_LEDS];
CHSV fleds[NUM_FLEDS];

// Binning
const int NUM_BINS = floor(NUM_LEDS/(BIN_WIDTH)); //Get the number of bins based on NUM_LEDS and BIN_WIDTH
const int HALF_NUM_BINS = floor(NUM_LEDS/(2*BIN_WIDTH)); //Over two for half wrap

int genFrequencyBinsHorizontal[NUM_BINS];
float genFrequencyLabelsHorizontal[NUM_BINS];

int genFrequencyHalfBinsHorizontal[HALF_NUM_BINS];
float genFrequencyHalfLabelsHorizontal[HALF_NUM_BINS];
float logLevelsEq[HALF_NUM_BINS];

int timer = 0;
int counter = 0;

// for timing purposes
float startTimer = millis();

bool beatDetected;

//----------------------------------------------------------------------
//----------------------------CORE PROGRAM------------------------------
//----------------------------------------------------------------------

int testData = 12345;

// only send data when proper reciever ready message recieved on this end
bool recieverReadyFlag = false;
const int recieverReadyMessage = 0xff;

// Run setup once
void setup() {
  // Enable Serial
  Serial.begin(2000000);
  Serial.println("serial port open");
  
  // Enable the audio shield and set the output volume
  AudioMemory(48);
  audioShield.enable();
  audioShield.inputSelect(micIn);
  audioShield.volume(auxInputVolume);
  audioShield.micGain(60);

  // Start LED interface
  writeFrequencyBinsHorizontal();
  color_spectrum_half_wrap_setup();

  leds.begin();

  delay(100);
}

void loop() {
  if (fft.available()){
    color_spectrum_half_wrap(true);
    beatDetectorUpdate();
    leds.show();
  }
  timer = millis();
  if (timer-startTimer > 100){
    moving_color_spectrum_half_wrap(1);   // modifies color mapping
    startTimer = millis();
  }
}


//-----------------------------------------------------------------------
//----------------------------SETUP FUNCTIONS----------------------------
//-----------------------------------------------------------------------

//Dynamically create frequency bin volume array for NUM_BINS
void writeFrequencyBinsHorizontal(){
  // clean this up and determine it's necessity
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
    if(genFrequencyHalfLabelsHorizontal[i]<4000){
      logLevelsEq[i] = (float(log10(genFrequencyHalfLabelsHorizontal[i])*0.008+85.))/85.;
    }
    else{
      logLevelsEq[i] = (float(-log10(genFrequencyHalfLabelsHorizontal[i])*0.01 + 85.))/85.;
    }
    sum = genFrequencyHalfLabelsHorizontal[i];
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
    allLedsSetPixel(HALF_LEDS - i - 1,fled.r,fled.g,fled.b);
    allLedsSetPixel(HALF_LEDS + i,fled.r,fled.g,fled.b);
    hsv_leds[HALF_LEDS - i - 1] = fleds[i];
    hsv_leds[HALF_LEDS + i] = fleds[i];
  }
}

//-----------------------------------------------------------------------
//----------------------------LOOP FUNCTIONS----------------------------
//-----------------------------------------------------------------------

void color_spectrum_half_wrap(bool useEq){
  unsigned int x, freqBin;
  float level;
  int j_val;
  int j;
  int k;
  int right;
  int left;
  freqBin = 0;
  for (x=0; x < HALF_NUM_BINS; x++) {
      // get the volume for each horizontal pixel position
      level = read_fft(freqBin, freqBin + genFrequencyHalfBinsHorizontal[x] - 1);
      //using equal volume contours to create a liner approximation (lazy fit) and normalizing. took curve for 60Db. labels geerates freq in hz for bin
      //gradient value (0.00875) was calculated but using rlly aggressive 0.06 to account for bassy speaker, mic,  and room IR.Numbers seem way off though...
      if(useEq==true){
        level = (level*logLevelsEq[x]*255)*(10); 
      }
      if(level>255){
        level =255;
      }
      //Serial.println(level);
      right = (HALF_NUM_BINS - x)*BIN_WIDTH;
      left = (HALF_NUM_BINS + x)*BIN_WIDTH;
      //uncomment for full spec
      //level = 255;

      if (level>45) {   // this was 55
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
            if(j_val < 15){
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
      // seems like we have too many arrays doing the same thing, 
      // should definitely clean this up
    }
}


void color_spectrum_half_wrap_update(int index, float level) {
  if(index >= NUM_LEDS || index < 0){
    ; // this condition should have been taken care of elsewhere
  }else if(index >= HALF_LEDS){
    // why is this even an option??
    int f_index = index - HALF_LEDS;
    CRGB fled = CHSV(fleds[f_index].hue,255,level);
    allLedsSetPixel(index,fled.r,fled.g,fled.b);
    hsv_leds[index] = CHSV(fleds[f_index].hue,255,level);
  }else{
    int f_index = abs(index - HALF_LEDS +1);
    CRGB fled = CHSV(fleds[f_index].hue,255,level);
    allLedsSetPixel(index,fled.r,fled.g,fled.b);
    hsv_leds[index] = CHSV(fleds[f_index].hue,255,level);
  }
}

void moving_color_spectrum_half_wrap(int delta){
  if(beatDetected){
    delta = 60;
    beatDetected = false;
  }
  for(int i=0;i<(NUM_FLEDS);i++){
    CHSV fledx = fleds[i];
    fledx.hue = fledx.hue + delta;
    
    fleds[i] = fledx;
  }
}

float read_fft(unsigned int binFirst, unsigned int binLast) {
   if (binFirst > binLast) {
      unsigned int tmp = binLast;
      binLast = binFirst;
      binFirst = tmp;
    }
    if (binFirst > 511) return 0.0;
    if (binLast > 511) binLast = 511;
    uint32_t sum = 0;
    do {
      sum +=1000*fft.read(binFirst++);
      //Serial.println(sum);
    } while (binFirst <= binLast);
    return (float)sum * (1.0 / 16384.0);
  }

float getBassPower(int maxBin){
  float power = 0;
  for (int i = 0; i <= maxBin; i++){
    power += pow(read_fft(0,i),2);
  }
  //Serial.println(power);
  return power;
}

float prevBassPower = 0;

bool beatDetector(){
  // return true if beat detected
  float newBassPower = getBassPower(10);  
  if ((newBassPower - prevBassPower) > beat_threshold){
    // beat detected!
    //Serial.println("beat detected");
    prevBassPower = newBassPower;
    return true;
  }
  else{
    prevBassPower = newBassPower;
    return false;
  }
}

int beatTimer = 16;

void beatDetectorUpdate(){
  if (beatDetector() && beatTimer > 11){
    beatDetected = true;
    beatTimer = 0;
  }
  beatTimer += 1;
}

//-----------------------------------------------------------------------
//----------------------------LED CONTROL----------------------------
//-----------------------------------------------------------------------

void allLedsSetPixel(int i, int r, int g, int b) {
  // iterate through each of 8 strips
  for (int x = 0; x < 8; x++){
    float red;
    float green;
    float blue;
    // not really sure why pureC matters, might change decay type
    bool pureC = true;

    red = r*(1-((8*(1./12)) +((x%4)*(1./11.1))));
    green = g*(1-((8*(1./12)) +((x%4)*(1./11.1))));
    blue = b*(1-((8*(1./12)) +((x%4)*(1./11.1))));

    if((r>0&&g>0)||(r>0&&b>0)||(b>0&&g>0)){
      pureC = false;
    }
    
    if(((red<20)||green<20)||(blue<20)) && (pureC == false)){
      red = 0;
      green = 0;
      blue = 0;
    } 
      
    leds.setPixel(x*NUM_LEDS+i, red, green, blue);
  }
}
