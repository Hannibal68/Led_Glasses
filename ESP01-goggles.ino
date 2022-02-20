/***********************************************
 * ESP 8266 - RGB ledstrip - NEOpixel
 * 
 * ESP01 pins: PIN 0 = PH5 = IO0 = PWM
 *             PIN 1 = PH2 = IO1 = TXD0
 *             PIN 3 = PH3 = IO2 = TXD1 / PWM  <---- used
 *                     PH1 = GND
 *                     PH7 = RXD0
 *                     PH8 = VCC
 *  GPIO0 = GPIO0 (need to have pull-up resistors connected to ensure the module starts up correctly)
 *  GPIO1 = BuiltIn blue Led ESp01
 *  TX    = GPIO1 (used as the Data line, will get some debug output on GPIO1 on power up)
 *  GPIO2 = GPIO2 (need to have pull-up resistors connected to ensure the module starts up correctly)
 *  GPIO3 = RXD (button 
 *  RX    = GPIO3 (best practice = input) (will be output HIGH on startup)
 * 
 *  WEMOS D1:
 ***********************************************/

#include <SPI.h>
//#include <Ethernet.h>
#include <ESP8266WiFi.h>
//#include <BlynkSimpleEthernet.h>
#include <BlynkSimpleEsp8266.h>
#include <Adafruit_NeoPixel.h>
//#include <SimpleTimer.h> // here is the SimpleTimer library
#include <TimeLib.h>
#include <WidgetRTC.h>

//SimpleTimer timer; // Create a Timer object called "timer"!
WidgetRTC rtc;

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "f776d3dd65fc4ba2923eca140a731c5f"; //"9c4329f0745d42bdb1857a21b629a0b8"
char ssid[] = "Kleffies"; 
char pass[] = "Feelknavsnah68"; 

#define PIN_RGB           2           // RGB Neopixel
#define PIN_BUT           3           // Button pin (normal TX)
#define GOG_N_LEFT       12           // ring left
#define GOG_N_MID        3            // led middle
#define GOG_N_RIGHT      12           // ring right
#define GOG_S_LEFT       0
#define GOG_S_MID        12
#define GOG_S_RIGHT      15 
const int  NUMPIXELS = GOG_N_LEFT+GOG_N_MID+GOG_N_RIGHT;
const int  GOGNUM  = NUMPIXELS+GOG_N_MID;
const byte GOG[GOGNUM] = {1,2,3,4,5,6,7,8,9,10,11,0,12,13,14,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12};
const byte GOG_R[GOG_N_RIGHT] = {15,26,25,24,23,22,21,20,19,18,17,16};
#define RRED           0              // 65536 x 0/6
#define ORAN           5500           // 65536 x 1/12 (5461)
#define YEL            10923          // 65536 x 1/6
#define GRN            21845          // 65536 x 2/6
#define CYANN          32768          // 65536 x 3/6
#define BLU            43690          // 65536 x 4/6
#define PUR            54613          // 65536 x 5/6
#define RBOW           180            // Rainbow colors
#define dCOL           180            // colorcircle partitioning
#define SAT            255            // [0-255] = saturation, 128 = pastel tint
#define SPOW           200            // start brightness/power
#define SPOW_MAX       220            // maximum brightness
#define BLYNK_PRINT Serial

uint16_t col[9] = {1,15,30,50,70,90,110,130,150};
byte leds[NUMPIXELS][3] = {};
byte effect;                     // slider effect number
int Power = SPOW;                // brightness backup
int maxPow = SPOW_MAX;           // slider max brightness
int n1a = 0, n1b = 0;            // counters
uint16_t COL1, COL2, COLbuf;     // color 16 bit choose
uint16_t COLst1, COLst2;         // color 16 bit start
int  col1, col2;                 // slider color 8 bit
int  wait, stp;                  // scroll speed leds (0 is slow, 100 is fast)
int  spd, len;                   // slider speed and length light
bool newData = true;             // check new serial data
bool change = true;              // change variable
bool dir = false;                // direction fill

BlynkTimer timer;
// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel NeoPixel = Adafruit_NeoPixel(NUMPIXELS, PIN_RGB, NEO_GRB + NEO_KHZ800);

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return NeoPixel.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return NeoPixel.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    WheelPos -= 170;
    return NeoPixel.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

