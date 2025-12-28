// --- Motor Pin Mapping (ESP32 GPIO) ---
#define LEFT_MOTOR_IN1 8
#define LEFT_MOTOR_IN2 9
#define RIGHT_MOTOR_IN3 10
#define RIGHT_MOTOR_IN4 20
#define ENA 21  // Left PWM
#define ENB 3   // Right PWM

// --- Motor Control Functions ---
void setMotorSpeed(int ena, int enb) {
  analogWrite(ENA, ena);
  analogWrite(ENB, enb);
}

void moveForward(int speed = 200) {
  digitalWrite(LEFT_MOTOR_IN1, HIGH);
  digitalWrite(LEFT_MOTOR_IN2, LOW);
  digitalWrite(RIGHT_MOTOR_IN3, HIGH);
  digitalWrite(RIGHT_MOTOR_IN4, LOW);
  setMotorSpeed(speed, speed);
}

void moveBackward(int speed = 200) {
  digitalWrite(LEFT_MOTOR_IN1, LOW);
  digitalWrite(LEFT_MOTOR_IN2, HIGH);
  digitalWrite(RIGHT_MOTOR_IN3, LOW);
  digitalWrite(RIGHT_MOTOR_IN4, HIGH);
  setMotorSpeed(speed, speed);
}

void turnLeft(int speed = 200) {
  digitalWrite(LEFT_MOTOR_IN1, LOW);
  digitalWrite(LEFT_MOTOR_IN2, HIGH);
  digitalWrite(RIGHT_MOTOR_IN3, HIGH);
  digitalWrite(RIGHT_MOTOR_IN4, LOW);
  setMotorSpeed(speed, speed);
}

void turnRight(int speed = 200) {
  digitalWrite(LEFT_MOTOR_IN1, HIGH);
  digitalWrite(LEFT_MOTOR_IN2, LOW);
  digitalWrite(RIGHT_MOTOR_IN3, LOW);
  digitalWrite(RIGHT_MOTOR_IN4, HIGH);
  setMotorSpeed(speed, speed);
}

void stopMotors() {
  digitalWrite(LEFT_MOTOR_IN1, LOW);
  digitalWrite(LEFT_MOTOR_IN2, LOW);
  digitalWrite(RIGHT_MOTOR_IN3, LOW);
  digitalWrite(RIGHT_MOTOR_IN4, LOW);
  setMotorSpeed(0, 0);
}

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "eyes.h"

// ---------------- CONFIG ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_SDA 6
#define OLED_SCL 7



#define TOUCH_PIN 4
#define LDR_PIN   1
#define BUZZER_PIN 5


#define DARK_THRESHOLD 500
#define SHAKE_THRESHOLD 15.0

const unsigned long INTERACTION_LOCK_TIME = 3000;
const unsigned long NEGLECT_TIMEOUT = 60000;
const unsigned long SAD_TIMEOUT = 120000;
const unsigned long EYE_MOVE_INTERVAL = 3000;
const unsigned long MOOD_CYCLE_INTERVAL = 6000;

// ------------- OBJECTS ------------------
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_MPU6050 mpu;

// ------------- STATES -------------------
enum State { IDLE, LOVE, ANGRY, DIZZY, SLEEPING };
State currentState = IDLE;

// ------------- VARIABLES ----------------
int currentMood = 0;
int eyeOffsetX = 0;
int eyeOffsetY = 0;

unsigned long lastInteraction = 0;
unsigned long stateStartTime = 0;
unsigned long lastEyeMove = 0;
unsigned long lastMoodChange = 0;
unsigned long lastBlink = 0;
unsigned long nextBlinkInterval = 0;
unsigned long dizzyStartTime = 0;

bool wasTouched = false;
unsigned long touchStartTime = 0;
bool giggleActive = false;



// -------- FUNCTION PROTOTYPES -----------
void drawEyes(int moodID);
void forceMood(int moodID);
void blink();
void updateEyeOffset();
unsigned long getBlinkInterval(int mood);
void giggle(unsigned long now);

