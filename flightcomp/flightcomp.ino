#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_BMP085.h>
#include <ESP32Servo.h>

/* ---------------- PIN DEFINITIONS ---------------- */
#define LED_GREEN 17
#define LED_RED   16
#define SERVO_X_PIN 18
#define SERVO_Y_PIN 19

/* ---------------- OBJECTS ---------------- */
Adafruit_MPU6050 mpu;
Adafruit_BMP085 bmp;
Servo servoX, servoY;

/* ---------------- FLIGHT STATES ---------------- */
enum FlightState {
  IDLE,
  ARMED,
  BOOST,
  COAST,
  DESCENT,
  LANDED
};

FlightState state = IDLE;

/* ---------------- TIMING ---------------- */
unsigned long lastLoopTime = 0;
const unsigned long LOOP_INTERVAL_US = 5000; // 200 Hz

/* ---------------- IMU VARIABLES ---------------- */
float angleX = 0, angleY = 0;
float gyroXrate, gyroYrate;
float accRoll, accPitch;

/* ---------------- PID ---------------- */
float kp = 0.8, ki = 0.0, kd = 0.15;
float errorX, errorY, prevErrorX = 0, prevErrorY = 0;
float integralX = 0, integralY = 0;

/* ---------------- LAUNCH DETECTION ---------------- */
bool launchDetected = false;
unsigned long launchTime = 0;

/* ---------------- SETUP ---------------- */
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  if (!mpu.begin()) {
    Serial.println("MPU6050 FAIL");
    while (1);
  }

  if (!bmp.begin()) {
    Serial.println("BMP180 FAIL");
    while (1);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);


  servoX.attach(SERVO_X_PIN, 500, 2500);
  servoY.attach(SERVO_Y_PIN, 500, 2500);

  // Servo test: sweep both servos from 0 to 180 and back
  for (int pos = 0; pos <= 180; pos += 4) {
    servoX.write(pos);
    servoY.write(pos);
    delay(10);
  }
  for (int pos = 180; pos >= 0; pos -= 4) {
    servoX.write(pos);
    servoY.write(pos);
    delay(10);
  }
  servoX.write(90);
  servoY.write(90);

  digitalWrite(LED_GREEN, HIGH);
  delay(1000);
  digitalWrite(LED_GREEN, LOW);

  state = ARMED;
  Serial.println("SYSTEM ARMED");
}

/* ---------------- MAIN LOOP ---------------- */
void loop() {
  unsigned long now = micros();
  if (now - lastLoopTime < LOOP_INTERVAL_US) return;
  float dt = (now - lastLoopTime) / 1e6;
  lastLoopTime = now;

  sensors_event_t acc, gyro, temp;
  mpu.getEvent(&acc, &gyro, &temp);

  /* ---- ACCEL ANGLES ---- */
  accRoll  = atan2(acc.acceleration.y, acc.acceleration.z) * 180 / PI;
  accPitch = atan(-acc.acceleration.x /
             sqrt(acc.acceleration.y * acc.acceleration.y +
                  acc.acceleration.z * acc.acceleration.z)) * 180 / PI;

  /* ---- GYRO RATES ---- */
  gyroXrate = gyro.gyro.x * 180 / PI;
  gyroYrate = gyro.gyro.y * 180 / PI;

  /* ---- COMPLEMENTARY FILTER ---- */
  angleX = 0.98 * (angleX + gyroXrate * dt) + 0.02 * accRoll;
  angleY = 0.98 * (angleY + gyroYrate * dt) + 0.02 * accPitch;

  /* ---------------- FLIGHT STATE LOGIC ---------------- */
  float accZ = acc.acceleration.z;

  switch (state) {

    case ARMED:
      if (accZ > 3 * 9.81) { // Launch detection
        launchDetected = true;
        launchTime = millis();
        state = BOOST;
        Serial.println("🚀 LAUNCH DETECTED");
      }
      break;

    case BOOST:
      if (millis() - launchTime > 1500) { // Burnout estimate
        state = COAST;
        Serial.println("COAST PHASE");
      }
      break;

    case COAST:
      if (accZ < 0.5 * 9.81) {
        state = DESCENT;
        Serial.println("DESCENT");
      }
      break;

    case DESCENT:
      if (abs(accZ - 9.81) < 0.3) {
        state = LANDED;
        Serial.println("LANDED");
      }
      break;

    default:
      break;
  }

  /* ---------------- PID CONTROL (ALL STATES EXCEPT LANDED) ---------------- */
  if (state != LANDED) {
    errorX = angleX;
    errorY = angleY;

    integralX += errorX * dt;
    integralY += errorY * dt;

    float dX = (errorX - prevErrorX) / dt;
    float dY = (errorY - prevErrorY) / dt;

    float PIDX = kp * errorX + kd * dX;
    float PIDY = kp * errorY + kd * dY;

    PIDX = constrain(PIDX, -30, 30);
    PIDY = constrain(PIDY, -30, 30);

    servoX.write(constrain(90 - PIDX, 0, 180));
    servoY.write(constrain(90 - PIDY, 0, 180));

    prevErrorX = errorX;
    prevErrorY = errorY;
  } else {
    servoX.write(90);
    servoY.write(90);
  }

  /* ---------------- STATUS LED ---------------- */
  // Green always ON if everything OK, else red always ON
  if ((state == BOOST || state == COAST || state == ARMED || state == DESCENT) && !isnan(angleX) && !isnan(angleY)) {
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
  } else {
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, HIGH);
  }

  /* ---------------- TELEMETRY ---------------- */
  Serial.print("State: "); Serial.print(state);
  Serial.print(" | AngleX: "); Serial.print(angleX);
  Serial.print(" | AngleY: "); Serial.print(angleY);
  Serial.print(" | AccZ: "); Serial.println(accZ);
}