void setup() {
  // Debug console
  pinMode(PIN_BUT,INPUT_PULLUP);
  // Serial.begin(115200);
  Serial.begin(115200,SERIAL_8N1,SERIAL_TX_ONLY); // use RX as normal I/O
  // Serial.begin(115200,SERIAL_8N1,SERIAL_RX_ONLY); // use TX as normal I/O
  
  Blynk.begin(auth, ssid, pass);
  // You can also specify server:
  // Blynk.begin(auth, ssid, pass, "blynk-cloud.com", 80);
  // Blynk.begin(auth, ssid, pass, IPAddress(192,168,4,2), 8080);

  // Begin synchronizing time
  // rtc.begin();

  // Other Time library functions can be used, like:
  // timeStatus(), setSyncInterval(interval)...
  // Read more: http://www.pjrc.com/teensy/td_libs_Time.html
  // Display digital clock every 10 seconds
  // timer.setInterval(10000L, clockDisplay);
  timer.setInterval(1000L, sendUptime); // Here you set interval (1sec) and which function to

  NeoPixel.begin();
  NeoPixel.setBrightness(150);   // adjust brightness here
  NeoPixel.clear();
  NeoPixel.show();               // Initialize all pixels to 'off'

//  colorWipe(NeoPixel.Color(255, 0, 0), 50); // Red
//  colorWipe(NeoPixel.Color(0, 255, 0), 50); // Green
//  colorWipe(NeoPixel.Color(0, 0, 255), 50); // Blue
//  rainbow(16);
//  rainbowCycle(16);
    randomSeed(ESP.getCycleCount());
    effect = 5;
    newData = true;
    spd = 50;
    col1 = 1;   COL1 = HSV(col1);
    col2 = 100; COL2 = HSV(col2);
    maxPow = SPOW_MAX;
    len = 3;
    Blynk.virtualWrite(V0,effect);
    Blynk.virtualWrite(V1,col1);
    Blynk.virtualWrite(V2,col2);
    Blynk.virtualWrite(V3,maxPow);
    Blynk.virtualWrite(V4,len);
    Blynk.virtualWrite(V5,spd);
}

void loop() {
    Blynk.run();     // to avoid delay() function!
    //  timer.run();
    switch(effect) {
      case 1:                                   // Meteor cycle (werkt)
         if (newData) {
            newData = false;
         }
         MeteorCycle(spd);
         break;
      case 2:                                   // Snake 1 length (werkt)
         if (newData) {
            newData = false;
         }
         Goggle_1snake(len, spd);
         break;
      case 3:                                   // Snake 2 left and right (werkt)
         if (newData) {
            newData = false;
         }
         Goggle_2snake(len, spd);
         break;
      case 4:
         if (newData) {                        // Scroll 2 colors (werkt niet)
            COLst1 = COL1; COLst2 = COL2;
            n1a = 0;
            newData = false;
         }
         Scroll2Colors(3,spd);
         break;
      case 5:                                   // Sparkle colors (werkt)
         if (newData) {
            FillArray();
            newData = false;
         }
         Sparkle(spd);
         break;
      case 6:                                  // Fade 2 colors (werkt)
         if (newData) {
            change = true;
            stp = 20;
            Power = 40; // maxPow = 220 werkt
            newData = false;
         }
         FadeColor(spd);
         break;
    }
}

/*
BLYNK_WRITE(V1) {  // Widget WRITEs to Virtual Pin V1
  int R = param[0].asInt();
  int G = param[1].asInt();
  int B = param[2].asInt();
//  int shift = param.asInt();
//  for (int i = 0; i < NUMPIXELS; i++) {
//    NeoPixel.setPixelColor(i, NeoPixel.Color(R,G,B));
    // NeoPixel.setPixelColor(i, Wheel(shift & 255));
    // OR: NeoPixel.setPixelColor(i, Wheel(((i * 256 / NeoPixel.numPixels()) + shift) & 255));
//  }
//  NeoPixel.show(); // sends updated pixel color to hardware
}
*/

BLYNK_WRITE(V0) {  // Button Widget is writing to pin V0
  int neweffect = param.asInt();
  if (neweffect != effect) newData = true;
  effect = neweffect;
}

BLYNK_WRITE(V1) {  // Button Widget is writing to pin V1
  col1 = param.asInt();
  COL1 = HSV(col1);
}

BLYNK_WRITE(V2) {  // Button Widget is writing to pin V2
  col1 = param.asInt();
  COL2 = HSV(col1);
}

BLYNK_WRITE(V3) {  // Button Widget is writing to pin V3
  maxPow = param.asInt();
}

BLYNK_WRITE(V4) {  // Button Widget is writing to pin V4
  len = param.asInt();
}

BLYNK_WRITE(V5) {  // Button Widget is writing to pin V5
  spd = param.asInt();
}

void sendUptime() {
// This function sends Arduino up time every 1 second to Virtual Pin (V5)
// In the app, Widget's reading frequency should be set to PUSH
// You can send anything with any interval using this construction
// Don't send more that 10 values per second
  Blynk.virtualWrite(V2, millis() / 1000);
  Blynk.virtualWrite(V3, millis() / 1000);
}

uint16_t HSV(int C180) {
   if (C180 <= dCOL) return (C180-1)*65535/dCOL; // (C180<180 ? 0 : 1)
   else return C180;
}

void rainbow(uint8_t wait) {
  uint16_t i, j;
  for(j=0; j<256; j++) {
    for(i=0; i<NUMPIXELS; i++) {
      NeoPixel.setPixelColor(i, Wheel((i+j) & 255));
    }
    NeoPixel.show();
    delay(wait);
  }
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait2) {
  uint16_t i, j;
  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< NeoPixel.numPixels(); i++) {
      NeoPixel.setPixelColor(i, Wheel(((i * 256 / NeoPixel.numPixels()) + j) & 255));
    }
    NeoPixel.show();
    delay(wait2);
  }
}

