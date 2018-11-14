#include "FastLED.h"
#include "kiss_fft.h"


// How many leds in your strip?
#define NUM_LEDS 150
#define serialYes true

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN

#include <math.h>       /* floor */

#define MIC_PIN 7
#define DATA_PIN 12
#define CLOCK_PIN 13

#define RAND_MAX 255

// What fraction of spectrum do u want to show at one time
#define fraction_of_spectrum 15
#define white CHSV(0,0,255)
#define red CRGB(255 ,0,0)
#define orange CHSV(10, 255, 255)
#define yellow CRGB(255, 245, 0)
#define green CRGB(0, 255, 0)
#define blue CHSV(160, 255, 255)
#define purple CHSV(180, 255, 255)
#define pink CHSV(250, 255, 255) 


// Define the array of leds
CRGB fleds[NUM_LEDS * fraction_of_spectrum];
CRGB rgb_leds[255 * 3];
CRGB leds[NUM_LEDS];
int color_number = 0;

int random_int(int min, int max) 
{
   return min + rand() % (max - min);
}

class LightStarter{
    public: void color_spectrum() {
        for (int i = 0;i < NUM_LEDS; i++){
          float number = i * 255;
          float number1 = number / NUM_LEDS;
          float number2 = floor(number1);
          leds[i] = CHSV(number2,255,255);
        }
      }
      
    public: void color_spectrum_wrap() {
        for (int i = 0;i < NUM_LEDS; i++){
          float number = i * 255;
          float number1 = number / NUM_LEDS;
          float number2 = floor(number1);
          leds[i] = CHSV(number2,255,255);
          fleds[i] = CHSV(number2,255,255);
          fleds[2*NUM_LEDS - i - 1] = CHSV(number2,255,255);
        }
      }
    
    public: void color_spectrum_half_wrap() {
        for (int i = 0;i < NUM_LEDS; i++){
          float number = i * 255;
          float number1 = number / NUM_LEDS;
          float number2 = floor(number1);
          fleds[i] = CHSV(number2,255,255);
          fleds[2*NUM_LEDS - i - 1] = CHSV(number2,255,255);
        }
        for (int i = 0; i< NUM_LEDS/2; i++){
          leds[NUM_LEDS/2 - i - 1] = fleds[i];
          leds[NUM_LEDS/2 + i] = fleds[i];
        }
      }
    
    public: void color_partial_spectrum(int color_start) {
      for (int i = 0;i < NUM_LEDS/10;i++){
          leds[5*i+1] = CHSV(color_start+2*i,255,255);
          leds[5*i+2] = CHSV(color_start+2*i,255,255);
          leds[5*i+3] = CHSV(color_start+2*i+1,255,255);
          leds[5*i+4] = CHSV(color_start+2*i+1,255,255);
          leds[5*i] = CHSV(color_start+2*i+1,255,255);
          leds[149 - 5*i] = CHSV(color_start+2*i,255,255);
          leds[148 - 5*i] = CHSV(color_start+2*i,255,255);
          leds[147 - 5*i] = CHSV(color_start+2*i,255,255);
          leds[146 - 5*i] = CHSV(color_start+2*i+1,255,255);
          leds[145 - 5*i] = CHSV(color_start+2*i+1,255,255);
      }
    }
    
    public: void rgb_spectrum() {
        int color[3] = {255,0,0};
        
        for (int i = 0;i < 255; i++){
          color[0] = 255 - i;
          color[1] = i;
          color[2] = 0;
          rgb_leds[i] = CRGB(color[0],color[1],color[2]);
        }
        for (int i = 0;i < 255; i++){
          color[0] = 255 - i;
          color[1] = i;
          color[2] = 0;
          rgb_leds[255+i] = CRGB(color[2],color[0],color[1]);
        }
        for (int i = 0;i < 255; i++){
          color[0] = 255 - i;
          color[1] = i;
          color[2] = 0;
          rgb_leds[510+i] = CRGB(color[1],color[2],color[0]);
        }
        for (int k = 0; k < NUM_LEDS; k++){
          leds[k] = rgb_leds[k];
        }
      }
    
