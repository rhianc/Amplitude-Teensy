//Serial LED Light Visualisations
//Inspired by the LED Audio Spectrum Analyzer Display

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <math.h>
#include <SD.h>
#include <SerialFlash.h>

#define TtoTSerial Serial3 // Serial Communication with other teensy

// Line In
const int myInput = AUDIO_INPUT_LINEIN;

// PINS!
const int AUDIO_INPUT_PIN = 14;              // Input ADC pin for audio data.

// Audio library objects
AudioInputI2S            audioInput;         // audio shield: line-in
AudioInputAnalog         adc1(AUDIO_INPUT_PIN);       
AudioAnalyzeFFT1024      fft;        
AudioConnection          patchCord1(audioInput, 0, fft, 0);                 
AudioControlSGTL5000     audioShield;

// for testing purposes
int startTimer = 0;
int timer = 0;

//----------------------------------------------------------------------
//----------------------------CORE PROGRAM------------------------------
//----------------------------------------------------------------------

// Sender Necessary Constants
const int highHex = 0xff00;
const int lowHex = 0x00ff;

float timeNow= micros();

int testData = 12345;

// only send data when proper reciever ready message recieved on this end
bool recieverReadyFlag = true;
const int recieverReadyMessage = 0xff;

// Run setup once
void setup() {
  // Enable the audio shield and set the output volume
  Serial.begin(2000000);
  audioShield.enable();
  audioShield.inputSelect(myInput);
  audioShield.volume(1);
  
  TtoTSerial.begin(2000000); // highest zero-error baud rate (4608000 has theoretical -0.79% error)
  
  // the audio library needs to be given memory to start working
  AudioMemory(12);
}

void loop() {
  if (fft.available() && recieverReadyFlag) {
    sendFFT();  // Send all FFT data to other teensy/teensies 
  }
  else{
    listenForRecieverReadyFlag(); // only send if reciever is ready!
  }
}

void listenForRecieverReadyFlag(){
  if (TtoTSerial.available() > 0){
    byte possibleReadyMessage = TtoTSerial.read();  
    if (possibleReadyMessage == recieverReadyMessage){    // check if correct message has been recieved
      recieverReadyFlag = true;                           // now we can send the whole fft at once!
    }
  }
}

void sendFFT(){
  for (unsigned int x = 0; x < 512; x++){
    int dataPoint = fft.output[x];
    //unsigned int dataPoint = x;
    byte firstOne = (byte)((dataPoint&highHex)>>8);
    byte secondOne = (byte)(dataPoint&lowHex);
    TtoTSerial.write(firstOne);                   // transit MSBits first, cast to 8bit data
    TtoTSerial.write(secondOne);                  // transmit LSBits second, cast to 8bit data
    //unsigned int reconstruct = (firstOne << 8) + secondOne;
    //Serial.println(reconstruct);
    recieverReadyFlag = false;
  }
//  float newTime = micros();
//  String difference = String(newTime - timeNow);
//  timeNow = newTime;
//  Serial.print("FFT Sent, microseconds elapsed: ");
//  Serial.print(difference);
//  Serial.print("\n");
}

void sendTest(){
  TtoTSerial.write((byte)((testData&highHex)>>8)); // transit MSBits first, cast to 8bit data
  TtoTSerial.write((byte)(testData&lowHex));  // transmit LSBits second, cast to 8bit data
}
