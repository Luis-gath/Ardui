#include <Arduino.h>
#include <Servo.h>

// ---------------- Pines L298N ----------------
const int ENA = 5;   // PWM motor izquierdo (motor derecho )
const int IN1 = 6;   // dir izq
const int IN2 = 7;
const int IN3 = 8;   // dir der
const int IN4 = 9;
const int ENB = 11;  // PWM motor derecho (motor izquierdo)

// ------------- Sensor y Servo ---------------
const int TRIG_PIN = A2;
const int ECHO_PIN = A3;
const int SERVO_PIN = 3;

// ------------ Velocidades/Tiempos -----------
const int VEL_MANUAL = 150;
const int VEL_AUTO   = 110;
const int T_PASO     = 200;  // ms
const int T_GIRO     = 170;  // ms (ligeramente mayor)
const int PWM_GIRO   = 170;  // PWM en giros sobre su eje

// ---------- Umbrales de seguridad -----------
const unsigned long TIMEOUT_US = 25000UL; // 25ms ~4m
const int DIST_STOP = 22;   // <=: frena YA (súbelo si aún “toca”)
const int DIST_SLOW = 40;   // zona de desaceleración
const int DIST_SAFE = 60;   // >: avance normal

// ------- Trim/calibración de motores --------
// Si el izq corre más: baja TRIM_L (ej. 0.90)
// Si el der corre más: baja TRIM_R
const float TRIM_L = 1.00;
const float TRIM_R = 0.90;

// -------- Estado y servo de escaneo ---------
bool modoAutomatico = false;
Servo sensorServo;

