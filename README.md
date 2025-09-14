# Ardui

Sketch de ejemplo para controlar un carrito con un controlador de motores
L298N mediante comandos enviados por el puerto serie.

## Comandos soportados

El archivo `Carrito-sensor-ultrasonido-Blue/carrito.ino` escucha un carácter y
realiza un movimiento corto:

- `F` &rarr; avance hacia adelante.
- `B` &rarr; retroceso.
- `L` &rarr; giro a la izquierda.
- `R` &rarr; giro a la derecha.

- `M` &rarr; activa/desactiva el modo automático de evasión de obstáculos usando un
  sensor ultrasónico montado sobre un servo.

Cualquier otro carácter detiene el carrito.
