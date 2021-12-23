
/* SHOCK COUNTER v1.1*/
/*Use Keypad instead of buttons*/
#include <Keypad.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <avr/sleep.h>
#include <SparkFun_ADXL345.h>         // SparkFun ADXL345 Library

#define wakeInterruptPin 3
#define sensInterruptPin 2
#define OLED_RESET 4
#define pinLED 13
#define threshAnalogPin 1
#define timeAnalogPin 2

volatile uint32_t count = 0;
uint32_t maxcount = 100000;
uint8_t mode = 1;      // Modes: //0 - Sleep //1 - Normal //2 - Setting
uint8_t Hflag = 0;     // Hold flag: change count in 10, 100, 1000 steps
volatile uint8_t Iflag = 0;    // Interrupt flag
uint8_t firetime = 10;  // Time between shots in ms
uint8_t TapThresh = 32;  // (16|8|4|2) / 256 g per increment
const uint8_t TapDur = 20;    // (16|8|4|2) / 256 * 10 ms per increment
int8_t data_count = -1;
char Data[6] = {0, 0, 0, 0, 0, 0};
unsigned long long interrupTime = millis();
uint16_t lastcount = 0;

Adafruit_SSD1306 display(OLED_RESET);  //Communicate OLED
ADXL345 adxl = ADXL345(8);             // Accelerometer I2C Communication


void StarPress()
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

      maxcount = min ( atol(Data), 999999 );
      mode = 1;
      displaycount();
      delay(10);
      adxl.singleTapINT(1);
      attachInterrupt(digitalPinToInterrupt(sensInterruptPin), ADXL_ISR, RISING);   //turn on external interrupt again.
      clearData();
    }
  }
}
void HashtagLongPressStart()
{
  if (mode == 0)
  {
    mode = 1;
    welcome();
    displaycount();
    adxl.powerOn();
    adxl.singleTapINT(1);
    attachInterrupt(digitalPinToInterrupt(sensInterruptPin), ADXL_ISR, RISING);   // Attach Interrupt
  }
  else
  {
    mode = 0;
    detachInterrupt(digitalPinToInterrupt(sensInterruptPin));   // detach Interrupt
    adxl.singleTapINT(0);
    goodbye();
  }
}
void HashtagLongPressStop()
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