// ---------------------------------------
void setup() {
      // Motor pins
      pinMode(LEFT_MOTOR_IN1, OUTPUT);
      pinMode(LEFT_MOTOR_IN2, OUTPUT);
      pinMode(RIGHT_MOTOR_IN3, OUTPUT);
      pinMode(RIGHT_MOTOR_IN4, OUTPUT);
      pinMode(ENA, OUTPUT);
      pinMode(ENB, OUTPUT);
      stopMotors();
    // ...existing code...
  Serial.begin(115200);

  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;);
  }

  pinMode(TOUCH_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);


  if (!mpu.begin()) {
    Serial.println("MPU6050 not found!");
    for (;;);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  randomSeed(analogRead(0));

  lastInteraction = millis();
  forceMood(0);

  lastBlink = millis();
  nextBlinkInterval = getBlinkInterval(currentMood);
}

// ---------------------------------------
void loop() {
    // --- Motor reactions to emotions ---
    switch (currentState) {
      case LOVE:
        moveForward(255); // Move forward fast when happy/loved
        break;
      case ANGRY:
        turnLeft(255); // Spin in place when angry
        break;
      case DIZZY:
        turnRight(180); // Slow spin when dizzy
        break;
      case SLEEPING:
        stopMotors(); // Stop when sleeping
        break;
      case IDLE:
      default:
        stopMotors(); // Stop in idle
        break;
    }
  unsigned long now = millis();
  // ...existing code...


  // -------- SENSOR READS --------
  int lightLevel = analogRead(LDR_PIN);
  bool isTouched = digitalRead(TOUCH_PIN);

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float accelMag = sqrt(
    sq(a.acceleration.x) +
    sq(a.acceleration.y) +
    sq(a.acceleration.z)
  );

  float movement = abs(accelMag - 9.81);


  // -------- SLEEP LOGIC WITH DIZZY & BUZZER --------
  if (lightLevel < DARK_THRESHOLD) {
    if (currentState != SLEEPING && currentState != DIZZY) {
      currentState = DIZZY;
      dizzyStartTime = now;
      drawEyes(7);  // dizzy eyes
      // Buzzer: short beep for dizzy
      tone(BUZZER_PIN, 1200, 120);
      Serial.println("Getting dizzy...");
      return;
    }
    if (currentState == DIZZY) {
      drawEyes(7); // keep showing dizzy eyes
      if (now - dizzyStartTime > 1800) { // 1.8 seconds of dizzy
        currentState = SLEEPING;
        drawEyes(2);  // sleepy eyes
        // Buzzer: lower beep for sleep
        tone(BUZZER_PIN, 400, 200);
        Serial.println("Sleeping...");
      }
      return;
    }
    if (currentState == SLEEPING) {
      drawEyes(2); // keep showing sleepy eyes
      return;
    }
  } else if (currentState == SLEEPING || currentState == DIZZY) {
    currentState = IDLE;
    lastInteraction = now;
    forceMood(0);
    Serial.println("Woke up!");
    // Buzzer: quick wake beep
    tone(BUZZER_PIN, 900, 80);
  }

  // -------- TOUCH --------
  if (isTouched && !wasTouched) {
    currentState = LOVE;
    stateStartTime = now;
    lastInteraction = now;
    blink();
    drawEyes(6);  // love eyes
    // Cute love melody
    int loveNotes[4] = { 1400, 1800, 1600, 2000 };
    int loveDur[4] = { 70, 70, 70, 120 };
    for (int i = 0; i < 4; i++) {
      tone(BUZZER_PIN, loveNotes[i], loveDur[i]);
      delay(loveDur[i] + 20);
    }
    noTone(BUZZER_PIN);
    touchStartTime = now;
  }
  // Touch hold detection
  if (isTouched && wasTouched) {
    if (touchStartTime == 0) touchStartTime = now;
    if (!giggleActive && (now - touchStartTime > 2000)) {
      giggleActive = true;
      giggle(now);
      // Reset giggle after animation
      giggleActive = false;
      touchStartTime = 0;
    }
  }
  if (!isTouched) {
    touchStartTime = 0;
    giggleActive = false;
  }
  wasTouched = isTouched;
// ...existing code...

  // -------- SHAKE --------
  if (movement > SHAKE_THRESHOLD && currentState == IDLE) {
    currentState = ANGRY;
    stateStartTime = now;
    lastInteraction = now;
    blink();
    drawEyes(3);  // angry eyes
    // Harsh angry tone
    tone(BUZZER_PIN, 300, 180);
    delay(180);
    tone(BUZZER_PIN, 200, 120);
    delay(120);
    noTone(BUZZER_PIN);
  }

  // -------- EXIT LOCKED STATES --------
  if ((currentState == LOVE || currentState == ANGRY) &&
      (now - stateStartTime > INTERACTION_LOCK_TIME)) {
    currentState = IDLE;
    forceMood(currentMood);
  }

  // -------- IDLE BEHAVIOR --------
  if (currentState == IDLE) {

    // Blinking
    if (now - lastBlink > nextBlinkInterval) {
      blink();
      drawEyes(currentMood);
      lastBlink = now;
      nextBlinkInterval = getBlinkInterval(currentMood);
    }

    // Eye movement
    if (now - lastEyeMove > EYE_MOVE_INTERVAL) {
      updateEyeOffset();
      drawEyes(currentMood);
      lastEyeMove = now;
    }

    // Mood decay
    unsigned long idleTime = now - lastInteraction;

    if (idleTime > SAD_TIMEOUT) {
      if (currentMood != 5) forceMood(5);
    }
    else if (idleTime > NEGLECT_TIMEOUT) {
      if (currentMood != 4) forceMood(4);
    }
    else if (now - lastMoodChange > MOOD_CYCLE_INTERVAL) {
      int newMood = random(0, 2);
      if (newMood != currentMood) forceMood(newMood);
      lastMoodChange = now;
    }
  }
}

