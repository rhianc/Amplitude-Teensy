#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <math.h>
#include <SD.h>
#include <SerialFlash.h>
#include "FastLED.h"

// Set-Up for audioShield communication
///////////////////////////////////////////////

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
int placement[8] = {4,5,6,7,3,2,1,0};

//----------------------------------------------
//----------------------------CORE PROGRAM------------------------------
//----------------------------------------------------------------------

// Run setup once
void setup() {
  // Enable Serial
  Serial.begin(9600);
  delay(100);
  leds.begin();
  leds.show();
}

void loop() {
  Serial.println("hello");
  delay(1000);
  test();
  leds.show();
//  checkForMessage();
//  if(lightsOn){
//    if (fft.available()){
//      color_spectrum_half_wrap(true);
//      //beatDetectorUpdate();
//      leds.show();
//    }
//    timer = millis();
//    if (timer-startTimer > 100){
//      moving_color_spectrum_half_wrap(1);   // modifies color mapping
//      startTimer = millis();
//    }
//  }
}


//-----------------------------------------------------------------------
//----------------------------SETUP FUNCTIONS----------------------------
//-----------------------------------------------------------------------

void test(){
  for (int x = 0; x < 8; x++){ 
    for(int i=0;i<NUM_LEDS;i++){
      if(placement[x] < 4){
        leds.setPixel(x*NUM_LEDS+i, 255-(placement[x]*60), placement[x]*60, 0);
      }else if(placement[x] < 7){
        leds.setPixel(x*NUM_LEDS+i,0, 255-((placement[x]-4)*80), (placement[x]-4)*80);
      }else{
        leds.setPixel(x*NUM_LEDS+i,255, 0, 255);
      }
    }
  }
}



//-----------------------------------------------------------------------
//----------------------------LOOP FUNCTIONS----------------------------
//-----------------------------------------------------------------------
