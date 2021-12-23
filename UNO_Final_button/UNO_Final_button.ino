#include "OneButton.h"
#include <Adafruit_GFX.h> // Change Wire.h library to WSWire.h in all below libraries
#include <Adafruit_SSD1306.h>
#include <avr/sleep.h>
#include <SparkFun_ADXL345.h>         // ADXL345 Library
#include <WSWire.h>

#define wakeInterruptPin 3
#define sensInterruptPin 2
#define OLED_RESET 4
#define pinLED 13
#define upButtonPin 5
#define dnButtonPin 6
#define mdButtonPin 3
#define threshAnalogPin 6
#define timeAnalogPin 7

volatile uint16_t count = 0;
uint16_t maxcount = 2000;
uint8_t mode = 1;      // Modes: //0 - Sleep //1 - Normal //2 - Setting
uint8_t Hflag = 0;     // Hold flag: change count in 10, 100, 1000 steps
volatile uint8_t Iflag = 0;    // Interrupt flag
uint16_t firetime = 10;  // Time between shots in ms
uint8_t TapThresh = 32;  // (16|8|4|2) / 256 g per increment
const uint8_t TapDur = 20;    // (16|8|4|2) / 256 * 10 ms per increment
volatile unsigned long long interrupTime = millis();
uint16_t lastcount = 0;

Adafruit_SSD1306 display(OLED_RESET);  //Communicate OLED
ADXL345 adxl = ADXL345();             // Accelerometer I2C Communication

void UClick()
{
  if (mode != 0)
  {
    if (mode == 2 && maxcount < 10000) {
      maxcount++;
    }
    displaycount();
  }
}

void ULongPress()
{
  if (mode != 0)
  {
    if (mode == 2) {
      Hflag++;
      if (Hflag > 30 && maxcount < 9000)
      {
        maxcount = maxcount + 1000;
        delay(300);
      }
      else if (Hflag > 20 && maxcount < 9901)
      {
        maxcount = maxcount + 100;
        delay(200);
      }
      else if (Hflag > 10 && maxcount < 9991)
      {
        maxcount = maxcount + 10;
        delay(100);
      }
      else if (maxcount < 9999)
      {
        maxcount++;
        delay(50);
      }
    }
    else if (mode == 1)
    {
      count = 0;
    }
    displaycount();
  }
}
void ULongPressStop()
{
  Hflag = 0;
}

void DClick()
{
  if (mode != 0)
  {
    if (mode == 2 && maxcount < 10000) {
      maxcount--;
    }
    displaycount();
  }
}
void DLongPress()
{
  if (mode != 0)
  {
    if (mode == 2) {
      Hflag++;
      if (Hflag > 30 && maxcount > 1000)
      {
        maxcount = maxcount - 1000;
        delay(300);
      }
      else if (Hflag > 20 && maxcount > 100)
      {
        maxcount = maxcount - 100;
        delay(200);
      }
      else if (Hflag > 10 && maxcount > 10)
      {
        maxcount = maxcount - 10;
        delay(100);
      }
      else if (maxcount > 1)
      {
        maxcount--;
        delay(50);
      }
    }
    else if (mode == 1)
    {
      count = 0;
    }
    displaycount();
  }
}
void DLongPressStop()
{
  Hflag = 0;
}

void MClick()
{
  if (mode != 0)
  { // Change mode from normal to setting and vice versa
    if (mode == 1)
    {
      mode = 2;
      displaycount();
      detachInterrupt(digitalPinToInterrupt(sensInterruptPin));   //turn off external interrupt so it ignores gunshots.
      adxl.singleTapINT(0);
    }
    else
    {
      mode = 1;
      displaycount();
      delay(10);
      adxl.singleTapINT(1);
      attachInterrupt(digitalPinToInterrupt(sensInterruptPin), ADXL_ISR, RISING);   //turn on external interrupt again.
    }
  }
}
void MLongPressStart()
{
  if (mode == 0)
  {
    mode = 1;
    welcome();
    lastcount = 0;
    displaycount();
    adxl.powerOn();
    adxl.singleTapINT(1);
    attachInterrupt(digitalPinToInterrupt(sensInterruptPin), ADXL_ISR, RISING);   // Attach Interrupt
  }
  else
  {
    lastcount = 0;
    mode = 0;
    detachInterrupt(digitalPinToInterrupt(sensInterruptPin));   // detach Interrupt
    adxl.singleTapINT(0);
    goodbye();
  }
}
void MLongPressStop()
{
  if (mode == 0)
  {
    Going_To_Sleep();
  }
}
void Going_To_Sleep()
{
  adxl.setRegisterBit(ADXL345_POWER_CTL, 3, 0); //Put Sensor in Standby
  sleep_enable(); //enable sleep mode
  attachInterrupt(digitalPinToInterrupt(wakeInterruptPin), wakeup, LOW); // set pin 3 as wake up interrupt when it becomes zero (or CHANGE or RISING or FALLING)
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_cpu();
  Hflag = 0;
}
void wakeup()
{
  sleep_disable();
  delay(100);
  detachInterrupt(digitalPinToInterrupt(wakeInterruptPin));
}

