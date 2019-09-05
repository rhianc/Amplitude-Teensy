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

const int ALPHABET_LEN = 27;
char alphabet[ALPHABET_LEN] = "abcdefghijklmnopqrstuvwxyz ";
//2 digits of OCTAL numbers. need to be broken down into binary. each number in a lettrs array represents a row. 6 binary digits in a row, 8 rows. Each letter is 6 columns of 8 lights, which limits small display to 10 letters.
//                                 A{14,22,41,41,77,41,41,41}B                         C                         D                         E                         F                         G                         H                         I                         J                         K                         L                         M                         N                         O                         P                         Q                         R                         S                         T                         U                         V                         W                         X                         Y                         Z
//int pixel_map[ALPHABET_LEN][8] = {{14,22,41,41,77,41,41,41},{76,41,41,76,76,41,41,76},{77,40,40,40,40,40,40,77},{76,43,43,43,43,43,43,76},{77,40,40,77,77,40,40,77},{77,40,40,77,40,40,40,40},{77,40,40,40,47,41,41,77},{63,63,63,77,77,63,63,63},{77,14,14,14,14,14,14,77},{77,14,14,14,14,54,74,30},{61,62,64,70,70,64,62,61},{60,60,60,60,60,60,77,77},{36,77,55,55,55,55,55,55},{41,61,71,55,55,47,43,41},{36,63,63,63,63,63,63,36},{76,63,63,76,60,60,60,60},{36,63,63,63,73,67,76,75},{77,63,63,77,74,66,63,63},{77,60,60,74,17,03,03,77},{77,77,14,14,14,14,14,14},{63,63,63,63,63,63,77,36},{63,63,22,22,36,36,14,14},{55,55,55,55,55,55,77,36},{63,36,36,14,14,36,36,63},{63,63,36,36,14,14,14,14},{77,03,06,14,14,30,60,77}};
int pixel_map[ALPHABET_LEN][8] = {{00,14,22,41,77,41,41,00},{00,76,41,76,41,41,76,00},{00,77,40,40,40,40,77,00},{00,74,47,43,43,47,74,00},{00,77,40,77,40,40,77,00},{00,77,40,77,40,40,40,00},{00,77,40,40,47,41,77,00},{00,63,63,77,77,63,63,00},{00,77,14,14,14,14,77,00},{00,77,14,14,54,74,30,00},{00,63,66,74,74,66,63,00},{00,60,60,60,60,60,77,00},{00,77,55,55,55,55,55,00},{00,41,61,51,45,43,41,00},{00,36,63,63,63,63,36,00},{00,76,63,76,60,60,60,00},{00,36,63,63,63,67,35,00},{00,77,63,67,74,66,63,00},{00,77,60,74,17,03,77,00},{00,77,14,14,14,14,14,00},{00,63,63,63,63,63,36,00},{00,63,22,22,22,36,14,00},{00,55,55,55,55,55,77,00},{00,63,22,14,14,22,63,00},{00,63,63,36,14,14,14,00},{00,77,06,14,14,30,77,00},{00,00,00,00,00,00,00,00}};
int LetterArray[8][60] = {0};
int placement[8] = {4,5,6,7,3,2,1,0};

int audio_gain = 5000;


// Bluetooth Stuff
String config;
bool lightsOn = false;
String message = "";
int incomingByte;
//----------------------------------------------------------------------
//----------------------------CORE PROGRAM------------------------------
//----------------------------------------------------------------------

// Run setup once
void setup() {
  // Enable Serial
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial.println("serial port open");
  config = "on";
  lightsOn = true;
  Serial1.print("AT+RESET");
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
  fillLetterArray("eat ass");
  delay(100);
}

void loop() {
  checkForMessage();
  if (fft.available()){
    color_spectrum_half_wrap(true);
    //beatDetectorUpdate();
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

//Write lettering into lights
void fillLetterArray(char input[]){
  size_t len = strlen(input);
  for(int i=0;i<len;i++){
    fillLetterArrayHelper(30-(len*4)+(8*i) + 1,input[i]);
  }
}

void fillLetterArrayHelper(int index_start, char letter){
  int row;
  int left;
  int right;
  const char *ptr = strchr(alphabet, letter);
    if(ptr) {
       int index = ptr - alphabet;
       // do something
       for(int j=0;j<8;j++){
        row = pixel_map[index][j];
        left = floor(row/10);
        right = row%10;
        int pixels[6] = {(left/4)%2,(left/2)%2,left%2,(right/4)%2,(right/2)%2,right%2};
        for(int k=0;k<6;k++){
          LetterArray[j][index_start+k] = pixels[k];
        }
       }
    }
}

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

void checkForMessage(){
  if (Serial1.available()){
    incomingByte = Serial1.read();
    message += char(incomingByte);
    }else{
      if (message == "" || message == config){
        //do nothing
     }else{
        Serial.println(message);
        config = message;
        if(config == "on"){
          lightsOn = true;
        }else if(config == "off"){
          allLedsOff();
          lightsOn = false;
        }else{
          fillLetterArray(config);
        }
        message = "";
     }
   }
}

void allLedsOff(){
  for(i=0;i<NUM_LEDS;i++){
    allLedsSetPixel(i,0,0,0);
  }
}

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
      sum +=audio_gain*fft.read(binFirst++);
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
    float white;
    // not really sure why pureC matters, might change decay type
    bool pureC = true;

    red = r*(1-((8*(1./12)) +((x%4)*(1./11.1))));
    green = g*(1-((8*(1./12)) +((x%4)*(1./11.1))));
    blue = b*(1-((8*(1./12)) +((x%4)*(1./11.1))));

    float colors[3] = {red, green, blue};

    if((r>0&&g>0)||(r>0&&b>0)||(b>0&&g>0)){
      pureC = false;
    }
    
    if(((colors[0]<20)||(colors[1]<20)||(colors[2]<20)) && (pureC == false)){
      for(int c;c<3;c++){
        colors[c] = 0;
      }
    }   
    if(LetterArray[placement[x]][i] == 1){
      white = (red +green + blue)/3.;
      leds.setPixel(x*NUM_LEDS+i, 0, 0, 0);
    }else{
      leds.setPixel(x*NUM_LEDS+i, colors[0], colors[1], colors[2]);
    }
  }
}