void MeteorCycle(int wait2) {
  for (int i=0; i<NUMPIXELS; i++) {
    NeoPixel.setPixelColor(i, NeoPixel.gamma32(NeoPixel.ColorHSV(COL1, SAT, maxPow)));
    NeoPixel.show();
    delay(wait2);
    NeoPixel.clear();
    NeoPixel.show();
    delay(wait2);
  }
}

void Goggle_1snake(int len2, int wait2) {
   int x=0;
   uint16_t CC;
   NeoPixel.clear();
   n1b = n1a;
   for (int i=0; i<len2; i++) {
      if (col1 == RBOW || col2 == RBOW) CC = (len2-i-1)*(65000/len2);
      else CC = COL1;
      NeoPixel.setPixelColor(GOG[n1a+x], NeoPixel.gamma32(NeoPixel.ColorHSV(CC, SAT, (i+1)*(240/len2))));
      x++;
      if (n1a+x >= GOGNUM) {
         x = 0;
         n1a = 0;
      }
   }
   NeoPixel.show();
   n1a = n1b + 1;
   if (n1a >= GOGNUM) n1a = 0;
   delay(wait2);
}

void Goggle_2snake(int len2, int wait2) {
   int x = 0;
   uint16_t CC;
   NeoPixel.clear();
   n1b = n1a;
   for (int i=0; i<len2; i++) {
      if (col1 == RBOW || col2 == RBOW) CC = (len2-i-1)*(65000/len2);
      else CC = COL1;
      NeoPixel.setPixelColor(n1a+x,        NeoPixel.gamma32(NeoPixel.ColorHSV(CC, SAT, (i+1)*(240/len2))));
      NeoPixel.setPixelColor(GOG_R[n1a+x], NeoPixel.gamma32(NeoPixel.ColorHSV(CC, SAT, (i+1)*(240/len2))));
      x++;
      if (n1a+x >= GOG_N_LEFT) {
         x = 0;
         n1a = 0;
      }
   }
   NeoPixel.show();
   n1a = n1b + 1;
   if (n1a >= GOG_N_LEFT) n1a = 0;
   delay(wait2);
}

void Scroll2Colors(int len2, int wait2) {     // werkt niet
  n1b = len2-n1a;
  COLbuf = COLst1;
  for (int i=0; i < NUMPIXELS;  i++) {
     NeoPixel.setPixelColor(GOG[i], NeoPixel.gamma32(NeoPixel.ColorHSV(COLbuf, SAT, maxPow)));
     n1b++;
     if (n1b >= len2) {
        n1b = 0; 
        if (COLbuf == COL1) COLbuf = COL2;
        else COLbuf = COL1;
     }
  }
  n1a++; if (n1a > len2) {
     n1a = 1;
     if (COLst1 == COL1) COLst1 = COL2;
     else COLst1 = COL1;
  }
  NeoPixel.show();
  delay(wait2);
}

void Sparkle(int wait2) {
  int stp1 = 10;
  for (int i=0; i<NUMPIXELS; i++) {
    stp1 = random(10,20);
    if (leds[i][2] == 1) {
      if (leds[i][1] > 20+stp1) {
        leds[i][1] = leds[i][1]-stp1;
      }
      else {
        leds[i][0] = col[random(9)];
        leds[i][2] = 0;
      }
    }
    else {
      if (leds[i][1] < maxPow-stp1) {
        leds[i][1] = leds[i][1]+stp1;
      }
      else {
        leds[i][2] = 1;
      }
    }
    COLbuf = HSV(leds[i][0]);
    NeoPixel.setPixelColor(i, NeoPixel.gamma32(NeoPixel.ColorHSV(COLbuf, SAT, leds[i][1])));
  }
  NeoPixel.show();
  delay(wait2);
}

void FillArray() {
  for (int i=0; i<NUMPIXELS; i++) {
    leds[i][0] = col[random(9)];                // random color
    leds[i][1] = random(1-10)*maxPow/10+40;     // random brightness
    leds[i][2] = random(2);                     // up or down brightness;
  }
}

void FadeColor(int wait2) {
   const byte minPow = 20;
   for (int i=0; i < NUMPIXELS; i++) {
      if (COL1 == COL2)
         NeoPixel.setPixelColor(i, NeoPixel.gamma32(NeoPixel.ColorHSV(COL1, SAT, Power)));
      else {
         if (i < GOG_N_LEFT)
            NeoPixel.setPixelColor(i, NeoPixel.gamma32(NeoPixel.ColorHSV(COL1, SAT, Power)));
         else
            if (i >= GOG_S_RIGHT)
               NeoPixel.setPixelColor(i, NeoPixel.gamma32(NeoPixel.ColorHSV(COL2, SAT, maxPow-Power+abs(stp))));
            else 
               NeoPixel.setPixelColor(i, NeoPixel.gamma32(NeoPixel.ColorHSV(COL2, 0, 0)));
      }
   }
   Power = Power + stp;
   if (Power < minPow || Power > maxPow) {
      stp = -stp;
      Power = Power + stp;
      COLbuf = COL1;  COL1 = COL2;  COL2 = COLbuf;
   }
   NeoPixel.show();
   delay(wait2);
}
