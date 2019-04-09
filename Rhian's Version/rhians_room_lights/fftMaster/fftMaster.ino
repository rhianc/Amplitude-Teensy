//Serial LED Light Visualisations
//Inspired by the LED Audio Spectrum Analyzer Display

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <math.h>
#include <SD.h>
#include <SerialFlash.h>

#define TtoTSerial Serial3 // Serial Communication with other teensy

// Line IN
const int myInput = AUDIO_INPUT_LINEIN;

// PINS!
const int AUDIO_INPUT_PIN = 14;        // Input ADC pin for audio data.
const int OUTPUT_PIN_0 = 1;     // First LED Strip
const int OUTPUT_PIN_1 = 5;      // Second LED Strip

// Audio library objects
AudioInputI2S            audioInput;         // audio shield: mic or line-in

//AudioOutputI2S           audioOutput;                   // stream music to audio shield output
//AudioConnection          patchCord2(audioInput, 0, audioOutput, 0);
//AudioConnection          patchCord3(audioInput, 1, audioOutput, 1);

AudioInputAnalog         adc1(AUDIO_INPUT_PIN);       
AudioAnalyzeFFT1024      fft;        

AudioConnection          patchCord1(audioInput, 0, fft, 0);                 
//AudioConnection          patchCord1(adc1, fft);

AudioControlSGTL5000 audioShield;

// empty arrays for generating 

// not 100% sure where these are used
int startTimer = 0;
int timer = 0;
int counter = 0;
//----------------------------------------------------------------------
//----------------------------CORE PROGRAM------------------------------
//----------------------------------------------------------------------

// Sender Necessary Constants
const int highHex = 0xff00;
const int lowHex = 0x00ff;

float timeNow= micros();

int testData = 12345;


// Run setup once
void setup() {
  // Enable the audio shield and set the output volume.
  Serial.begin(9600);
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(1);
  
  TtoTSerial.begin(2000000); // highest zero-error baud rate
  
  pinMode(15, INPUT_PULLUP); // pin 14 used for button read
  // the audio library needs to be given memory to start working
  AudioMemory(12); // this is probably why the longer FFT wasn't working, is this the right amount??
}

void loop() {
  if (fft.available()) {
    //sendTest();
    sendFFT();  // Send all FFT data to other teensy/teensies 
  }
}

void sendFFT(){
  for (int x = 0; x < 512; x++){
    //int dataPoint = fft.output[x];
    int dataPoint = 0;
    TtoTSerial.write((byte)((dataPoint&highHex)>>8)); // transit MSBits first, cast to 8bit data
    TtoTSerial.write((byte)(dataPoint&lowHex));  // transmit LSBits second, cast to 8bit data
  }
  float newTime = micros();
  String difference = String(newTime - timeNow);
  timeNow = newTime;
  Serial.print("FFT Sent, microseconds elapsed: ");
  Serial.print(difference);
  Serial.print("\n");
}

void sendTest(){
  TtoTSerial.write((byte)((testData&highHex)>>8)); // transit MSBits first, cast to 8bit data
  TtoTSerial.write((byte)(testData&lowHex));  // transmit LSBits second, cast to 8bit data
}
