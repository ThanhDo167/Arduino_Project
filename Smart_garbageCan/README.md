# Smart Trash Can

An automatic trash can lid opener based on Arduino UNO. An HC-SR04 ultrasonic sensor detects when a hand or object approaches, and a SG90 servo motor opens the lid automatically. Built with the Arduino framework using PlatformIO on VS Code.

---

## Table of Contents

1. [Features](#1-features)
2. [Hardware and Wiring](#2-hardware-and-wiring)
3. [How It Works](#3-how-it-works)
4. [Code Description](#4-code-description)
5. [Configuration Parameters](#5-configuration-parameters)
6. [Build and Upload](#6-build-and-upload)
7. [Serial Monitor](#7-serial-monitor)
8. [Limitations and Future Work](#8-limitations-and-future-work)

---

## 1. Features

- Automatically opens the lid when an object is detected within 20 cm.
- Holds the lid open for 3.5 seconds, then closes it automatically.
- Averages 3 consecutive distance readings before each decision to filter out spurious measurements.
- Calls `servo.detach()` after the lid closes to eliminate servo buzz and reduce power consumption while idle.
- Prints the averaged distance to the Serial Monitor every cycle for easy debugging and threshold tuning.

---

## 2. Hardware and Wiring

### Components

| Component | Qty | Function |
|---|---|---|
| Arduino UNO (ATmega328P) | 1 | Main microcontroller |
| SG90 servo motor | 1 | Opens and closes the lid |
| HC-SR04 ultrasonic sensor | 1 | Measures distance to detect approach |
| Jumper wires | — | Interconnects all components |
| 5 V power supply (USB or adapter) | 1 | Powers the entire system |

### Pin Mapping

| Arduino Pin | Connected to | Direction | Notes |
|---|---|---|---|
| Pin 5 | HC-SR04 TRIG | OUTPUT | Sends the trigger pulse |
| Pin 6 | HC-SR04 ECHO | INPUT | Receives the echo pulse |
| Pin 9 | SG90 Signal (orange wire) | PWM OUTPUT | Controls servo angle |
| 5 V | HC-SR04 VCC + SG90 VCC | — | 5 V power rail |
| GND | HC-SR04 GND + SG90 GND | — | Common ground |

### Wiring Diagram

```
Arduino UNO
┌──────────────────────┐
│                      │      ┌──────────────────┐
│  Pin 5  (TRIG) ──────┼─────►│  HC-SR04         │
│  Pin 6  (ECHO) ◄─────┼──────│  TRIG / ECHO     │
│  5 V           ──────┼─────►│  VCC             │
│  GND           ──────┼─────►│  GND             │
│                      │      └──────────────────┘
│                      │
│  Pin 9  (PWM)  ──────┼──────► SG90 Signal  (orange wire)
│  5 V           ──────┼──────► SG90 VCC     (red wire)
│  GND           ──────┼──────► SG90 GND     (brown / black wire)
└──────────────────────┘
```

> **Note:** The SG90 draws up to 250 mA at stall. When powered directly from the Arduino 5 V pin alongside other peripherals, total current draw may approach the 500 mA USB limit. If the board resets during servo movement, use a separate 5 V supply for the servo and connect only the ground to the Arduino.

---

## 3. How It Works

### Distance Measurement with HC-SR04

The HC-SR04 works by emitting an ultrasonic burst at 40 kHz through the TRIG pin (held HIGH for 10 µs) and measuring how long the echo takes to return on the ECHO pin. Distance is calculated as:

```
Distance (cm) = pulseIn(ECHO, HIGH) [µs] / 58
```

**Why divide by 58:**
Speed of sound ≈ 340 m/s = 0.034 cm/µs. The sound travels to the target and back, so the total path is twice the actual distance:

```
distance = (time × 0.034 cm/µs) / 2
         = time × 0.017 cm/µs
         = time / 58.82
         ≈ time / 58
```

### 3-Sample Averaging Filter

Each loop iteration takes 3 readings spaced 10 ms apart and uses their average:

```cpp
khcach_tb = (averDist[0] + averDist[1] + averDist[2]) / 3;
```

This rejects single-sample spikes caused by electrical noise, surface reflections at an angle, or the sensor occasionally returning 0 (timeout). Three samples at 10 ms intervals add only 20 ms of latency — imperceptible to the user.

### Lid Control State Machine

```
Object within 20 cm?
        │
   ┌────YES────┐
   │           │ NO
   ▼           ▼
servo.attach   servo.write(90°)   ← close lid
delay(1 ms)    delay(1000 ms)     ← wait for servo to reach position
servo.write(0°) ← open lid       servo.detach()  ← cut PWM signal
delay(3500 ms) ← hold open
[continue measuring]
```

### Why `servo.detach()` After Closing

When a servo is attached (`servo.attach()`), the Arduino continuously outputs a PWM signal to hold the shaft at the commanded angle. Even while the lid is stationary and no load is applied, the servo driver draws current and the motor may produce a faint buzz or jitter. Calling `servo.detach()` stops the PWM output entirely:

- Eliminates audible buzz when the lid is closed.
- Reduces idle current draw (the servo draws near zero when detached, versus 10–50 mA at rest with PWM active).
- Frees the timer resource and removes PWM noise from pin 9.

The lid stays closed by gravity or a mechanical stop — no PWM signal is needed to hold it.

---

## 4. Code Description

### `setup()`

```cpp
void setup() {
    Serial.begin(9600);        // Start serial for debug output

    pinMode(triPin, OUTPUT);   // TRIG: output — we send the pulse
    pinMode(EchPin, INPUT);    // ECHO: input  — we receive the echo

    servo.attach(servoPin);    // Attach servo to pin 9
    servo.write(closeAngle);   // Command lid to closed position (90°)
    delay(100);                // Wait for the servo to physically move
    servo.detach();            // Cut PWM — lid holds closed by gravity
}
```

On every power-on or reset, the system always commands the lid to the closed position first. This guarantees a known starting state regardless of where the servo shaft was left before power was removed.

### `loop()`

```cpp
void loop() {
    // Step 1: Take 3 readings 10 ms apart
    for (int i = 0; i <= 2; i++) {
        khcach = readDistance();
        averDist[i] = khcach;
        delay(10);
    }

    // Step 2: Average the readings
    khcach_tb = (averDist[0] + averDist[1] + averDist[2]) / 3;
    Serial.println(khcach_tb);   // Print to Serial Monitor for tuning

    // Step 3: Act on the result
    if (khcach_tb <= nguong_khcach) {
        // Object within threshold — open the lid
        servo.attach(servoPin);
        delay(1);                // Allow PWM output to stabilise before commanding angle
        servo.write(openAngle);  // 0° = fully open
        delay(3500);             // Hold open for 3.5 seconds
    } else {
        // Nothing nearby — close the lid
        servo.write(closeAngle); // 90° = fully closed
        delay(1000);             // Wait for the servo to complete the motion
        servo.detach();          // Stop PWM output
    }
}
```

**Note on `delay(1)` before `servo.write()`:** After `servo.attach()`, the PWM output takes one timer cycle (~20 ms period) to initialise. Without a short pause, the first `servo.write()` call may be lost or produce a partial movement. A 1 ms delay is sufficient because `pulseIn()` already spent tens of milliseconds measuring the echo.

### `readDistance()`

```cpp
float readDistance() {
    // Ensure TRIG starts from a clean LOW state
    digitalWrite(triPin, LOW);
    delayMicroseconds(2);

    // Send 10 µs trigger pulse
    digitalWrite(triPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(triPin, LOW);

    // Measure echo pulse width and convert to centimetres
    float distance = pulseIn(EchPin, HIGH) / 58.00;
    return distance;
}
```

`pulseIn(EchPin, HIGH)` blocks until the ECHO pin goes HIGH, then counts microseconds until it goes LOW again, returning the pulse width. Dividing by 58 converts that time to centimetres using the derivation shown in Section 3.

The function returns a `float` to preserve sub-centimetre precision, though integer arithmetic would be sufficient for a 20 cm threshold decision.

---

## 5. Configuration Parameters

All tunable values are declared as named constants at the top of `main.cpp`:

| Constant | Default | Description |
|---|---|---|
| `servoPin` | `9` | Arduino pin connected to SG90 signal wire |
| `openAngle` | `0` | Servo angle for fully open lid (degrees) |
| `closeAngle` | `90` | Servo angle for fully closed lid (degrees) |
| `triPin` | `5` | HC-SR04 TRIG pin |
| `EchPin` | `6` | HC-SR04 ECHO pin |
| `nguong_khcach` | `20` | Detection threshold distance (cm) |
| `delay(3500)` in loop | `3500 ms` | How long the lid stays open |

### Adjusting the Detection Distance

```cpp
const int nguong_khcach = 20;   // Change to desired distance in cm
```

Typical values:
- `15` cm — requires hand to be very close; good for narrow bin openings.
- `20` cm — default; comfortable for most waste bins.
- `30` cm — opens earlier; useful for large bins or users with limited mobility.

Use the Serial Monitor output to observe the actual measured distances in your installation environment and set the threshold accordingly.

### Adjusting How Long the Lid Stays Open

```cpp
delay(3500);   // Change to desired duration in milliseconds
```

- `2000` ms — faster cycle; suitable for quick, single-item disposal.
- `3500` ms — default; comfortable for most use cases.
- `5000` ms — stays open longer; useful when discarding multiple items at once.

### Adjusting Servo Angles

If the lid does not sit fully open or fully closed due to the mounting angle of the servo:

```cpp
const int openAngle  = 0;    // Increase if lid does not open far enough
const int closeAngle = 90;   // Adjust until lid closes flush
```

Typical servo range is 0°–180°. Adjust in 5° increments, observe the physical result, and repeat.

---

## 6. Build and Upload

### Requirements

- [Visual Studio Code](https://code.visualstudio.com/) — any recent version
- [PlatformIO IDE extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide) for VS Code
- USB cable (Type-A to Type-B) for Arduino UNO
- USB driver for the onboard USB-to-serial chip:
  - **CH340** — common on clone boards; download from manufacturer if not auto-detected
  - **ATmega16U2** — genuine Arduino boards; driver included with the Arduino IDE

### PlatformIO Configuration (`platformio.ini`)

```ini
[env:uno]
platform = atmelavr
board = uno
framework = arduino
lib_deps = arduino-libraries/Servo@^1.3.0
```

PlatformIO automatically downloads the `Servo` library on the first build. No manual library installation is required.

### Steps

1. Install VS Code and the PlatformIO IDE extension. Restart VS Code after installation.
2. Open VS Code. On the PlatformIO Home tab, select **Open Project** and navigate to the `SMART_CAN` folder.
3. Connect the Arduino UNO to the PC via USB.
4. Verify the COM port appears in Windows Device Manager under **Ports (COM & LPT)**. If it does not appear, install the appropriate USB driver.
5. Click **Build** (checkmark icon ✓ in the bottom toolbar) to compile. Expected: 0 errors, 0 warnings.
6. Click **Upload** (right-arrow icon →) to flash the firmware. PlatformIO auto-detects the COM port.
7. Click **Serial Monitor** (plug icon) and set the baud rate to **9600** to view distance readings.

### Troubleshooting Upload Errors

| Error message | Likely cause | Fix |
|---|---|---|
| `avrdude: ser_open(): can't open device` | Wrong or missing COM port | Check Device Manager; try a different USB cable |
| `avrdude: stk500_recv(): programmer not responding` | Board not detected | Press the RESET button on the UNO just before upload starts |
| Upload succeeds but servo does not move | Wiring error | Double-check pin 9 → SG90 signal; check 5 V and GND |

---

## 7. Serial Monitor

After uploading, open the Serial Monitor at **9600 baud**. The averaged distance (in cm) is printed once per loop cycle:

```
45
44
43
17    ← hand approaches, lid opens
16
15
15
46    ← hand withdrawn, lid closes after 3.5 s
47
45
```

### Using Serial Output for Calibration

1. Place the bin in its final installation position.
2. Hold your hand at the distance you want the lid to open, and note the Serial Monitor reading.
3. Set `nguong_khcach` to that value plus a small margin (2–3 cm) to avoid false triggers from objects passing nearby.
4. Rebuild and upload.

### Common Abnormal Readings

| Reading | Likely cause |
|---|---|
| `0` | Echo timeout — object too close (< 2 cm), too far (> 400 cm), or no echo returned |
| Erratic spikes | Reflective or angled surface; nearby interference from another ultrasonic source |
| Consistently lower than actual | Sensor aimed at a soft surface (fabric, foam) that absorbs part of the pulse |

---

## 8. Limitations and Future Work

### Current Limitations

**Blocking `delay()` calls** — The main loop is entirely blocked during the 3.5-second open period and the 1-second close wait. During these periods, the microcontroller cannot respond to any other event. If the user withdraws their hand immediately after the lid opens, the lid remains open for the full 3.5 seconds regardless. There is no way to interrupt or shorten the cycle once it starts.

**Open-loop servo control** — The system commands a servo angle and assumes the shaft reaches it. There is no position feedback. If the lid is physically obstructed (a bag caught under the lid, a mechanical jam), the servo will stall, draw excessive current, and potentially overheat, while the code continues as if the command succeeded.

**HC-SR04 measurement limitations** — The sensor performs poorly against soft, angled, or very small surfaces. It can also pick up reflections from objects other than the intended target (walls, furniture beside the bin). The 3-sample average reduces but does not eliminate outliers.

**No lid-state indicator** — There is no LED, buzzer, or display to indicate whether the lid is currently open or closed. Users must look at the bin directly.

**Single detection zone** — The sensor points in one fixed direction. If the bin is in a corner or against a wall, background objects may reduce the effective detection range or cause false positives.

### Future Improvements

- **Replace `delay()` with `millis()`** — Implement a non-blocking state machine so the loop can check whether the hand is still present and close the lid early if it is withdrawn before the timeout expires. This also allows adding other features (LED, button) without restructuring the code.

- **Add a status LED** — A simple green/red LED to indicate open/closed state gives users instant feedback without looking at the lid itself, which is useful when the bin is at floor level.

- **Add a fill-level sensor** — Mount an additional HC-SR04 or an infrared sensor inside the bin, pointing downward, to measure how full the bin is. When the fill level exceeds a threshold, trigger a buzzer or send a notification.

- **Add a push-button override** — A physical button on the side of the bin to open the lid manually, for situations where the ultrasonic sensor cannot detect the user (e.g., gloved hands, very slow approach).

- **Upgrade to ESP32** — Replace the Arduino UNO with an ESP32 to add Wi-Fi connectivity. The bin can then send push notifications to a smartphone when it is full, log usage data, or be controlled remotely via a simple web interface.

- **Use a stronger servo** — The SG90 provides approximately 1.8 kg·cm of torque, which may be insufficient for a heavier lid or a lid with a spring mechanism. Upgrading to an MG996R (10 kg·cm) or similar metal-gear servo improves reliability for larger bins.