    public: void color_spread_spectrum() {
        int ALL_LEDS = fraction_of_spectrum * NUM_LEDS/2;
        for (int j = 0;j < ALL_LEDS; j++){
          float number_ = j * 255;
          float number_1 = number_ / ALL_LEDS;
          float number_2 = floor(number_1);
          fleds[j] = CHSV(number_2,255,255);
          fleds[fraction_of_spectrum*NUM_LEDS - 1 - j] = CHSV(number_2,255,255);
        }
        for (int k = 0; k < NUM_LEDS; k++){
          leds[k] = fleds[k];
        }
      }
    public: void groups_of_color(){
        for (int i = 0; i < NUM_LEDS/6; i++) {
          leds[i] = CRGB(255 ,0,0);
        }
        for (int i = NUM_LEDS/6; i < 2*NUM_LEDS/6; i++) {
          leds[i] = CHSV(10, 255, 255);
        }
        for (int i = 2*NUM_LEDS/6; i < 3*NUM_LEDS/6; i++) {
          leds[i] = CRGB(255, 245, 0);
        }
        for (int i = 3*NUM_LEDS/6; i < 4*NUM_LEDS/6; i++) {
          leds[i] = CRGB(0, 255, 0);
        }
        for (int i = 4*NUM_LEDS/6; i < 5*NUM_LEDS/6; i++) {
          leds[i] = CHSV(160, 255, 255);
        }
        for (int i = 5*NUM_LEDS/6; i < NUM_LEDS; i++) {
          leds[i] = CHSV(180, 255, 255);
        }
      }
    public: void christmas(){
      for (int i = 0; i< NUM_LEDS/5; i++){
        if(i%2 == 0){
          leds[5*i] = CRGB(255 ,0,0);
          leds[5*i+1] = CRGB(255 ,0,0);
          leds[5*i+2] = CRGB(255 ,0,0);
          leds[5*i+3] = CRGB(255 ,0,0);
          leds[5*i+4] = CRGB(255 ,0,0);     
          }
        else{
          leds[5*i] = CRGB(0, 255, 0);
          leds[5*i+1] = CRGB(0, 255, 0);
          leds[5*i+2] = CRGB(0, 255, 0);
          leds[5*i+3] = CRGB(0, 255, 0);
          leds[5*i+4] = CRGB(0, 255, 0);
          }
        }
      }
    public: void rainbow(){
      for (int i=0;i<NUM_LEDS/6;i++){
        leds[6*i] = CRGB(255 ,0,0);
        leds[6*i+1] = CHSV(10, 255, 255);
        leds[6*i+2] = CRGB(255, 245, 0);
        leds[6*i+3] = CRGB(0, 255, 0);
        leds[6*i+4] = CHSV(160, 255, 255);
        leds[6*i+5] = CHSV(180, 255, 255);
        }
      }
    public: void rainbow_2(){
      for (int i=0;i<NUM_LEDS/30;i++){
        for (int x=0;x<5;x++){
        leds[30*i+x] = red;
        }
      
        for (int x=5;x<10;x++){
        leds[30*i+x] = orange;
        }
        
        for (int x=10;x<15;x++){
        leds[30*i+x] = yellow;
        }
        
        for (int x=15;x<20;x++){
        leds[30*i+x] = green;
        }
        
        for (int x=20;x<25;x++){
        leds[30*i+x] = blue;
        }
        
        for (int x=25;x<30;x++){
        leds[30*i+x] = purple;
        }
      }
      }
      
    public: void bomber(){
      for (int i = 0; i< int(NUM_LEDS/5); i++){
        if(i%2 == 0){
          leds[5*i] = CRGB(50, 255, 0);
          leds[5*i+1] = CRGB(50, 255, 0);
          leds[5*i+2] = CRGB(50, 255, 0);
          leds[5*i+3] = CRGB(50, 255, 0);
          leds[5*i+4] = CRGB(50, 255, 0);
          }
        else{
          leds[5*i] = CRGB(1 ,1,1);
          leds[5*i+1] = CRGB(1 ,1,1);
          leds[5*i+2] = CRGB(1 ,1,1);
          leds[5*i+3] = CRGB(1 ,1,1);
          leds[5*i+4] = CRGB(1 ,1,1);
          }
        }
      }
    
