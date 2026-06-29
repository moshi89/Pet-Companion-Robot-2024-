/*
  Pet Companion Robot_Uno.ino
  Combined: Pet Feeder + Obstacle-Avoiding Car
  Target board: Arduino Uno

  Pin assignments (Uno-safe, 0-13 + A0-A5)
  ─────────────────────────────────────────
  Motor driver
    motorA1 → 2   motorA2 → 3
    motorB1 → 4   motorB2 → 5
    enA     → 10  enB     → 11

  Ultrasonic (HC-SR04)
    trig → 9   echo → 8

  LEDs (night light)   ← moved from 2,4 to free pins
    ledPin  → 6
    ledPin1 → 7

  LCD (4-bit mode)     ← moved from 44-49 to free pins
    
  Servo                ← pin 15 doesn't exist on Uno → A1
    servo → A1

  Switch               ← pin 14 = A0 on Uno
    switch → A0

  Photoresistor
    signal → A2

  DFPlayer Mini (SoftwareSerial) ← Uno has no Serial1
    TX → 13   RX → 12   (connect module: DFPlayer TX→12, RX→13)

  Potentiometer (track select)
    wiper → A3  ← 
*/

#include <LiquidCrystal.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include <DFPlayer_Mini_Mp3.h>

// ── Ultrasonic ──────────────────────────────────────────────────────
#define TRIG_PIN  9
#define ECHO_PIN  8

// ── Motor driver ────────────────────────────────────────────────────
#define MOTOR_A1  2
#define MOTOR_A2  3
#define MOTOR_B1  4
#define MOTOR_B2  5
#define EN_A      10
#define EN_B      11
const int MOTOR_SPEED = 120;   // 0-255

// ── LEDs (night light) ──────────────────────────────────────────────
#define LED_PIN   6
#define LED_PIN1  7
#define PHOTO_PIN A2

// ── LCD (parallel 4-bit) ────────────────────────────────────────────
// 
LiquidCrystal lcd(A3, A4, A5, 12, 13, 11);  // RS, EN, D4, D5, D6, D7

byte us1[8] = { B11111,B11111,B11111,B00000,B00000,B00000,B00000,B00000 };
byte us2[8] = { B11111,B11111,B11111,B11111,B11111,B11111,B11111,B11111 };
byte us3[8] = { B11111,B11111,B11111,B11111,B00000,B00000,B00000,B00000 };
byte us4[8] = { B11111,B11111,B11111,B11111,B11111,B00000,B00000,B00000 };
byte us5[8] = { B11111,B11111,B00000,B00000,B00000,B00000,B00000,B00000 };
byte us6[8] = { B11111,B11111,B00000,B00000,B00000,B00000,B10000,B11000 };
byte us7[8] = { B11111,B11111,B00000,B00000,B00000,B00000,B00001,B00011 };

// ── Servo & switch ──────────────────────────────────────────────────
#define SERVO_PIN  A1  // 
#define SWITCH_PIN A0  // 

Servo myServo;
bool  isFeeding  = false;

// ── DFPlayer (SoftwareSerial) ───────────────────────────────────────

SoftwareSerial dfSerial(12, 13);   // RX, TX (from Uno's perspective)

// ── Photoresistor state ─────────────────────────────────────────────
int           photoValue = 0;
unsigned long darkTime   = 0;
bool          ledOn      = false;

// ───────────────────────────────────────────────────────────────────
void setup() {
  // LCD
  lcd.createChar(0, us1);
  lcd.createChar(1, us2);
  lcd.createChar(2, us3);
  lcd.createChar(3, us4);
  lcd.createChar(4, us5);
  lcd.createChar(5, us6);
  lcd.createChar(6, us7);
  lcd.begin(16, 2);
  lcd.clear();

  // Servo
  myServo.attach(SERVO_PIN);
  myServo.write(0);

  // Switch & sensors & LEDs
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(PHOTO_PIN,  INPUT);
  pinMode(LED_PIN,    OUTPUT);
  pinMode(LED_PIN1,   OUTPUT);

  // Ultrasonic
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Motor driver
  pinMode(MOTOR_A1, OUTPUT);
  pinMode(MOTOR_A2, OUTPUT);
  pinMode(MOTOR_B1, OUTPUT);
  pinMode(MOTOR_B2, OUTPUT);
  pinMode(EN_A,     OUTPUT);
  pinMode(EN_B,     OUTPUT);

  // DFPlayer via SoftwareSerial
  Serial.begin(9600);
  dfSerial.begin(9600);
  mp3_set_serial(dfSerial);
  delay(1);
  mp3_set_volume(20);
}

