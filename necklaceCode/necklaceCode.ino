#include <Adafruit_NeoPixel.h>

#define aref_voltage 3.3         // we tie 3.3V to ARef and measure it with a multimeter!

#define PIN      6
#define N_LEDS 32
Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_LEDS, PIN, NEO_GRB + NEO_KHZ800);


//TMP36 Pin Variables
const int tempPin = 0;        //the analog pin the TMP36's Vout (sense) pin is connected to
//the resolution is 10 mV / degree centigrade with a
//500 mV offset to allow for negative temperatures
int tempReading;        // the analog reading from the sensor

const int button1Pin = 20;//A2 on baord
const int button2Pin = 21;//A3 on board

long timerA = 0;

bool button1down = false;
bool button2down = false;
bool bothButtonsDown = false;
int state = 1;
//state key
//0 = leds off
//1 = random colours
//2 = health bar

int specialState = 0;
//specialState key
// 0 = none
// 1 = twitter
// 2 = flashlight = super bright leds

int brightness = 3;

//bluetoothstuff
#include <Arduino.h>
#include <SPI.h>
#if not defined (_VARIANT_ARDUINO_DUE_X_) && not defined (_VARIANT_ARDUINO_ZERO_)
#include <SoftwareSerial.h>
#endif
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "BluefruitConfig.h"
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);



void setup() {
  // We'll send debugging information via the Serial monitor
  delay(500);
  Serial.begin(115200);
  strip.begin();
  Serial.println("-----start-----");

  // If you want to set the aref to something other than 5v
  analogReference(EXTERNAL);

  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);

  if ( !ble.begin(VERBOSE_MODE) )
  {
    //error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }
  Serial.println( F("OK!") );

  ble.echo(false);
  ble.info();
  ble.verbose(false);


}

void loop() {

  pollForCommand();

  if (millis() > timerA)
  {
    timerA = millis() + 2000;

    updateLEDS();

    //temp stuff
    tempReading = analogRead(tempPin);
    //Serial.print("Temp reading = ");
    //Serial.print(tempReading);     // the raw analog reading
    // converting that reading to voltage, which is based off the reference voltage
    float voltage = tempReading * aref_voltage;
    voltage /= 1024.0;
    // print out the voltage
    // Serial.print(" - ");
    //Serial.print(voltage); Serial.print(" volts"); Serial.println(millis());
    // now print out the temperature
    float temperatureC = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset
    //to degrees ((volatge - 500mV) times 100)
    Serial.print(temperatureC); Serial.println(" degrees C");
  }


  int button1 = digitalRead(button1Pin);
  int button2 = digitalRead(button2Pin);

  if (button1 == LOW)
  {
    if (button1down == false)
    {
      Serial.println("button 1 pressed");
      if (state != 0)
      {
        state = 0;
        updateLEDS();
      } else if (state == 0) {
        state = 1;
        updateLEDS();
      }
    }
    button1down = true;
    delay(100);
  } else {
    button1down = false;
  }
  if (button2 == LOW)
  {
    if (button2down == false) {
      Serial.println("button 2 pressed");
      state++;
      if (state > 3) {
        state = 1;
      }
      updateLEDS();
    }
    button2down = true;
    delay(100);
  } else
  {
    button2down = false;
  }

  if (button2down && button1down && !bothButtonsDown)
  {
    Serial.println(Both buttons pressed);
    ble.print("AT+BLEUARTTX=");
    ble.println("BothButtonsPressed");
    bothButtonsDown = true;
    whiteFlash();
  } else
  {
    bothButtonsDown = false;
  }

}

static void updateLEDS() {
  if (specialState == 0)
  {
    if (state == 1)
      bar();//shows randon lights
    else if (state == 2)
    {
      TurnLEDSoff();
      healthBar(2); //shows health bar
    }
    else if (state == 0)
      TurnLEDSoff(); //turns off all leds
  }
  else if (specialState == 1)
  {
    blueFlash(); //twitter notification
  } else if (specialState == 2)
  {
    flashlight();
  }
}

