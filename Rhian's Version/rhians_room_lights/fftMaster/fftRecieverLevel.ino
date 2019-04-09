// TO DO:
// normalize input data so that it interacts identicaly with lights code (>>16)
// will need to convert data to float first


#define TtoTSerial Serial3

int fftData[512];

void setup() {
  TtoTSerial.begin(2000000); // highest zero-error baud rate
  Serial.begin(9600);
}

float timeNow= micros();

void loop() {
  getFFT();

  // call an update lights function here which will be updated synchronously
  // with the FFT itself
}

int binCount = 0;
int byteCount = 0;
int dataInBits = 0;

void getFFT() {
  // Receives FFT data over serial from main teensy
  int incomingByte;
  if (TtoTSerial.available() > 0) {
    incomingByte = TtoTSerial.read();
    if (byteCount == 0){
      dataInBits = incomingByte << 8;
      byteCount = 1;
    }
    else{
      byteCount = 0;
      fftData[binCount] = dataInBits + incomingByte;
      Serial.print(fftData[binCount]);
      Serial.print("\n");
      binCount = (binCount + 1) % 512;
      // below section just to test speed
      dataInBits = 0;
      if (binCount == 511) {
        float newTime = micros();
        String difference = String(newTime - timeNow);
        timeNow = newTime;
        Serial.print("FFT Received, microseconds elapsed: ");
        Serial.print(difference);
        Serial.print("\n");
      }
    }
   
    
    //Serial.print("UART received: ");
    //Serial.println(incomingByte, DEC);
    //TtoTSerial.print("UART received:");
    //TtoTSerial.println(incomingByte, DEC);
  }
}