    public: void color(CRGB hue){
      for (int i = 0;i<NUM_LEDS;i++){
        leds[i] = hue;
        }
    }
    
    public: void republican_party(){
      for (int i = 0;i<NUM_LEDS;i++){
        leds[i] = CHSV(0,0,255);
        
        }
    }
} lightStarter;

void setup() {
  Serial.begin(500);
  // PUT YOUR STARTING COLORS HERE
  lightStarter.color_spectrum_half_wrap();

  // Uncomment/edit one of the following lines for your leds arrangement.
  // FastLED.addLeds<TM1803, DATA_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<TM1804, DATA_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<TM1809, DATA_PIN, RGB>(leds, NUM_LEDS);
//  FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<WS2812, DATA_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  // FastLED.addLeds<APA104, DATA_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<UCS1903, DATA_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<UCS1903B, DATA_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<GW6205, DATA_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<GW6205_400, DATA_PIN, RGB>(leds, NUM_LEDS);

  // FastLED.addLeds<WS2801, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<SM16716, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<LPD8806, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<P9813, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<APA102, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<DOTSTAR, RGB>(leds, NUM_LEDS);

  // FastLED.addLeds<WS2801, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<SM16716, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<LPD8806, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<P9813, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
  // FastLED.addLeds<DOTSTAR, DATA_PIN, CLOCK_PIN, RGB>(leds, NUM_LEDS);
}
class LightChanger{
    public: void merge() {
      CRGB jawn = leds[0];
      for (int i = 0; i< NUM_LEDS - 1; i++){
          leds[i] = leds[i+1];
        }
      leds[NUM_LEDS - 1] = jawn;
    }
    public: void merge_wrap() {
      CRGB jawnski = fleds[0];
      for (int j = 0; j< 2*NUM_LEDS - 1; j++){
          fleds[j] = fleds[j+1];
        }
      fleds[2*NUM_LEDS - 1] = jawnski;
      for (int i = 0; i< NUM_LEDS; i++){
          leds[i] = fleds[i];
        }
    }
    public: void merge_half_wrap() {
      CRGB jawnski = fleds[0];
      for (int j = 0; j< 2*NUM_LEDS - 1; j++){
          fleds[j] = fleds[j+1];
        }
      fleds[2*NUM_LEDS - 1] = jawnski;
      for (int i = 0; i< NUM_LEDS/2; i++){
          leds[NUM_LEDS/2 - i - 1] = fleds[i];
          leds[NUM_LEDS/2 + i] = fleds[i];
        }
    }
    public: void merge_spread() {
      CRGB jawnski = fleds[0];
      for (int j = 0; j< fraction_of_spectrum*NUM_LEDS - 1; j++){
          fleds[j] = fleds[j+1];
        }
      fleds[fraction_of_spectrum*NUM_LEDS - 1] = jawnski;
      for (int i = 0; i< NUM_LEDS; i++){
          leds[i] = fleds[i];
        }
      }
    public: void merge_rgb() {
      CRGB jawnski = rgb_leds[0];
      for (int j = 0; j< 764; j++){
          rgb_leds[j] = rgb_leds[j+1];
        }
      rgb_leds[764] = jawnski;
      for (int i = 0; i< NUM_LEDS; i++){
          leds[i] = rgb_leds[i];
        }
      }
    public: void flow() {
      for (int i = 0; i<NUM_LEDS; i++){
        leds[i] = CHSV(color_number,255,255);
        } 
      color_number = color_number + 1;
    }
    
    public: void flash() {
      for (int i = 0; i<NUM_LEDS; i++){
        leds[i] = CHSV(color_number,255,255);
        } 
      color_number = random_int(0,255);
    }
    
    public: void flow2(int color) {
      for (int i = 0; i<NUM_LEDS; i++){
        leds[i] = CHSV(color,255,255);
        } 
    }
} lightChanger;

void loop() {
  lightChanger.merge_half_wrap();
  FastLED.show();
  delay(100);
}





