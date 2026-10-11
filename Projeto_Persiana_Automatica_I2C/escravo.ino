#include <Wire.h>
#include <Servo.h>

Servo persiana;
const int pinoServo = 9;
int angulo = 0;

void setup() {
    persiana.attach(pinoServo);
    
    Wire.begin(10);
    Wire.onReceive(recebe);
}

void loop() {
    delay(10);
}

void recebe(int numBytes) {
    if (Wire.available()) {
        angulo = Wire.read();
        persiana.write(angulo);
    }
}