// ----------------- Helpers ------------------
int clip(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

void setMotorIzq(int pwm, bool forward) {
  pwm = clip(pwm, 0, 255);
  pwm = (int)clip((int)(pwm * TRIM_L), 0, 255);
  digitalWrite(IN1, forward ? HIGH : LOW);
  digitalWrite(IN2, forward ? LOW  : HIGH);
  analogWrite(ENA, pwm);
}

void setMotorDer(int pwm, bool forward) {
  pwm = clip(pwm, 0, 255);
  pwm = (int)clip((int)(pwm * TRIM_R), 0, 255);
  digitalWrite(IN3, forward ? HIGH : LOW);
  digitalWrite(IN4, forward ? LOW  : HIGH);
  analogWrite(ENB, pwm);
}

void parar() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

void adelantePWM(int pwm) {
  setMotorIzq(pwm, true);
  setMotorDer(pwm, true);
}

void atrasPWM(int pwm) {
  setMotorIzq(pwm, false);
  setMotorDer(pwm, false);
}

// --------- Movimientos manuales cortos -------
void pasoAdel() { adelantePWM(VEL_MANUAL); delay(T_PASO); parar(); }
void pasoAtras(){ atrasPWM(VEL_MANUAL);    delay(T_PASO); parar(); }
void pasoIzq()  { setMotorIzq(PWM_GIRO, true);
                  setMotorDer(PWM_GIRO, false);
                  delay(T_GIRO); parar(); }
void pasoDer()  { setMotorIzq(PWM_GIRO, false);
                  setMotorDer(PWM_GIRO, true);
                  delay(T_GIRO); parar(); }

// -------------- Sensor distancia -------------
unsigned int medirDistanciaCM_una() {
  digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long t = pulseIn(ECHO_PIN, HIGH, TIMEOUT_US);
  if (t == 0) return 0; // sin eco (muy lejos o sin objeto)
  unsigned int cm = (unsigned int)(t * 0.0343 / 2.0);
  if (cm > 500) cm = 500;
  return cm;
}

// Mediana de 3 (robusta). Trata 0 como “muy lejos” para anti-choque.
unsigned int distanciaMedianaCM() {
  unsigned int a = medirDistanciaCM_una();
  unsigned int b = medirDistanciaCM_una();
  unsigned int c = medirDistanciaCM_una();

  auto norm = [](unsigned int x){ return x == 0 ? 500u : x; };
  a = norm(a); b = norm(b); c = norm(c);

  // mediana simple
  unsigned int m = a;
  if ((a >= b && a <= c) || (a <= b && a >= c)) m = a;
  else if ((b >= a && b <= c) || (b <= a && b >= c)) m = b;
  else m = c;
  return m;
}

// Escaneo suave para elegir mejor ángulo
int escanearSuave() {
  unsigned int maxDist = 0;
  int mejorAng = 90;

  for (int ang = 30; ang <= 150; ang += 10) {
    sensorServo.write(ang);
    delay(60); // asentamiento del servo
    unsigned int d = distanciaMedianaCM(); // ya trata 0 como muy lejos
    if (d > maxDist) { maxDist = d; mejorAng = ang; }
  }
  sensorServo.write(90);
  delay(80);
  return mejorAng;
}

// Avanza en “micro-pasos” controlando velocidad y frenando si entra en zona STOP.
// Devuelve false si tuvo que frenar por seguridad.
bool avanzaConChequeo(unsigned long ms_total) {
  unsigned long t0 = millis();
  while (millis() - t0 < ms_total) {
    unsigned int d = distanciaMedianaCM();
    if (d <= DIST_STOP) { parar(); return false; }

    int pwm;
    if (d <= DIST_SLOW) {
      // Desacelera linealmente entre DIST_STOP..DIST_SLOW
      pwm = map((int)d, DIST_STOP + 1, DIST_SLOW, 90, VEL_AUTO);
      pwm = clip(pwm, 80, VEL_AUTO);
    } else {
      pwm = VEL_AUTO;
    }
    adelantePWM(pwm);
    delay(30); // paso corto y vuelve a medir
  }
  return true;
}

void modoAuto() {
  // Mirar al frente y medir de forma robusta
  sensorServo.write(90);
  delay(40);
  unsigned int d = distanciaMedianaCM();

  // Si aún está muy cerca, da un respiro hacia atrás
  if (d <= DIST_STOP + 3) {
    parar();
    atrasPWM(140);
    delay(140);
    parar();
  }

  // Si está libre (>= DIST_SAFE), avanzar con chequeo continuo
  if (d >= DIST_SAFE) {
    avanzaConChequeo(250);  // avanza ~0.25s con verificación recurrente
    return;
  }

  // Zona lenta: avanza pero controlado. Si no puede, escanea.
  if (d > DIST_STOP && d < DIST_SAFE) {
    bool pudo = avanzaConChequeo(220);
    if (pudo) return;
  }

  // Objeto cerca: detener, escanear y girar hacia el mejor ángulo
  parar();
  int angulo = escanearSuave();

  if (angulo < 85) {
    // gira un poco más si está muy hacia un lado
    setMotorIzq(PWM_GIRO, true);
    setMotorDer(PWM_GIRO, false);
    delay(map(90 - angulo, 0, 60, T_GIRO, T_GIRO + 120));
    parar();
  } else if (angulo > 95) {
    setMotorIzq(PWM_GIRO, false);
    setMotorDer(PWM_GIRO, true);
    delay(map(angulo - 90, 0, 60, T_GIRO, T_GIRO + 120));
    parar();
  } else {
    // centro: intenta avanzar suave
    adelantePWM(VEL_AUTO);
    delay(120);
    parar();
  }

  // Tras girar, avanza un poco con chequeo para confirmar que no va a chocar
  avanzaConChequeo(260);
}

// ----------------- Setup / Loop ----------------
void setup() {
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);

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
      if (!modoAutomatico) parar();
    } else if (!modoAutomatico) {
      switch (c) {
        case 'A': pasoAdel(); break;
        case 'B': pasoAtras(); break;
        case 'C': pasoIzq();   break;
        case 'D': pasoDer();   break;
        default:  parar();     break;
      }
    }
  }

  if (modoAutomatico) {
    modoAuto();
  }
}
