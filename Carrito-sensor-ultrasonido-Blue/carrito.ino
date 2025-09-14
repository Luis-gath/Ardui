#include <Servo.h>

// pines servo y sensor
const int SERVO_PIN = 3;
const int TRIG_PIN = A2;
const int ECHO_PIN = A3;

// pines L298N
const int ENA = 5;
const int IN1 = 6;
const int IN2 = 7;
const int IN3 = 8;
const int IN4 = 9;
const int ENB = 11;

Servo s;
bool modo = false; // false = manual, true = auto

const int vel = 200;    // velocidad PWM 0-255
const int tPaso = 200;  // tiempo de cada paso (ms)
const int tGiro = 150;  // tiempo giro sobre eje (ms)
const int seg = 20;     // distancia segura (cm)

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  s.attach(SERVO_PIN);
  s.write(90);
  Serial.begin(9600);
  parar();
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'M') {
      modo = !modo;
      parar();
    } else if (!modo) {
      manual(c);
    }
  }

  if (modo) {
    autoRun();
  }
}

void manual(char c) {
  switch (c) {
    case 'A': pasoAdel(); break;
    case 'B': pasoAtras(); break;
    case 'C': pasoIzq(); break;
    case 'D': pasoDer(); break;
  }
}

void pasoAdel() {
  analogWrite(ENA, vel);
  analogWrite(ENB, vel);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  delay(tPaso);
  parar();
}

void pasoAtras() {
  analogWrite(ENA, vel);
  analogWrite(ENB, vel);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  delay(tPaso);
  parar();
}

void pasoIzq() {
  analogWrite(ENA, vel);
  analogWrite(ENB, vel);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  delay(tGiro);
  parar();
}

void pasoDer() {
  analogWrite(ENA, vel);
  analogWrite(ENB, vel);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  delay(tGiro);
  parar();
}

void parar() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

long medir() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long dur = pulseIn(ECHO_PIN, HIGH, 20000);
  long dist = dur / 58;
  return dist;
}

void autoRun() {
  long d = medir();
  if (d == 0 || d > seg) {
    adelante();
  } else {
    parar();
    s.write(150);
    delay(300);
    long izq = medir();
    s.write(30);
    delay(300);
    long der = medir();
    s.write(90);
    delay(300);
    if (izq > der) {
      pasoIzq();
    } else {
      pasoDer();
    }
  }
}

void adelante() {
  analogWrite(ENA, vel);
  analogWrite(ENB, vel);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}
