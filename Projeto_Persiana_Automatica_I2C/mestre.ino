#include <Wire.h>

const int pinoLDR = A0;

void setup() {
    Wire.begin();
}

void loop() {
    int leituraLDR = analogRead(pinoLDR);
    int angulo = map(leituraLDR, 0, 1023, 0, 180);

    Wire.beginTransmission(10);
    Wire.write(angulo);
    Wire.endTransmission();

    delay(50);
}