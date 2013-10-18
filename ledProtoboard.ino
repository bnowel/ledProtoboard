#include <SoftPWM.h>
// For the Accelerometer
#include <LowPower.h>
#include <SPI.h>
#include <ADXL362.h>

ADXL362 xl;
int interruptPin = 2;          //Setup ADXL362 interrupt output to Interrupt 0 (digital pin 2)
int interruptStatus = 0;


#define DELAY 300
//uint8_t leds[12] = {6,5,4,19,18,17,16,15,14,9,8,7};
uint8_t leds[12] = {18,19,17,15,16,14,8,9,7,5,6,4};

#define NUM_COLORS 4
// The list of possible colors
uint8_t colorsR[NUM_COLORS] = {255, 0, 0, 255};
uint8_t colorsG[NUM_COLORS] = {0, 255, 0, 0};
uint8_t colorsB[NUM_COLORS] = {0, 0, 255, 255};

int wait = DELAY;
int color = 0;

int LastX, LastY, LastZ;  
#define NUM_READS_TO_SKIP 10
int readsToSkip = 0;

void setColor(uint8_t r, uint8_t g, uint8_t b) {
    SoftPWMSet(leds[0], r);
    SoftPWMSet(leds[1], g);
    SoftPWMSet(leds[2], b);
    
    SoftPWMSet(leds[3], r);
    SoftPWMSet(leds[4], g);
    SoftPWMSet(leds[5], b);
    
    SoftPWMSet(leds[6], r);
    SoftPWMSet(leds[7], g);
    SoftPWMSet(leds[8], b);
    
    SoftPWMSet(leds[9], r);
    SoftPWMSet(leds[10], g);
    SoftPWMSet(leds[11], b);    
}

void setup()
{
  SoftPWMBegin();
  Serial.begin(9600);
  xl.begin(10);                //soft reset
    
  //  Setup Activity and Inactivity thresholds
  //     tweaking these values will effect the "responsiveness" and "delay" of the interrupt function
  //     my settings result in a very rapid, sensitive, on-off switch, with a 2 second delay to sleep when motion stops
  xl.setupDCActivityInterrupt(300, 10);		// 300 code activity threshold.  With default ODR = 100Hz, time threshold of 10 results in 0.1 second time threshold
  xl.setupDCInactivityInterrupt(80, 200);		// 80 code inactivity threshold.  With default ODR = 100Hz, time threshold of 30 results in 2 second time threshold

  //
  // Setup ADXL362 for proper autosleep mode
  //
	
  // Map Awake status to Interrupt 1
  // *** create a function to map interrupts... coming soon
  xl.SPIwriteOneRegister(0x2A, 0x40);   
	
  // Setup Activity/Inactivity register
  xl.SPIwriteOneRegister(0x27, 0x3F); // Referenced Activity, Referenced Inactivity, Loop Mode  
      
  // turn on Autosleep bit
  byte POWER_CTL_reg = xl.SPIreadOneRegister(0x2D);
  POWER_CTL_reg = POWER_CTL_reg | (0x04);				// turn on POWER_CTL[2] - Autosleep bit
  xl.SPIwriteOneRegister(0x2D, POWER_CTL_reg);
  xl.beginMeasure();                      // DO LAST! enable measurement mode  
  
  //
  // Setup interrupt function on Arduino
  //    IMPORTANT - Do this last in the setup, after you have fully configured ADXL.  
  //    You don't want the Arduino to go to sleep before you're done with setup
  //
  pinMode(2, INPUT);    
  attachInterrupt(0, interruptFunction, RISING);  // A high on output of ADXL interrupt means ADXL is awake, and wake up Arduino 
 // SoftPWMSetFadeTime(ALL, 50, 400);
}

#define MAX_DIFF 800
boolean moved(int curr, int last) {
  int diff;
  
  diff = curr - last;
  diff = abs(diff);
  
  return diff > MAX_DIFF;
}

void loop()
{
  int XValue, YValue, ZValue, Temperature, diff;
  
  //Serial.print('.');
  interruptStatus = digitalRead(interruptPin);

  // if ADXL362 is asleep, call LowPower.powerdown  
  if(interruptStatus == 0) { 
    setColor(0,0,0);
    delay(100);
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);     
  } else {
    delay(10);
    xl.readXYZTData(XValue, YValue, ZValue, Temperature);
    
    if (readsToSkip <= 0) {
      if (moved(XValue, LastX) || moved(YValue, LastY) || moved(ZValue, LastZ)) {
          setColor(colorsR[color], colorsG[color], colorsB[color]);
          color = (color + 1) % NUM_COLORS;
          
          // Wait a bit so we only change one color per shake
          delay(200);
          readsToSkip = NUM_READS_TO_SKIP;
      }
    } else {
      readsToSkip--;
    }
    
    
    LastX = XValue;
    LastY = YValue;
    LastZ = ZValue;
    //Serial.print(XValue); Serial.print(" ");
    //Serial.print(YValue); Serial.print(" ");
    //Serial.print(ZValue); Serial.print("\n");
  }
}

void interruptFunction(){
  Serial.println("\nArduino is Awake! \n");
}

