#include <Arduino.h>
#include <Servo.h>

// Pines para el controlador de motores L298N
const int ENA = 5;
const int IN1 = 6;
const int IN2 = 7;
const int IN3 = 8;
const int IN4 = 9;
const int ENB = 11;

// Pines para sensor ultrasónico y servo
const int TRIG_PIN = 12;
const int ECHO_PIN = 13;
const int SERVO_PIN = 10;

// Configuración de movimiento
const int vel = 200;    // velocidad PWM 0-255
const int tPaso = 200;  // duración de un paso hacia adelante o atrás (ms)
const int tGiro = 150;  // duración de un giro sobre su eje (ms)

const int distanciaSegura = 20;  // distancia mínima en cm

bool modoAutomatico = false;
Servo sensorServo;

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  sensorServo.attach(SERVO_PIN);
  sensorServo.write(90);

  Serial.begin(9600);
  parar();
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'M') {
      modoAutomatico = !modoAutomatico;
      if (!modoAutomatico) {
        parar();
      }
    } else if (!modoAutomatico) {
      switch (c) {
        case 'F': pasoAdel(); break;  // Adelante (Forward)
        case 'B': pasoAtras(); break; // Atrás (Backward)
        case 'L': pasoIzq(); break;   // Izquierda (Left)
        case 'R': pasoDer(); break;   // Derecha (Right)
        default: parar(); break;      // Cualquier otra tecla detiene el carro
      }
    }
  }
  if (modoAutomatico) {
    modoAuto();
  }
}

// Realiza un pequeño avance hacia adelante
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

// Realiza un pequeño retroceso
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

// Gira ligeramente a la izquierda
void pasoIzq() {
  analogWrite(ENA, vel);
  analogWrite(ENB, vel);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  delay(tGiro);
  parar();
}

// Gira ligeramente a la derecha
void pasoDer() {
  analogWrite(ENA, vel);
  analogWrite(ENB, vel);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  delay(tGiro);
  parar();
}

// Detiene ambos motores
void parar() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

long medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duracion = pulseIn(ECHO_PIN, HIGH, 20000); // espera máximo 20 ms
  return duracion * 0.034 / 2;
}

void modoAuto() {
  sensorServo.write(90);
  delay(100);
  long d = medirDistancia();
  if (d > distanciaSegura) {
    pasoAdel();
  } else {
    parar();
    sensorServo.write(150);
    delay(200);
    long dIzq = medirDistancia();
    sensorServo.write(30);
    delay(200);
    long dDer = medirDistancia();
    sensorServo.write(90);
    if (dIzq > dDer) {
      pasoIzq();
    } else {
      pasoDer();
    }
  }
}

