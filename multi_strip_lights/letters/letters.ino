#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SerialFlash.h>
#include "FastLED.h"
#include <math.h>

const int ALPHABET_LEN = 26;
char alphabet[ALPHABET_LEN] = "abcdefghijklmnopqrstuvwxyz";
//2 digits of OCTAL numbers. need to be broken down into binary. each number in a lettrs array represents a row. 6 binary digits in a row, 8 rows. Each letter is 6 columns of 8 lights, which limits small display to 10 letters.
//                                 A                        B                         C                         D                         E                         F                         G                         H                         I                         J                         K                         L                         M                         N                         O                         P                         Q                         R                         S                         T                         U                         V                         W                         X                         Y                         Z
int pixel_map[ALPHABET_LEN][8] = {{14,22,41,41,77,41,41,41},{76,41,41,76,76,41,41,76},{77,40,40,40,40,40,40,77},{76,43,43,43,43,43,43,76},{77,40,40,77,77,40,40,77},{77,40,40,77,40,40,40,40},{77,40,40,40,47,41,41,77},{63,63,63,77,77,63,63,63},{77,14,14,14,14,14,14,77},{77,14,14,14,14,54,74,30},{61,62,64,70,70,64,62,61},{60,60,60,60,60,60,77,77},{36,77,55,55,55,55,55,55},{41,61,71,55,55,47,43,41},{36,63,63,63,63,63,63,36},{76,63,63,76,60,60,60,60},{36,63,63,63,73,67,76,75},{77,63,63,77,74,66,63,63},{77,60,60,74,17,03,03,77},{77,77,14,14,14,14,14,14},{63,63,63,63,63,63,77,36},{63,63,22,22,36,36,14,14},{55,55,55,55,55,55,77,36},{63,36,36,14,14,36,36,63},{63,63,36,36,14,14,14,14},{77,03,06,14,14,30,60,77}};
int full_blank[8] = {0,0,0,0,0,0,0,0};
int half_blank[4] = {0,0,0,0};
int LetterArray[8][60] = {0};
int start_number;


void setup() {
  // put your setup code here, to run once:
  fillLetterArray("test");
}

void loop() {
  // put your main code here, to run repeatedly:
  for(int c=0;c<ALPHABET_LEN;c++){
    Serial.println(alphabet[c]);
  }
}

void fillLetterArray(char input[]){
  size_t len = strlen(input);
  for(int i=0;i<len;i++){
    fillLetterArrayHelper(30-(len*3)+(6*i),input[i]);
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
