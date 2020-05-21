#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

// Which pin on the Flora is connected to the NeoPixels?
#define LED_PIN    6  // NeoPixels attached to pin 6
#define MIC_PIN   A9  // Microphone is attached to this analog pin
#define DC_OFFSET  0  // DC offset in mic signal - if unusure, leave 0
#define NOISE     10  // Noise/hum/interference in mic signal
 
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))



  
// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(64, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  analogReference(EXTERNAL);
  pixels.begin();
  pixels.setBrightness(40); //To reduce power consumption
  Serial.begin(9600);
}

void clear() {
  for(int i=0;i<64;i++){
    pixels.setPixelColor(i, pixels.Color(0,0,0));
  }
}

void setPixelForColor(int row, int col, uint32_t color) {
  if ((row % 2) == 1){
    pixels.setPixelColor(row*8+(7-col), color); 
  } else {
    pixels.setPixelColor(row*8+col, color); 
  }
}


void drawShape(byte shape[], uint32_t color) {
    for(int i=0;i<8;i++) {
      for(int j=0; j<8; j++) {
        if(bitRead(shape[i], j) == 1) {
          setPixelForColor(i, j, color);
        }
        else {
          setPixelForColor(i, j, 0);
        }
      }
    }
    pixels.show(); // This sends the updated pixel color to the hardware.
}

/* local definitions */
#define POP_COUNT_THRESHOLD 30
#define LOW_NOISE_START 150
#define LOW_NOISE_DELAY_AFTER_POP 50
#define SMILE_FOR_A_WHILE 750

typedef enum {LOW_NOISE_B4_POP, COUNT_POP, LOW_NOISE_AFTER_POP} pop_state_type;
int countLowNoise=0,countPop=99, countAfterPop=0;
int smileTimer=0;
pop_state_type popState=LOW_NOISE_B4_POP;
int lvl=10;      // Current "dampened" audio level
  
bool handlePopStateChanges(int lvl)
{
    bool returnSmile=false;

    // check for state changes
    switch(popState) {
      case LOW_NOISE_B4_POP:
        // whilst in this state, count low noise
        if (lvl<5) {
          countLowNoise++;
        }
        else {
          // change to counting pops, if lvl is above 5 and low noise count is above threshold
          if ((countLowNoise > LOW_NOISE_START) & (lvl > 5)) {
            popState=COUNT_POP;
            countPop=1; // had one count to start
          }

          // reset low noise count
          countLowNoise=0;
        }
        break;

      case COUNT_POP:
        // whilst in this state, increment pop counts
        if (lvl > 5) {
          countPop++;
        }
        
        // change state to after pop counting, if 0>popCount<threshold and its low noise again
        if ((countPop>1) & (countPop<POP_COUNT_THRESHOLD) & (lvl<5)) {
          popState=LOW_NOISE_AFTER_POP;
          countPop=0;
        }
        else {
          // if low noise, revert to start
          if (lvl<5) popState=LOW_NOISE_B4_POP;
        }

        // always reset the low noise count
        countLowNoise=0;
        break;

      case LOW_NOISE_AFTER_POP:
        // whilst in this state, count low noise
        if (lvl<5) {
          countLowNoise++;
          if (countLowNoise>LOW_NOISE_DELAY_AFTER_POP) {
            returnSmile=true;
            countLowNoise=0;
            popState=LOW_NOISE_B4_POP;
          }
        }
        else {
          // back to start
          popState=LOW_NOISE_B4_POP;
          countLowNoise=0;
        }
      break;
    }
    return(returnSmile);
}



byte mouth[][8] = {{B00000000,B00000000,B00000000,B11111111,B11111111,B00000000,B00000000,B00000000},
                   {B00000000,B00000000,B01111110,B10000001,B10000001,B01111110,B00000000,B00000000},
                   {B00000000,B00111100,B01000010,B10000001,B10000001,B01000010,B00111100,B00000000},
                   {B00111100,B01000010,B10000001,B10000001,B10000001,B10000001,B01000010,B00111100},
                   {B00000000,B00000000,B10000001,B11000011,B01111110,B00111100,B00000000,B00000000}};
 
void loop() {
  int n;

  n   = analogRead(MIC_PIN);                        // Raw reading from mic 
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvl = ((lvl * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)
  //Serial.println(lvl, DEC);

  // look for pop count state changes
  if (handlePopStateChanges(lvl)) smileTimer = SMILE_FOR_A_WHILE;
  
  // look for a pop to make a smile
  if (smileTimer > 0) {
    drawShape(mouth[4], pixels.Color(28,172,247));
    smileTimer = MAX(0, smileTimer-1);
  }
  else {
    // otherwise process talking
    if (lvl<5) {
      drawShape(mouth[0], pixels.Color(28,172,247));
    }
    else {
      if (lvl<12) {
           drawShape(mouth[1], pixels.Color(28,172,247)); 
      }
      else {
        if (lvl<20) {
           drawShape(mouth[2], pixels.Color(28,172,247)); 
        }
        else{
           drawShape(mouth[3], pixels.Color(28,172,247)); 
        }
      }
    }
  }
}
