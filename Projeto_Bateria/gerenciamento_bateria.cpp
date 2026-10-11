/**
 * SISTEMA DE GERENCIAMENTO DE RECARGA DE BATERIA PARA SMARTPHONES
 * Perfil CC-CV  |  Arduino Uno + Potenciometro + Slide Switch + Servo + LCD 16x2
 * 
 * LIGACOES UTILIZADAS
 *   Potenciometro (nivel da bateria) .... A0
 *   Slide Switch (cabo do carregador) ... D7  (HIGH = conectado, LOW = desconectado)
 *   Servo (amperimetro) ................. D9
 *   LCD (modo 4 bits) ................... RS=D12, E=D11, D4=D5, D5=D4, D6=D3, D7=D2
 *                                         (RW em GND, pot. de 10k no V0, LED+ com 220 ohm)
 */

#include <LiquidCrystal.h>
#include <Servo.h>

/**------------------------------
 * PINOS
 * ------------------------------ */
const int PIN_POT   = A0;
const int PIN_CHAVE = 7;
const int PIN_SERVO = 9;

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);  // RS, E, D4, D5, D6, D7
Servo servoCorrente;

/**------------------------------
 * PARAMETROS
 * ------------------------------ */
const unsigned long INTERVALO_LCD  = 100;  // ms - atualizacao do texto do LCD
const unsigned long INTERVALO_ANIM = 350;  // ms - troca de quadro de animação
const int       NUM_QUADROS    = 6;    // quadros do icone da bateria (0 a 5)
const int       COLUNA_ICONE   = 15;   // canto superior direito do LCD

/**------------------------------
 * ESTADO
 * ------------------------------ */
unsigned long tLcd  = 0;
unsigned long tAnim = 0;
int quadroAnim  = 0;
int anguloAtual     = -1;

/**------------------------------
 * ICONES DA BATERIA 
 * ------------------------------ */
void criarIconesBateria() {
    for (int k = 0; k < NUM_QUADROS; k++) {
        byte g[8];
        g[0] = 0b01110;
        g[1] = 0b11111;
        for (int r = 2; r <= 6; r++) {
            int linhaDeBaixo = 6 - r;
            g[r] = (linhaDeBaixo < k) ? 0b11111 : 0b10001;
        }
        g[7] = 0b11111;
        lcd.createChar(k, g);
    }
}

/**------------------------------
 * MODELAGEM DO SISTEMA
 * ------------------------------ */
int calcularAngulo(bool conectado, int carga) {
    if (!conectado || carga >= 100) return 0;       // corrente zero
    if (carga <= 80) return 180;                    // Fase CC
    return (int)(180.0f * (100 - carga) / 20.0f);   // Fase CV
}

int calcularTempoMin(int carga) {
    if (carga >= 100) return 0;
    if (carga <= 80) return (int)lroundf((80 - carga) * 1.5f + 60.0f);
    float d = 100 - carga;
    
    return (int)lroundf(0.15f * d * d);
}

/**------------------------------
 * LCD
 * ------------------------------ */
void escreverLinha(int linha, const char *texto, int largura) {
    char buf[17];
    snprintf(buf, sizeof(buf), "%-*s", largura, texto);

    lcd.setCursor(0, linha);
    lcd.print(buf);
}

void atualizarLCD(bool conectado, int carga) {
    char l1[17], l2[17];

    snprintf(l1, sizeof(l1), "Bateria: %d%%", carga);

    if (!conectado) {
        snprintf(l2, sizeof(l2), "Desconectado");
    } else if (carga >= 100) {
        snprintf(l2, sizeof(l2), "Carga Concluida");
    } else {
        snprintf(l2, sizeof(l2), "Tempo: %d min", calcularTempoMin(carga));
    }

    escreverLinha(0, l1, 15);
    escreverLinha(1, l2, 16);

    // ICONE
    int icone;
    if (conectado && carga < 100) {
        icone = quadroAnim;
    } else {
        icone = (int)((carga * (NUM_QUADROS - 1) + 50) / 100);
    }

    lcd.setCursor(COLUNA_ICONE, 0);
    lcd.write(icone);
}

/**------------------------------
 * SETUP / LOOP
 * ------------------------------ */
void setup() {
    pinMode(PIN_CHAVE, INPUT);
    servoCorrente.attach(PIN_SERVO);
    servoCorrente.write(0);

    lcd.begin(16, 2);
    criarIconesBateria();
}

void loop() {
    unsigned long agora = millis();

    // LEITURAS
    bool conectado = (digitalRead(PIN_CHAVE) == HIGH);
    int carga      = map(analogRead(PIN_POT), 0, 1023, 0, 100);

    // SERVO
    int angulo = calcularAngulo(conectado, carga);
    if (angulo != anguloAtual) {
        servoCorrente.write(angulo);
        anguloAtual = angulo;
    }

    // ANIMACAO
    if (conectado && carga < 100 && (agora - tAnim >= INTERVALO_ANIM)) {
        tAnim = agora;
        quadroAnim = (quadroAnim + 1) % NUM_QUADROS;
    }

    // LCD
    if (agora - tLcd >= INTERVALO_LCD) {
        tLcd = agora;
        atualizarLCD(conectado, carga);
    }
}