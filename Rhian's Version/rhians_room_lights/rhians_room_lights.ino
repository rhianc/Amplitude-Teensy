// Serially Adressable LED Music/Game Visualization
// Simplification of previous version to make more modular and understandable

// Necessary Libraries
#include <Audio.h>
#include <WS2812Serial.h>
#include "FastLED.h" // used for CRGB and CHSV data types

// Permamenant Declarations
#define audioInputPin  14
#define ledsPerStrip  300
#define numLEDstrips  2

#define RED    0xFF0000

const int ledPins[numLEDstrips] = {1,5};

// Initialize Teensy Audio Library Objects
AudioInputAnalog         adc1(audioInputPin);       
AudioAnalyzeFFT1024      fft;                         
AudioConnection          patchCord1(adc1, fft);

// Initialize WS2812Serial LED Strip objects
byte drawingMemory0[ledsPerStrip*3];         //  3 bytes per LED
DMAMEM byte displayMemory0[ledsPerStrip*12]; // 12 bytes per LED

byte drawingMemory1[ledsPerStrip*3];         //  3 bytes per LED
DMAMEM byte displayMemory1[ledsPerStrip*12]; // 12 bytes per LED

WS2812Serial leds0(ledsPerStrip, displayMemory0, drawingMemory0, ledPins[0], WS2812_GRB);
WS2812Serial leds1(ledsPerStrip, displayMemory1, drawingMemory1, ledPins[1], WS2812_GRB);

// initialize LED array memory
CRGB leds[ledsPerStrip];

// useful color references
CRGB none(0,0,0);

void setup() {
  Serial.begin(9600); // USB Serial Connection for debugging
  // Audio connections require memory to work.
  AudioMemory(12);
  // start the WS2812Serial LED objects
  leds0.begin();
  leds1.begin();
  turn_off();
  leds0.setPixel(1, RED);
  leds1.setPixel(1, RED);
  leds1.show();
  leds0.show();
  //Serial.println(ledPins);
  //Serial.println(ledPins[0]);
}

void loop() {
//  if (beatDetector()) {
//    //Serial.println("beat detected");
//    int newColor = chooseNewRandomColor();
//    CRGB newtype(0,255,255);
//    newtype.setHue(newColor);
//    ledSetColorAll(newtype);
//  }
}

//----------------------------------------------------------------------
//-------------------------Universal Functions--------------------------
//----------------------------------------------------------------------

// Turns off all lights
void turn_off() {                                           
  for (int i = 0;i < ledsPerStrip; i++){
    ledSetColor(i, none);
  }
  //leds.show();
}

// When LED strips are symmetric, changes color for all led strips at once
void ledSetColor(int ledPosition, CRGB color){
   leds0.setPixel(ledPosition, color);
   leds1.setPixel(ledPosition, color);
   leds0.show();
   leds1.show();
}

// When LED strips are symmetric, changes color for all leds on all strips at once
void ledSetColorAll(CRGB color){
  for (int i = 0; i < ledsPerStrip; i++){
    leds0.setPixel(i, color);
    leds1.setPixel(i, color);
  }
  leds0.show();
  leds1.show();
}

int prevRandomHue = 0;

int chooseNewRandomColor(){
  int newHue = random(0, 256);
  while (abs(newHue - prevRandomHue) < 40){
  newHue = random(0, 256);
  Serial.println("finding new color");
  }
  prevRandomHue = newHue;
  Serial.println(newHue);
  return newHue;
}

//----------------------------------------------------------------------
//---------------------------Beat Detection-----------------------------
//----------------------------------------------------------------------
float beatThreshold = 0.045; // minimum derivative of bass noise to detect beat
int maxBassCutoffBin = 10;   // maxmimum frequency bin to look for beat power in

// finds total power in frequency sprectrum beneath maxBin frequency
float getBassPower(int maxBin){
  float power = 0;
  for (int i = 1; i < maxBin + 1; i++){
    power += pow(fft.read(i),2);
  }
  return power;
}

// holds value of previously calculated beat power for comparison 
float prevBassPower = 0;

// detects if theres a siginicant increase in bass power
bool beatDetector(){
  // return true if beat detected
  float newBassPower = getBassPower(maxBassCutoffBin);
  //Serial.println(newBassPower);
  if (newBassPower - prevBassPower > beatThreshold){
    // beat detected!
    Serial.println("Beat detected");
    prevBassPower = newBassPower;
    return true;
  }
  else{
    prevBassPower = newBassPower;
    return false;
  }
}