static void bar() {
  uint32_t c;
  for (int i = 0; i < 8; i++) {
    c = strip.Color(random(1, brightness), random(1, brightness), random(1, brightness));
    //int timee = (millis()/500)%8;
    //c = strip.Color(timee*i*0.5,0,timee*i);
    int x = i;
    for (int j = 0; j < 4; j++)
    {
      strip.setPixelColor(x, c);
      x += 8;
    }
  }
  strip.show();
}

static void TurnLEDSoff() {
  uint32_t c = strip.Color(0, 0, 0);
  for (int i = 0; i < 32; i++) {
    strip.setPixelColor(i, c);
  }
  strip.show();

}

static void healthBar(int h) {
  uint32_t c;
  c = strip.Color(0, brightness, 0);
  for (int i = 0; i < 8; i++) {
    int x = i;
    if (i < h)
    {
      for (int j = 0; j < 4; j++)
      {
        strip.setPixelColor(x, c);
        x += 8;
      }
    }
  }
  strip.show();
}


static void blueFlash() {
  uint32_t c;
  c = strip.Color(0, 0, 40);
  for (int i = 0; i < 32; i++)
  {
    strip.setPixelColor(i, c);
  }
  strip.show();
  delay(200);
  TurnLEDSoff();
  delay(150);
  c = strip.Color(0, 0, 40);
  for (int i = 0; i < 32; i++)
  {
    strip.setPixelColor(i, c);
  }
  strip.show();
  delay(200);
  specialState = 0;
  updateLEDS();
}

static void whiteFlash() {
  uint32_t c;
  c = strip.Color(50, 50, 50);
  for (int i = 0; i < 32; i++)
  {
    strip.setPixelColor(i, c);
  }
  strip.show();
  delay(200);
  TurnLEDSoff();
  delay(150);
  c = strip.Color(50, 50, 40);
  for (int i = 0; i < 32; i++)
  {
    strip.setPixelColor(i, c);
  }
  strip.show();
  delay(200);
  specialState = 0;
  updateLEDS();
}

static void flashlight() {
  uint32_t c;
  c = strip.Color(250, 250, 250);
  for (int i = 0; i < 32; i++)
  {
    strip.setPixelColor(i, c);
  }
  strip.show();
}

//bluetooth stuffs
void pollForCommand() {
  // Check for user input
  char inputs[BUFSIZE + 1];

  if ( getUserInput(inputs, BUFSIZE) )
  {
    // Send characters to Bluefruit
    Serial.print("[Send] ");
    Serial.println(inputs);

    ble.print("AT+BLEUARTTX=");
    ble.println(inputs);

    // check response stastus
    if (! ble.waitForOK() ) {
      Serial.println(F("Failed to send?"));
    }
  }

  // Check for incoming characters from Bluefruit
  ble.println("AT+BLEUARTRX");
  ble.readline();
  if (strcmp(ble.buffer, "OK") == 0) {
    // no data
    return;
  }
  // Some data was found, its in the buffer
  Serial.print(F("[Recv] ")); Serial.println(ble.buffer);
  ble.waitForOK();
  //String command = ble.buffer;
  commandChecker(ble.buffer);
}

bool getUserInput(char buffer[], uint8_t maxSize)
{
  // timeout in 100 milliseconds
  TimeoutTimer timeout(100);

  memset(buffer, 0, maxSize);
  while ( (!Serial.available()) && !timeout.expired() ) {
    delay(1);
  }

  if ( timeout.expired() ) return false;

  delay(2);
  uint8_t count = 0;
  do
  {
    count += Serial.readBytes(buffer + count, maxSize);
    delay(2);
  } while ( (count < maxSize) && (Serial.available()) );

  return true;
}

void commandChecker(String com)
{
  if (state != 0) { //make sure commands dont turn on screen if off
    if (com == "tweet")
    {
      specialState = 1;
    } else if (com == "flashlightON")
    {
      specialState = 2;
    }
    else if (com == "flashlightOFF") {
      specialState = 0;
    }
     else if (com == "random") {
      state = 1;
    }else if (com == "healthbar") {
      state = 2;
    }

  }
}