void welcome()
{
  digitalWrite(13, HIGH);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  //initialize with the I2C addr 0x3C (128x64)
  display.clearDisplay();  // clear the display before start
  display.setCursor(0, 0);  //SET CURSOR
  display.setTextSize(2);
  display.setTextColor(WHITE);

  display.println(" shock");
  display.println("   Counter");
  display.display();
  delay(1000);

  display.clearDisplay();
  display.setCursor(1, 1);
  display.setTextSize(1);
  display.println("By");
  display.setTextSize(2);
  display.setCursor(3, 15);
  display.print("Ali");
  display.setCursor(44, 15);
  display.print("Nourian");
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
    display.print(maxcount - count);
    display.setCursor(110, 10);
    display.setTextSize(1);
    display.println("RMD");

    display.setCursor(60, 25);
    display.setTextSize(1);
    display.print(count);
    display.setCursor(110, 22);
    display.setTextSize(1);
    display.println("CNT");
    display.display();
  }
  else if (mode == 2)
  {
    if (data_count == -1) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println(" Enter max shocks:");
      display.setTextSize(3);
      display.display();
    }
    else {
      display.setCursor(17 + 17 * data_count, 10);
      display.print(Data[data_count]);
      display.display();
    }
  }
}
/********************* ISR *********************/
/* Look for Interrupts and Triggered Action    */
void ADXL_ISR() {
  //detachInterrupt(digitalPinToInterrupt(sensInterruptPin));  // Turn Interrupts off
  cli();
  if (millis() - interrupTime > firetime) {
    Iflag = 1;
    count++;
    interrupTime = millis();
  }
  sei();
  adxl.getInterruptSource();  //Clear Interrupt Flag
}
/********************* KEYPAD *********************/
const byte rows = 4; //four rows
const byte cols = 3; //three columns
char keys[rows][cols] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[rows] = {12, 11, 10, 9}; //connect to the row pinouts of the keypad
byte colPins[cols] = {8, 7, 6}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );
//-------------------------------------------------
void setup()
{
  TapThresh = analogRead(threshAnalogPin) / 4;  // 45 degrees CW = add 0.4 g Threshold (max 25 rev or 25*360 deg = 16g)
  firetime =  analogRead(timeAnalogPin) / 4;  // 45 degrees CW = add 1.25 ms Fire time delay (max 25 rev or 25*360 deg = 255 ms)

  //Serial.begin(9600);
  //Serial.println("Testing your Analog buttons");
  //Serial.print("TapThresh: ");
  //Serial.println(TapThresh);
  adxl.setLowPower(0);
  adxl.setRate(3200);
  adxl.powerOn();                     // Power on the ADXL345
  adxl.setRangeSetting(16);            // Give the range settings
  // Accepted values are 2g, 4g, 8g or 16g
  // Higher Values = Wider Measurement Range
  // Lower Values = Greater Sensitivity
  adxl.setTapDetectionOnXYZ(1, 0, 0); // Detect taps in the directions turned ON "adxl.setTapDetectionOnX(X, Y, Z);" (1 == ON, 0 == OFF)
  // Set values for what is considered a TAP and what is a DOUBLE TAP (0-255)
  adxl.setTapThreshold(TapThresh);         // (16|8|4|2) / 256 g per increment
  adxl.setTapDuration(TapDur);            // (16|8|4|2) / 256 * 1000 μs per increment
  // Setting all interupts to take place on INT1 pin
  adxl.setImportantInterruptMapping(1, 1, 1, 1, 1);     // Sets "adxl.setEveryInterruptMapping(single tap, double tap, free fall, activity, inactivity);"
  // Accepts only 1 or 2 values for pins INT1 and INT2. This chooses the pin on the ADXL345 to use for Interrupts.
  // This library may have a problem using INT2 pin. Default to INT1 pin.

  // Turn on Interrupts for each mode (1 == ON, 0 == OFF)
  adxl.InactivityINT(0);
  adxl.ActivityINT(0);
  adxl.FreeFallINT(0);
  adxl.doubleTapINT(0);
  adxl.singleTapINT(1);
  //attaching interrupt
  attachInterrupt(digitalPinToInterrupt(sensInterruptPin), ADXL_ISR, RISING);   // Attach Interrupt


  keypad.addEventListener( keypadEvent );
  pinMode(13, OUTPUT);
  pinMode(sensInterruptPin, INPUT);
  pinMode(wakeInterruptPin, INPUT_PULLUP);
  digitalWrite(sensInterruptPin, LOW);

  count = 0;
  welcome();
  displaycount();
}

void loop()
{


  char key = keypad.getKey ();
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
    //digitalWrite(2,HIGH);
  }
  if (key != NO_KEY && key != '#' && key != '*' && mode == 2) {
    data_count++;
    Data[data_count] = key;
    displaycount();
  }

}
void keypadEvent ( KeypadEvent key )
{

  switch (keypad.getState())
  {
    case PRESSED:
      if (key == '*' && mode == 2) {
        StarPress();
      }
      if (key == '#' && mode == 2) {
        clearData();
        displaycount();
      }
      break;
    case RELEASED:
      if (key == '#' && mode == 0) {
        //HashtagLongPressStop();
      }
      break;
    case HOLD:
      if (key == '#') {
        //HashtagLongPressStart();
        count = 0;
        displaycount();
      }
      if (key == '*') {
        StarPress();
      }
      else if (mode == 1) {
        
      }
      break;
  }

}
void clearData()
{
  for (int i = 0; i < 6; i++) {
    Data [i] = 0;
  }
  data_count = -1;
}
