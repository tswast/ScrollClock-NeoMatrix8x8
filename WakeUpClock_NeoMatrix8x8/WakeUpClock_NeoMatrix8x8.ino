/*
   WAKE UP CLOCK - 8x8 NeoPixel Desktop Edition
   Forked from WORD CLOCK by Andy Doro

   A word clock using NeoPixel RGB LEDs for a color shift effect.

   Hardware:
   - Trinket Pro 5V (should work with other Arduino-compatibles with minor modifications)
   - DS1307 RTC breakout
   - NeoPixel NeoMatrix 8x8


   Software:

   This code requires the following libraries:

   - RTClib https://github.com/adafruit/RTClib
   - DST_RTC https://github.com/andydoro/DST_RTC
   - Adafruit_GFX https://github.com/adafruit/Adafruit-GFX-Library
   - Adafruit_NeoPixel https://github.com/adafruit/Adafruit_NeoPixel
   - Adafruit_NeoMatrix https://github.com/adafruit/Adafruit_NeoMatrix


   Wiring:
   - Solder DS1307 breakout to Trinket Pro, A2 to GND, A3 to PWR, A4 to SDA, A5 to SCL
   If you leave off / clip the unused SQW pin on the RTC breakout, the breakout can sit right on top of the Trinket Pro
   for a compact design! It'll be difficult to reach the Trinket Pro reset button, but you can activate the bootloader by
   plugging in the USB.
   - Solder NeoMatrix 5V to Trinket 5V, GND to GND, DIN to Trinket Pro pin 8.


   grid pattern

    A T W E N T Y D
    Q U A R T E R Y
    F I V E H A L F
    D P A S T O R O
    F I V E I G H T
    S I X T H R E E
    T W E L E V E N
    F O U R N I N E


    Acknowledgements:
    - Thanks Dano for faceplate / 3D models & project inspiration!

*/

// include the library code:
#include <Wire.h>
#include <RTClib.h>
#include <DST_RTC.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

// define how to write each of the words

// 64-bit "mask" for each pixel in the matrix- is it on or off?
uint64_t mask;

// define masks for each word. we add them with "bitwise or" to generate a mask for the entire "phrase".
#define MFIVE    mask |= 0xF00000000000        // these are in hexadecimal
#define MTEN     mask |= 0x5800000000000000
#define AQUARTER mask |= 0x80FE000000000000
#define TWENTY   mask |= 0x7E00000000000000
#define HALF     mask |= 0xF0000000000
#define PAST     mask |= 0x7800000000
#define TO       mask |= 0xC00000000
#define ONE      mask |= 0x43
#define TWO      mask |= 0xC040
#define THREE    mask |= 0x1F0000
#define FOUR     mask |= 0xF0
#define FIVE     mask |= 0xF0000000
#define SIX      mask |= 0xE00000
#define SEVEN    mask |= 0x800F00
#define EIGHT    mask |= 0x1F000000
#define NINE     mask |= 0xF
#define TEN      mask |= 0x1010100
#define ELEVEN   mask |= 0x3F00
#define TWELVE   mask |= 0xF600
#define ANDYDORO mask |= 0x8901008700000000

// define masks for each number
#define PICO_ZERO  075557
#define PICO_ONE   062227
#define PICO_TWO   071747
#define PICO_THREE 071317
#define PICO_FOUR  055711
#define PICO_FIVE  074717
#define PICO_SIX   044757
#define PICO_SEVEN 071111
#define PICO_EIGHT 075757
#define PICO_NINE  075711
#define PICO_COLON 002020

// define pins
#define NEOPIN 8  // connect to DIN on NeoMatrix 8x8
#define RTCGND A2 // use this as DS1307 breakout ground 
#define RTCPWR A3 // use this as DS1307 breakout power


// brightness based on time of day- could try warmer colors at night?
#define DAYBRIGHTNESS 40
#define NIGHTBRIGHTNESS 20

// cutoff times for day / night brightness. feel free to modify.
#define MORNINGCUTOFF 7  // when does daybrightness begin?   7am
#define NIGHTCUTOFF   22 // when does nightbrightness begin? 10pm


// define delays
#define FLASHDELAY 250  // delay for startup "flashWords" sequence
#define SHIFTDELAY 100   // controls color shifting speed