// Setup Buttons on their pins.
OneButton Ubtn(upButtonPin, true);
OneButton Dbtn(dnButtonPin, true);
OneButton Mbtn(mdButtonPin, true);

void welcome()
{
  digitalWrite(13, HIGH);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  //initialize with the I2C addr 0x3C (128x64)
  display.clearDisplay();  // clear the display before start
  display.setCursor(0, 0);  //SET CURSOR
  display.setTextSize(2);
  display.setTextColor(WHITE);

  display.println(" Shock");
  display.println("   Counter");
  display.display();
  delay(1000);

  display.clearDisplay();
  display.setCursor(1, 1);
  display.setTextSize(1);
  display.println("By");
  display.setTextSize(2);
  display.setCursor(38, 15);
  display.print("Ali");
  display.display();
  delay(1000);

  display.clearDisplay();
  display.setCursor(16, 6);
  display.setTextSize(1);
  display.print("Threshold: ");
  display.print(TapThresh * 16 / 250);
  display.println(" g");
  display.setCursor(10, 20);
  display.print("Time Delay: ");
  display.print(firetime);
  display.println(" ms");
  display.display();
  delay(2000);
}
void goodbye()
{
  display.setCursor(0, 0);  //SET CURSOR
  display.setTextSize(2);
  display.clearDisplay();
  display.println(" Going to");
  display.print("  Sleep");
  display.display();
  delay(500);
  display.print(".");
  display.display();
  delay(500);
  display.print(".");
  display.display();
  delay(500);
  display.print(".");
  display.display();
  delay(500);
  display.clearDisplay();  // clear the display
  display.display();
  digitalWrite(13, LOW);
}
void displaycount()
{
  if (mode == 1)
  {
    if (count - lastcount > 30) {
      display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
      lastcount = count;
    }
    Wire.begin();
    delay(100);
    display.clearDisplay();
    display.setTextSize(3);
    display.setCursor(0, 0);
    display.println(maxcount - count);

    display.setTextSize(1);
    display.println("Remained");

    display.setCursor(79, 1);
    display.setTextSize(2);
    display.print(count);
    display.setCursor(82, 16);
    display.setTextSize(1);
    display.println("Counted");
    display.display();
  }
  else
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Set Remained shocks:");
    display.setTextSize(3);
    display.setCursor(28, 10);
    display.println(maxcount);
    display.display();
  }
}
/********************* ISR *********************/
/* Interrupts and Triggered Action */
void ADXL_ISR() {
  //detachInterrupt(digitalPinToInterrupt(sensInterruptPin));  // Turn Interrupts off
  //adxl.singleTapINT(0);
  cli();
  if (millis() - interrupTime > firetime) {
    Iflag = 1;
    count++;
    interrupTime = millis();
  }
  sei();
  //adxl.singleTapINT(1);      //Trun Interrupts on again
  adxl.getInterruptSource();  //Clear Interrupt Flag
}
void setup()
{
  TapThresh = analogRead(threshAnalogPin) / 4;  // 45 degrees CW = add 0.4 g Threshold (max 25 rev or 25*360 deg = 16g)
  firetime = analogRead(timeAnalogPin) / 4;  // 45 degrees CW = add 1.25 ms Fire time delay (max 25 rev or 25*360 deg = 255 ms)

  //Serial.begin(9600);
  //Serial.println("Testing your Analog buttons");
  //Serial.print("TapThresh: ");
  //Serial.println(TapThresh);
  //adxl.setLowPower(0);
  adxl.setRate(1600);
  adxl.powerOn();                      // Power on the ADXL345
  adxl.setRangeSetting(16);            // Give the range settings
  /* Accepted values are 2g, 4g, 8g or 16g
     Higher Values = Wider Measurement Range
     Lower Values = Greater Sensitivity */
  adxl.setTapDetectionOnXYZ(1, 0, 0); // Detect taps in the directions turned ON "adxl.setTapDetectionOnX(X, Y, Z);" (1 == ON, 0 == OFF)
  // Set values for what is considered a TAP and what is a DOUBLE TAP (0-255)
  adxl.setTapThreshold(TapThresh);         // (16|8|4|2) / 256 g per increment
  adxl.setTapDuration(TapDur);            // (16|8|4|2) / 256 * 1000 μs per increment
  // Setting all interupts to take place on INT1 pin
  adxl.setImportantInterruptMapping(1, 1, 1, 1, 1);     // Sets "adxl.setEveryInterruptMapping(single tap, double tap, free fall, activity, inactivity);"
  // Accepts only 1 or 2 values for pins INT1 and INT2. This chooses the pin on the ADXL345 to use for Interrupts.
  // This library may have a problem using INT2 pin. Default to INT1 pin.

  //adxl.getInterruptSource(); // clear adxl INT flags
  // Turn on Interrupts for each mode (1 == ON, 0 == OFF)
  adxl.InactivityINT(0);
  adxl.ActivityINT(0);
  adxl.FreeFallINT(0);
  adxl.doubleTapINT(0);
  adxl.singleTapINT(1);
  //attaching interrupt
  attachInterrupt(digitalPinToInterrupt(sensInterruptPin), ADXL_ISR, RISING);   // Attach Interrupt

  Ubtn.setPressTicks(300);
  Ubtn.attachClick(UClick);
  Ubtn.attachDuringLongPress(ULongPress);
  Ubtn.attachLongPressStop(ULongPressStop);
  Dbtn.setPressTicks(300);
  Dbtn.attachClick(DClick);
  Dbtn.attachDuringLongPress(DLongPress);
  Dbtn.attachLongPressStop(DLongPressStop);
  Mbtn.setPressTicks(300);
  Mbtn.attachClick(MClick);
  Mbtn.attachLongPressStart(MLongPressStart);
  Mbtn.attachLongPressStop(MLongPressStop);
  pinMode(13, OUTPUT);
  pinMode(upButtonPin, INPUT_PULLUP);
  pinMode(dnButtonPin, INPUT_PULLUP);
  pinMode(mdButtonPin, INPUT_PULLUP);
  pinMode(sensInterruptPin, INPUT_PULLUP);
  //digitalWrite(sensInterruptPin, LOW);
  //pinMode(A4, INPUT_PULLUP);  // set pull-up on analog pin 4
  //pinMode(A5, INPUT_PULLUP);  // set pull-up on analog pin 5

  count = 0;
  welcome();
  displaycount();
}

void loop()
{

  // keep watching the push buttons:
  Ubtn.tick();
  Dbtn.tick();
  Mbtn.tick();


  // Accelerometer Readings
  //int x,y,z;
  //adxl.readAccel(&x, &y, &z);         // Read the accelerometer values and store them in variables declared above x,y,z

  // Output Results to Serial
  /* UNCOMMENT TO VIEW X Y Z ACCELEROMETER VALUES */
  /* SCALE FACTORs:*/
  /* 31.2mg/LSB for 16g*/
  /* 15.6mg/LSB for 8g*/
  /* 7.8mg/LSB for 4g*/
  /* 3.9mg/LSB for 2g*/

  //  Serial.print(x*SCALEFACTOR);
  //  Serial.print(", ");
  //  Serial.print(y);
  //  Serial.print(", ");
  //  Serial.println(z);

  if (Iflag == 1)
  {
    Iflag = 0;
    displaycount();
    digitalWrite(A4, LOW);
    digitalWrite(A5, LOW);
    //digitalWrite(2,HIGH);
  }

  //  unsigned int value = analogRead(A0);
  //  Serial.println(value);
  //  delay(250);

}