// ───────────────────────────────────────────────────────────────────
void loop() {
  // 1. LCD animation (two frames, 500 ms each)
  drawLCDFrame1();
  delay(500);
  drawLCDFrame2();
  delay(500);

  // 2. Feeder switch
  if (digitalRead(SWITCH_PIN) == LOW && !isFeeding) {
    isFeeding = true;
    dispenseFood();
  }
  if (digitalRead(SWITCH_PIN) == HIGH) {
    isFeeding = false;
  }

  // 3. DFPlayer track select via potentiometer
  int potValue  = analogRead(A3);
  int track     = map(potValue, 0, 1023, 1, 5);
  static int lastTrack = 0;
  if (track != lastTrack) {
    mp3_play(track);
    lastTrack = track;
    delay(300);
  }

  // 4. Night-light (photoresistor → LEDs after 5 s of darkness)
  photoValue = analogRead(PHOTO_PIN);
  if (photoValue < 100) {
    if (darkTime == 0) darkTime = millis();
    if (millis() - darkTime >= 5000 && !ledOn) {
      digitalWrite(LED_PIN,  HIGH);
      digitalWrite(LED_PIN1, HIGH);
      ledOn = true;
    }
  } else {
    darkTime = 0;
    if (ledOn) {
      digitalWrite(LED_PIN,  LOW);
      digitalWrite(LED_PIN1, LOW);
      ledOn = false;
    }
  }

  // 5. Obstacle avoidance
  long dist = getDistance();
  if (dist > 10 && dist < 30) {
    moveForward();
  } else {
    stopMotors();
  }

  delay(100);
}

// ───────────────────────────────────────────────────────────────────
// LCD helpers
// ───────────────────────────────────────────────────────────────────
void drawLCDFrame1() {
  lcd.setCursor(2,0);  lcd.write(byte(0));
  lcd.setCursor(3,0);  lcd.write(byte(3));
  lcd.setCursor(4,0);  lcd.write(byte(1));
  lcd.setCursor(5,0);  lcd.write(byte(1));
  lcd.setCursor(6,0);  lcd.write(byte(1));
  lcd.setCursor(7,0);  lcd.write(byte(1));
  lcd.setCursor(8,0);  lcd.write(byte(1));
  lcd.setCursor(9,0);  lcd.write(byte(1));
  lcd.setCursor(10,0); lcd.write(byte(1));
  lcd.setCursor(11,0); lcd.write(byte(1));
  lcd.setCursor(12,0); lcd.write(byte(3));
  lcd.setCursor(13,0); lcd.write(byte(0));
  lcd.setCursor(5,1);  lcd.write(byte(0));
  lcd.setCursor(6,1);  lcd.write(byte(2));
  lcd.setCursor(7,1);  lcd.write(byte(3));
  lcd.setCursor(8,1);  lcd.write(byte(3));
  lcd.setCursor(9,1);  lcd.write(byte(2));
  lcd.setCursor(10,1); lcd.write(byte(0));
}

void drawLCDFrame2() {
  lcd.setCursor(2,0);  lcd.write(byte(0));
  lcd.setCursor(3,0);  lcd.write(byte(3));
  lcd.setCursor(4,0);  lcd.write(byte(5));
  lcd.setCursor(5,0);  lcd.write(byte(4));
  lcd.setCursor(6,0);  lcd.write(byte(4));
  lcd.setCursor(7,0);  lcd.write(byte(4));
  lcd.setCursor(8,0);  lcd.write(byte(4));
  lcd.setCursor(9,0);  lcd.write(byte(4));
  lcd.setCursor(10,0); lcd.write(byte(4));
  lcd.setCursor(11,0); lcd.write(byte(6));
  lcd.setCursor(12,0); lcd.write(byte(3));
  lcd.setCursor(13,0); lcd.write(byte(0));
  lcd.setCursor(5,1);  lcd.write(byte(0));
  lcd.setCursor(6,1);  lcd.write(byte(2));
  lcd.setCursor(7,1);  lcd.write(byte(3));
  lcd.setCursor(8,1);  lcd.write(byte(3));
  lcd.setCursor(9,1);  lcd.write(byte(2));
  lcd.setCursor(10,1); lcd.write(byte(0));
}

// ───────────────────────────────────────────────────────────────────
// Servo / feeder
// ───────────────────────────────────────────────────────────────────
void dispenseFood() {
  for (int pos = 0; pos <= 90; pos += 10) {
    myServo.write(pos);
    delay(5);
  }
  delay(1000);
  for (int pos = 90; pos >= 0; pos -= 10) {
    myServo.write(pos);
    delay(5);
  }
}

// ───────────────────────────────────────────────────────────────────
// Ultrasonic
// ───────────────────────────────────────────────────────────────────
long getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2;   // centimetres
}

// ───────────────────────────────────────────────────────────────────
// Motor control
// ───────────────────────────────────────────────────────────────────
void moveForward() {
  digitalWrite(MOTOR_A1, HIGH); digitalWrite(MOTOR_A2, LOW);
  digitalWrite(MOTOR_B1, HIGH); digitalWrite(MOTOR_B2, LOW);
  analogWrite(EN_A, MOTOR_SPEED);
  analogWrite(EN_B, MOTOR_SPEED);
}

void stopMotors() {
  digitalWrite(MOTOR_A1, LOW); digitalWrite(MOTOR_A2, LOW);
  digitalWrite(MOTOR_B1, LOW); digitalWrite(MOTOR_B2, LOW);
}
