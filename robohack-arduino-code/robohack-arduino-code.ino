/* WIRING 
D7 Trig
D6 Echo
A1 Joy
20x4 Display A4/A5 i2C
D12 RED
D11 Yellow
D10  Green
*/

#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h> // include i/o class header

#include <IRremote.h>     
#include <Servo.h>

#define joystickPin A1
#define speakerPin 9
const int RECV_PIN = 2;

#define TRIG_PIN 7
#define ECHO_PIN 6

hd44780_I2Cexp lcd;
IRrecv irrecv(RECV_PIN);
decode_results results;
Servo mySpeaker;

const float maxDistance = 500; // cm
const float minDistance = 10; //cm

const float defaultDesiredDistance = 50; // cm 
const float warningDistance = 10; // cm
const float defaultDesiredDistanceRange = 75; // cm


float desiredDistance = 50; // cm
float desiredDistanceRange = 100; //cm

float duration, distance; 
String currentlyChanging = "Desired";
String currentLight = "none";

int deadzone = 64;

void setup() {
  Serial.begin(9600); //9600 is how fast we talk
  Serial.println("LCD Bootup");
  // initialize LCD with number of columns and rows:
  lcd.begin(20, 4);

  // Print a message to the LCD
  lcd.setCursor(1,0); //col 1 row 0
  lcd.print("Booting up...");

  Serial.println("Ultrasonic prep");
  pinMode(TRIG_PIN, OUTPUT); // trig pin (send info TO sesnor)
  pinMode(ECHO_PIN, INPUT); // echo pin (get info FROM sensor)

  Serial.println("Light prep");
  pinMode(10, OUTPUT); // green 
  pinMode(11, OUTPUT); // yellow
  pinMode(12, OUTPUT); // red
  
  Serial.println("IR Remote Bootup");
  irrecv.enableIRIn();
  irrecv.blink13(true);

  Serial.println("Speaker prep");
  mySpeaker.attach(speakerPin);
  Serial.println("Bootup complete");
}

void loop() {

  // read from ultrasonic sensor 
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH);
  distance = duration * 0.0343 /2; //convert to cm
  
  Serial.println(distance); // send distance back

  // read from and process joystick input 
  int joystickInput = analogRead(joystickPin);
  if (joystickInput > 512 + deadzone || joystickInput < 512 - deadzone) {
    if (currentlyChanging == "Desired") {
      desiredDistance -= ((float)(joystickInput) - 512.0) / 500.0;
      desiredDistance = clamp(desiredDistance, minDistance, maxDistance);
    } else if (currentlyChanging == "Warning") {
      desiredDistanceRange -= ((float)(joystickInput) - 512.0) / 500.0;
      desiredDistanceRange = clamp(desiredDistanceRange, minDistance, maxDistance - desiredDistance);
    }
  }
  

  if (distance < desiredDistance - warningDistance && joystickInput != 512) {
    mySpeaker.attach(speakerPin)
    mySpeaker.write(180);
    setLight("r");
    delay(100);
    mySpeaker.detach();
    setLight("none");
    delay(100);
  } else if (distance < desiredDistance + warningDistance) {
    setLight("r");
  } else if (distance < desiredDistance + desiredDistanceRange) {
    setLight("y");
  } else if (distance < maxDistance) {
    setLight("g");
  } else {
    setLight("none");
  }


  // read from and process IR input
  if (irrecv.decode(&results)){
    Serial.println(results.value, HEX);
    irrecv.resume();
    if (results.value == 0xFF02FD) { 
      desiredDistance = (int)distance;
      desiredDistance = clamp(desiredDistance, minDistance, maxDistance);   
    } else if (results.value == 0xFF906F) {
      desiredDistance += 5; 
      desiredDistance = clamp(desiredDistance, minDistance, maxDistance);
    } else if (results.value == 0xFFE01F) {
      desiredDistance -= 5; 
      desiredDistance = clamp(desiredDistance, minDistance, maxDistance);
    }
    
  }

  //display configuration to screen

  lcd.clear();
  lcd.setCursor(1,0); //col 1 row 0
  lcd.print("Desired Dist: " + String((int)desiredDistance) + "cm");
  lcd.setCursor(1,1); //col 1 row 1
  lcd.print("Warning Dist: " + String((int)warningDistance) + "cm");
  lcd.setCursor(1,2); //col 1 row 2
  lcd.print("Color: " + currentLight); 
  lcd.setCursor(1,3); 
  lcd.print("Reading: " + String((int)distance));
  delay(50);
}


float clamp(float input, int minimum, int maximum) {
    if (input > maximum) input = (float)maximum;
    if (input < minimum) input = (float)minimum; 
    return input; 
}


void setLight(String color) {
  if (currentLight != color) {
    if (currentLight == "none") {
      if (color == "r") digitalWrite(12, HIGH);  
      if (color == "y") digitalWrite(11, HIGH);
      if (color == "g") digitalWrite(10, HIGH);
    }
    if (currentLight == "r") {
      digitalWrite(12, LOW);
      if (color == "y") digitalWrite(11, HIGH);
      if (color == "g") digitalWrite(10, HIGH);
    }
    if (currentLight == "y") {
      digitalWrite(11, LOW);
      if (color == "r") digitalWrite(12, HIGH);
      if (color == "g") digitalWrite(10, HIGH);
    }
    if (currentLight == "g") {
      digitalWrite(10, LOW);
      if (color == "r") digitalWrite(12, HIGH);
      if (color == "y") digitalWrite(11, HIGH);
    }
    currentLight = color; 
    if (color == "none") {
      digitalWrite(10, LOW);
      digitalWrite(11, LOW);
      digitalWrite(12, LOW);
    }
  }  
}
