# Smart Pet Companion Robot (2024)

A first-year Arduino project designed to provide interaction for pets in 1-person households.

## Core Features

- **Obstacle Avoidance:** Uses HC-SR04 ultrasonic sensor for reactive movement.
- **Audio Interaction:** Integrates DFPlayer Mini to provide companionship via voice clips.
- **Automated Environment:** Dynamic LED lighting triggered by ambient light sensors.
- **Automated Feeding:** Servo-driven food dispenser triggered by a push switch.
- **LCD Animation:** Custom character animation displayed on a 16×2 LCD screen.

## Tech Stack

- Arduino Uno
- L298N Motor Driver
- HC-SR04 Ultrasonic Sensor
- DFPlayer Mini
- Photoresistor
- SG90 Servo Motor
- 16×2 LCD Display

## System diagram

![System overview](docs/diagrams/system-overview.png)

## Loop flowchart

![Loop flowchart](docs/diagrams/loop-flowchart.png)

## How it works

### Setup (`setup()`)

Runs once on power-on. It initializes every subsystem in order: registers the 7 custom LCD characters, attaches the servo to pin A1 at 0°, configures all digital pins as INPUT or OUTPUT, starts `Serial` for debugging, starts `SoftwareSerial` on pins 12/13 for the DFPlayer, and sets the speaker volume to 20.

### Main loop — 5 stages every cycle

**Stage 1 — LCD animation.** Two custom character frames are drawn back to back, each held for 500 ms. The 7 custom `byte` patterns (`us1`–`us7`) are pixel art stored in the LCD's character RAM; `lcd.write(byte(n))` draws them at specific cursor positions to create a simple walking animation.

**Stage 2 — Food dispenser.** Pin A0 reads the push switch (`INPUT_PULLUP` means the pin is HIGH normally and goes LOW when pressed). When it goes LOW and `isFeeding` is false, `dispenseFood()` is called. That function sweeps the servo from 0° to 90° in 10° increments (opening a food gate), holds for 1 second, then sweeps back to 0° (closing it). The `isFeeding` flag prevents re-triggering while the switch is still held.

**Stage 3 — Music track selection.** A potentiometer on pin A3 gives a 0–1023 analog reading which is `map()`-ped to a track number 1–5. The DFPlayer only receives a new `mp3_play()` command when the track number actually changes, so it doesn't restart the same song constantly. Communication goes through `SoftwareSerial` (pins 12/13) since the Uno only has one hardware serial port (used for the USB monitor).

**Stage 4 — Night light.** The photoresistor on A2 outputs a low analog value in darkness (below 100). The code records the timestamp when darkness first starts using `millis()`. Only after 5 continuous seconds of darkness does it turn the LEDs on pins 6 and 7 HIGH. If light returns at any point, `darkTime` resets to 0 and the LEDs go off immediately. This prevents flickering from brief shadows.

**Stage 5 — Obstacle avoidance.** `getDistance()` fires a 10 µs pulse on the trigger pin (9) and times the echo return on pin 8. The formula `duration × 0.034 / 2` converts microseconds to centimetres (speed of sound ÷ 2 for round trip). If the distance is between 10 and 30 cm, both motors run forward via the L298N driver. Outside that range — either too close or nothing detected — `stopMotors()` cuts all motor signals.

## Pin reference

| Pin | Component | Role |
|-----|-----------|------|
| 2 | L298N IN1 | Motor A direction |
| 3 | L298N IN2 | Motor A direction |
| 4 | L298N IN3 | Motor B direction |
| 5 | L298N IN4 | Motor B direction |
| 6 | LED | Night light |
| 7 | LED | Night light |
| 8 | HC-SR04 Echo | Distance read |
| 9 | HC-SR04 Trig | Distance pulse |
| 10 | L298N ENA | Motor A speed (PWM) |
| 11 | L298N ENB | Motor B speed (PWM) |
| 12 | DFPlayer TX | SoftwareSerial RX |
| 13 | DFPlayer RX | SoftwareSerial TX |
| A0 | Push switch | Feeder trigger |
| A1 | Servo | Food gate |
| A2 | Photoresistor | Ambient light |
| A3 | Potentiometer | Track select |
| A4 | LCD EN | Display enable |
| A5 | LCD D4 | Display data |

## Project Context

This was my 2024 "Adventure Design" term project. It served as my introduction to hardware integration and basic state-based logic in embedded systems.