// -------- BLINK INTERVALS --------
unsigned long getBlinkInterval(int mood) {
  switch (mood) {
    case 1: return random(1800, 3200); // happy
    case 2: return random(6000, 9000); // sleepy
    case 5: return random(5000, 7000); // sad
    default: return random(3000, 5000); // neutral
  }
}

// -------- DISPLAY HELPERS --------
void drawMouth(int moodID, unsigned long now) {
  // Draw a cute mouth at a fixed position, style depends on mood
  int mx = 48;
  int my = 48;
  int mw = 32;
  int mh = 12;
  int frame = (now / 400) % 2; // animate open/close every 400ms
  int cx = mx + mw/2;
  int cy = my + mh/2;
  switch (moodID) {
    case 1: // happy
      // Smile: lower half of a circle
      for (int16_t i = -12; i <= 12; i++) {
        int y = cy + (int)(sqrt(1.0 - (i*i)/144.0) * 8);
        display.drawPixel(cx + i, y, WHITE);
        if (frame == 1) display.drawPixel(cx + i, y+2, WHITE);
      }
      break;
    case 5: // sad
      // Sad: upper half of a circle
      for (int16_t i = -12; i <= 12; i++) {
        int y = cy + 8 - (int)(sqrt(1.0 - (i*i)/144.0) * 8);
        display.drawPixel(cx + i, y, WHITE);
        if (frame == 1) display.drawPixel(cx + i, y-2, WHITE);
      }
      break;
    case 3: // angry
      // Flat or slightly downturned
      display.drawLine(mx+8, my+mh, mx+mw-8, my+mh, WHITE);
      if (frame == 1) display.drawLine(mx+10, my+mh+2, mx+mw-10, my+mh+2, WHITE);
      break;
    case 6: // love
      // Smile with dimples
      for (int16_t i = -12; i <= 12; i++) {
        int y = cy + (int)(sqrt(1.0 - (i*i)/144.0) * 8);
        display.drawPixel(cx + i, y, WHITE);
      }
      display.fillCircle(cx-6, cy+4, 2, WHITE);
      display.fillCircle(cx+6, cy+4, 2, WHITE);
      break;
    default: // neutral or other
      if (frame == 0) {
        // Gentle smile
        for (int16_t i = -10; i <= 10; i++) {
          int y = cy + (int)(sqrt(1.0 - (i*i)/100.0) * 5);
          display.drawPixel(cx + i, y, WHITE);
        }
      } else {
        display.drawLine(mx+10, my+mh, mx+mw-10, my+mh, WHITE);
      }
      break;
  }
}