#define CLOCKWIDTH (17)  // hours (3+1+3+1=8), colon (2), minutes (3+1+3) with spaces in between
#define CLOCKHEIGHT 5
byte clockBuffer[CLOCKHEIGHT][CLOCKWIDTH];
byte screenBuffer[8][8];
int clockX;
int clockY;

RTC_DS1307 RTC; // Establish clock object
DST_RTC dst_rtc; // DST object

DateTime theTime; // Holds current clock time

int j;   // an integer for the color shifting effect

// Do you live in a country or territory that observes Daylight Saving Time?
// https://en.wikipedia.org/wiki/Daylight_saving_time_by_country
// Use 1 if you observe DST, 0 if you don't. This is programmed for DST in the US / Canada. If your territory's DST operates differently,
// you'll need to modify the code in the calcTheTime() function to make this work properly.
#define OBSERVE_DST 1


// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//Adafruit_NeoPixel matrix = Adafruit_NeoPixel(64, NEOPIN, NEO_GRB + NEO_KHZ800);

// configure for 8x8 neopixel matrix
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 8, NEOPIN,
                            NEO_MATRIX_TOP  + NEO_MATRIX_LEFT +
                            NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE,
                            NEO_GRB         + NEO_KHZ800);

// Colors to scale from night to full sun
uint16_t screenPalette[] = {
  matrix.Color(0, 0, 0),       // 0 black
  matrix.Color(68, 2, 40),     // 1 purple
  matrix.Color(121, 6, 10),    // 2 dark-red
  matrix.Color(180, 64, 16),   // 3 orangetone
  matrix.Color(188, 53, 0),    // 4 rosy brown
  matrix.Color(226, 88, 34),   // 5 flame
  matrix.Color(255, 0, 77),    // 6 red
  matrix.Color(255, 147,41),   // 7 candle, http://planetpixelemporium.com/tutorialpages/light.html
  matrix.Color(255, 119, 168), // 8 pink
  matrix.Color(255, 204, 170), // 9 peach
  matrix.Color(255, 214, 170), // 10 100W tungsten
  matrix.Color(255, 241, 232), // 11 white
};

#define PICO_BLACK 0
#define BROWN_COLOR 4
#define CANDLE_COLOR 7
#define PICO_WHITE ((sizeof(screenPalette) / sizeof(uint16_t)) - 1)

void setup() {
  // put your setup code here, to run once:

  //Serial for debugging
  //Serial.begin(9600);

  // set pinmodes
  pinMode(NEOPIN, OUTPUT);

  // set analog pins to power DS1307 breakout!
  pinMode(RTCGND, OUTPUT); // analog 2
  pinMode(RTCPWR, OUTPUT); // analog 3

  // set them going!
  digitalWrite(RTCGND, LOW);  // GND for RTC
  digitalWrite(RTCPWR, HIGH); // PWR for RTC

  // start clock
  Wire.begin();  // Begin I2C
  RTC.begin();   // begin clock

  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
    // DST? If we're in it, let's subtract an hour from the RTC time to keep our DST calculation correct. This gives us
    // Standard Time which our DST check will add an hour back to if we're in DST.
    DateTime standardTime = RTC.now();
    if (dst_rtc.checkDST(standardTime) == true) { // check whether we're in DST right now. If we are, subtract an hour.
      standardTime = standardTime.unixtime() - 3600;
    }
    RTC.adjust(standardTime);
  }

  // Clear screen buffer
  for (int sy=0; sy < 8; sy++) {
    for (int sx=0; sx < 8; sx++) {
      screenBuffer[sy][sx] = 0; //(sx + sy * 8) % (sizeof(screenPalette)/sizeof(uint16_t));
    }
  }

  matrix.begin();
  matrix.setBrightness(DAYBRIGHTNESS);
  matrix.fillScreen(0); // Initialize all pixels to 'off'
  matrix.show();

  // startup sequence... do colorwipe?
  //delay(500);
  //rainbowCycle(20);
  //delay(500);
  //flashWords(); // briefly flash each word in sequence
  //delay(500);
}

void loop() {
  // put your main code here, to run repeatedly:

  // get the time
  theTime = dst_rtc.calculateTime(RTC.now()); // takes into account DST
  // add 2.5 minutes to get better estimates
  //theTime = theTime.unixtime() + 150;

  adjustBrightness();
  //displayTime();

  //mode_moon(); // uncomment to show moon mode instead!
  scrollTime();

}



