# Model Rocket Flight Computer with TVC Gimbal

This project is a flight computer for a model rocket, featuring thrust vector control (TVC) using two servos for engine gimbal, an MPU6050 IMU for orientation, and a BMP180 barometric sensor for altitude. The system runs on an ESP32 and provides real-time stabilization and telemetry.

## Features
- Thrust vector control (TVC) with 2-axis gimbal (2 servos)
- MPU6050 IMU for attitude estimation (complementary filter)
- BMP180 for altitude and launch detection
- PID control for stabilization
- State machine: IDLE, ARMED, BOOST, COAST, DESCENT, LANDED
- Status LEDs (green: OK, red: error/landed)
- Servo and system self-test at startup
- Serial telemetry output

## Hardware Connections

| Component      | ESP32 Pin | Notes                        |
|---------------|-----------|------------------------------|
| MPU6050 SDA   | 21        | I2C data                     |
| MPU6050 SCL   | 22        | I2C clock                    |
| BMP180 SDA    | 21        | I2C data (shared)            |
| BMP180 SCL    | 22        | I2C clock (shared)           |
| Servo X (Pitch) | 18      | PWM output                   |
| Servo Y (Yaw)   | 19      | PWM output                   |
| Green LED     | 17        | Status OK                    |
| Red LED       | 16        | Status Error/Landed          |
| 3.3V/5V, GND  | -         | Power for sensors/servos      |

**Note:** Use a separate power supply for servos if possible.

## Wiring Diagram

```
         +-------------------+
         |      ESP32        |
         |                   |
         |   21  SDA  <------+-----+ MPU6050
         |   22  SCL  <------+-----+ BMP180
         |                   |
         |   18  PWM  ------> Servo X (Pitch)
         |   19  PWM  ------> Servo Y (Yaw)
         |   17  ------->|--- Green LED
         |   16  ------->|--- Red LED
         |                   |
         +-------------------+
```

- Connect both MPU6050 and BMP180 to the same I2C bus (SDA 21, SCL 22).
- Connect servo signal wires to GPIO 18 and 19. Power servos from a suitable supply.
- Connect LEDs with resistors (220Ω recommended) to GPIO 16 and 17.

## Software Overview
- Written for ESP32 (Arduino framework)
- Uses Adafruit_MPU6050, Adafruit_BMP085, ESP32Servo libraries
- Implements a complementary filter for attitude
- PID control for TVC servos
- State machine for flight phases
- Serial output for telemetry

## Getting Started
1. Install required libraries in Arduino IDE:
   - Adafruit MPU6050
   - Adafruit BMP085
   - ESP32Servo
2. Connect hardware as per the diagram above.
3. Upload the code to your ESP32.
4. Open Serial Monitor at 115200 baud for telemetry.

## License
MIT
# mork1-flightcomp-v1