void drawEyes(int moodID) {
  display.clearDisplay();

  int lx = 20 + eyeOffsetX;
  int rx = 74 + eyeOffsetX;
  int ly = 8 + eyeOffsetY;
  int ry = 8 + eyeOffsetY;

  // If moodID == 7, draw dizzy eyes (spiral or Xs)
  if (moodID == 7) {
    // Draw X eyes for dizzy (simple representation)
    // Left eye
    display.drawLine(lx+6, ly+6, lx+26, ly+26, WHITE);
    display.drawLine(lx+26, ly+6, lx+6, ly+26, WHITE);
    // Right eye
    display.drawLine(rx+6, ry+6, rx+26, ry+26, WHITE);
    display.drawLine(rx+26, ry+6, rx+6, ry+26, WHITE);
    // Optionally, draw a wavy mouth for dizzy
    int mx = 48;
    int my = 48;
    int mw = 32;
    int mh = 12;
    int cx = mx + mw/2;
    int cy = my + mh/2;
    for (int16_t x = -12; x <= 12; x++) {
      int y = cy + (int)(sin((x + (millis()/80)) * 0.4) * 4);
      display.drawPixel(cx + x, y, WHITE);
    }
    display.display();
    return;
  }

  display.drawBitmap(lx, ly, peyes[moodID][0][0], 32, 32, WHITE);
  display.drawBitmap(rx, ry, peyes[moodID][0][1], 32, 32, WHITE);
  drawMouth(moodID, millis());
  display.display();
}

void forceMood(int moodID) {
  currentMood = moodID;
  blink();
  drawEyes(currentMood);
}

void blink() {
  display.clearDisplay();
  display.drawBitmap(20, 8, eye0, 32, 32, WHITE);
  display.drawBitmap(74, 8, eye0, 32, 32, WHITE);
  display.display();
  delay(random(50, 100));
}

void updateEyeOffset() {
  int r = random(0, 5);
  eyeOffsetX = (r == 1) ? -5 : (r == 2) ? 5 : 0;
  eyeOffsetY = (r == 3) ? -4 : (r == 4) ? 4 : 0;
}

// -------- GIGGLE ANIMATION --------
void giggle(unsigned long now) {
  // Cute giggle melody (ascending-descending chirps)
  int giggleNotes[6] = { 1200, 1400, 1600, 1800, 1600, 1400 };
  int giggleDur[6] = { 60, 60, 60, 60, 60, 80 };
  for (int i = 0; i < 4; i++) {
    display.clearDisplay();
    // Eyes (closed or open)
    if (i % 2 == 0) {
      display.drawBitmap(20, 8, eye0, 32, 32, WHITE);
      display.drawBitmap(74, 8, eye0, 32, 32, WHITE);
    } else {
      display.drawBitmap(20, 8, peyes[1][0][0], 32, 32, WHITE);
      display.drawBitmap(74, 8, peyes[1][0][1], 32, 32, WHITE);
    }
    // Wavy mouth
    int mx = 48;
    int my = 48;
    int mw = 32;
    int mh = 12;
    int cx = mx + mw/2;
    int cy = my + mh/2;
    for (int16_t x = -12; x <= 12; x++) {
      int y = cy + (int)(sin((x + i*4) * 0.4) * 4);
      display.drawPixel(cx + x, y, WHITE);
    }
    display.display();
    // Play a giggle note
    tone(BUZZER_PIN, giggleNotes[i % 6], giggleDur[i % 6]);
    delay(giggleDur[i % 6] + 40);
  }
  noTone(BUZZER_PIN);
}
