// ===== mega_rover.ino =====
// Motor control + DHT11 + HC-SR04 + ADXL345
// Talks to ESP32-CAM over Serial1
// Receives: F/B/L/R/S
// Sends: "T:.. H:.. D:.. P:.. R:.." every second

#include <DHT.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

// ---- Left L298N ----
#define L_IN1 22
#define L_IN2 23
#define L_IN3 24
#define L_IN4 25
#define L_ENA 8
#define L_ENB 9

// ---- Right L298N ----
#define R_IN1 26
#define R_IN2 27
#define R_IN3 46
#define R_IN4 47
#define R_ENA 6
#define R_ENB 7

// ---- Sensors ----
#define DHTPIN 41
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define TRIG_PIN 42
#define ECHO_PIN 40

Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

int motorSpeed = 200;        // 0-255
const int DANGER_CM = 10;    // failsafe threshold

char currentMove = 'S';      // tracks last movement command

void setup() {
  Serial.begin(9600);   // link to ESP32-CAM

  int outs[] = {L_IN1,L_IN2,L_IN3,L_IN4,L_ENA,L_ENB,
                R_IN1,R_IN2,R_IN3,R_IN4,R_ENA,R_ENB};
  for (int i = 0; i < 12; i++) pinMode(outs[i], OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  dht.begin();

  if (!accel.begin()) {
    Serial.println("ADXL345 not found");
  } else {
    accel.setRange(ADXL345_RANGE_2_G);
  }

  stopMotors();
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    handleCommand(cmd);
  }

  // failsafe: if moving forward and too close, stop
  if (currentMove == 'F') {
    long d = readDistanceCM();
    if (d > 0 && d < DANGER_CM) {
      stopMotors();
      currentMove = 'S';
    }
  }

  static unsigned long lastSensor = 0;
  if (millis() - lastSensor > 1000) {
    lastSensor = millis();
    sendSensorData();
  }
}

void handleCommand(char cmd) {
  switch (cmd) {
    case 'F':
      // block forward if too close
      {
        long d = readDistanceCM();
        if (d > 0 && d <  M) {
          stopMotors();
          currentMove = 'S';
        } else {
          forward();
          currentMove = 'F';
        }
      }
      break;
    case 'B': backward();  currentMove = 'B'; break;
    case 'L': turnLeft();  currentMove = 'L'; break;
    case 'R': turnRight(); currentMove = 'R'; break;
    case 'S': stopMotors();currentMove = 'S'; break;
  }
}

// ---- Movement ----
void leftSide(bool fwd) {
  analogWrite(L_ENA, motorSpeed);
  analogWrite(L_ENB, motorSpeed);
  digitalWrite(L_IN1, fwd);  digitalWrite(L_IN2, !fwd);
  digitalWrite(L_IN3, fwd);  digitalWrite(L_IN4, !fwd);
}

void rightSide(bool fwd) {
  analogWrite(R_ENA, motorSpeed);
  analogWrite(R_ENB, motorSpeed);
  digitalWrite(R_IN1, fwd);  digitalWrite(R_IN2, !fwd);
  digitalWrite(R_IN3, fwd);  digitalWrite(R_IN4, !fwd);
}

void forward()  { leftSide(true);  rightSide(true);  }
void backward() { leftSide(false); rightSide(false); }
void turnLeft() { leftSide(false); rightSide(true);  }
void turnRight(){ leftSide(true);  rightSide(false); }

void stopMotors() {
  analogWrite(L_ENA, 0); analogWrite(L_ENB, 0);
  analogWrite(R_ENA, 0); analogWrite(R_ENB, 0);
  digitalWrite(L_IN1, LOW); digitalWrite(L_IN2, LOW);
  digitalWrite(L_IN3, LOW); digitalWrite(L_IN4, LOW);
  digitalWrite(R_IN1, LOW); digitalWrite(R_IN2, LOW);
  digitalWrite(R_IN3, LOW); digitalWrite(R_IN4, LOW);
}

// ---- Sensors ----
long readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long dur = pulseIn(ECHO_PIN, HIGH, 30000);
  if (dur == 0) return -1;
  return dur * 0.0343 / 2;
}

void sendSensorData() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  long dist = readDistanceCM();

  // tilt from accelerometer
  sensors_event_t event;
  accel.getEvent(&event);
  float ax = event.acceleration.x;
  float ay = event.acceleration.y;
  float az = event.acceleration.z;
  float pitch = atan2(ax, sqrt(ay*ay + az*az)) * 180.0 / PI;
  float roll  = atan2(ay, sqrt(ax*ax + az*az)) * 180.0 / PI;

  // send to ESP32 (whole number distance)
  Serial.print("T:");
  Serial.print(isnan(t) ? -1 : (int)t);
  Serial.print(" H:");
  Serial.print(isnan(h) ? -1 : (int)h);
  Serial.print(" D:");
  Serial.print(dist);
  Serial.print(" P:");
  Serial.print((int)pitch);
  Serial.print(" R:");
  Serial.println((int)roll);
}