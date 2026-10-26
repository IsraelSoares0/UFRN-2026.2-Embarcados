/**
 * @brief Controle de Velocidade e Direção de um Veículo Elétrico em Miniatura
 * Arduino Uno - Simulador Tinkercad
 *   
 * @author Israel Soares
 * @date   26/10/2026
 */

#include <Servo.h>

// ---------------- Pinos ----------------
const uint8_t PINO_DIRECAO    = A0;
const uint8_t PINO_VELOCIDADE = A1;
const uint8_t PINO_BOTAO      = 2;
const uint8_t PINO_SERVO      = 9;
const uint8_t PINO_MOTOR      = 5;   // PWM

// ---------------- Parâmetros ----------------
const int PWM_BAIXO = 40;                      // PWM de partida (motor ligado, sem mover o veículo)
const int PASSO_PWM = 5;                       // incremento de PWM por atualização
const unsigned long INTERVALO_PWM_MS = 20;     // intervalo entre atualizações do PWM
const unsigned long DEBOUNCE_MS = 50;          // debounce do botão
const unsigned long INTERVALO_SERIAL_MS = 250; // período de impressão no Serial Monitor
const int TOLERANCIA_ZERO = 2;                 // leitura (0-1023) considerada "zero" no pot. de velocidade

// ---------------- Variáveis de estado ----------------
Servo servoDirecao;

bool motorLigado = false;
int  pwmAtual = 0;
int  pwmReferencia = 0;

int  anguloServo = 90;
int  anguloRodas = 0;

// Botão (debounce + detecção de borda)
bool leituraBrutaAnterior = HIGH;
bool estadoBotao = HIGH;
unsigned long tUltimaMudancaBotao = 0;

unsigned long tUltimoPwm = 0;
unsigned long tUltimoSerial = 0;

// ---------------- Funções ----------------

/**
 * @brief Verifica se o botão está pressionado
 * @return Retorna true se o botão foi pressionado
 */
bool botaoPressionado() {
  bool leitura = digitalRead(PINO_BOTAO);
  bool evento = false;

  if (leitura != leituraBrutaAnterior) {
    tUltimaMudancaBotao = millis();
    leituraBrutaAnterior = leitura;
  }

  if ((millis() - tUltimaMudancaBotao) > DEBOUNCE_MS && leitura != estadoBotao) {
    estadoBotao = leitura;
    if (estadoBotao == LOW) {
      evento = true;
    }
  }
  return evento;
}

/**
 * @brief Atualiza direção do servo
 */
void atualizaDirecao() {
  int leitura = analogRead(PINO_DIRECAO);
  int novoAngulo = map(leitura, 0, 1023, 135, 45);

  if (novoAngulo != anguloServo) {
    anguloServo = novoAngulo;
    servoDirecao.write(anguloServo);
  }
  anguloRodas = anguloServo - 90;
}

/**
 * @brief Baseado no estado do botão em "botaoPressionado()" Liga ou Desliga o Motor
 */
void tratarBotao() {
  if (!botaoPressionado()) return;

  if (!motorLigado) {
    // Só liga se o potenciômetro de velocidade estiver em zero
    if (analogRead(PINO_VELOCIDADE) <= TOLERANCIA_ZERO) {
      motorLigado = true;
      pwmAtual = PWM_BAIXO;              // partida SEM aceleração gradual
      analogWrite(PINO_MOTOR, pwmAtual);
      tUltimoPwm = millis();
      Serial.println(F(">> Motor LIGADO (baixa rotacao)"));
    } else {
      Serial.println(F(">> Partida negada: leve o pot. de velocidade a zero"));
    }
  } else {
    motorLigado = false;
    pwmAtual = 0;                        // desligamento imediato
    analogWrite(PINO_MOTOR, 0);
    Serial.println(F(">> Motor DESLIGADO"));
  }
}

/**
 * @brief Atualiza o estado do motor com base no potenciometro conectado em "PINO_VELOCIDADE"
 */
void atualizaMotor() {
  if (!motorLigado) {
    pwmReferencia = 0;
    return;
  }

  // Referência vinda do potenciômetro (0 a 255)
  int potRef = map(analogRead(PINO_VELOCIDADE), 0, 1023, 0, 255);
  pwmReferencia = max(potRef, PWM_BAIXO);

  if (millis() - tUltimoPwm >= INTERVALO_PWM_MS) {
    tUltimoPwm = millis();

    if (pwmAtual < pwmReferencia) {
      pwmAtual += PASSO_PWM;
      if (pwmAtual > pwmReferencia) pwmAtual = pwmReferencia;
    } else if (pwmAtual > pwmReferencia) {
      pwmAtual -= PASSO_PWM;
      if (pwmAtual < pwmReferencia) pwmAtual = pwmReferencia;
    }

    pwmAtual = constrain(pwmAtual, 0, 255);
    analogWrite(PINO_MOTOR, pwmAtual);
  }
}

/**
 * @brief Impressão de status do sistema
 */
void imprimeStatus() {
  if (millis() - tUltimoSerial < INTERVALO_SERIAL_MS) return;
  tUltimoSerial = millis();

  Serial.print(F("Motor: "));
  Serial.print(motorLigado ? F("ON ") : F("OFF"));
  Serial.print(F(" | PWM ref: "));
  Serial.print(pwmReferencia);
  Serial.print(F(" | PWM atual: "));
  Serial.print(pwmAtual);
  Serial.print(F(" | Servo: "));
  Serial.print(anguloServo);
  Serial.print(F(" deg | Rodas: "));
  Serial.print(anguloRodas);
  Serial.println(F(" deg"));
}

// ---------------- Setup / Loop ----------------
void setup() {
  Serial.begin(9600);

  pinMode(PINO_BOTAO, INPUT_PULLUP);   // nível estável HIGH quando solto
  pinMode(PINO_MOTOR, OUTPUT);
  analogWrite(PINO_MOTOR, 0);

  leituraBrutaAnterior = digitalRead(PINO_BOTAO);
  estadoBotao = leituraBrutaAnterior;
  tUltimaMudancaBotao = millis();

  servoDirecao.attach(PINO_SERVO);
  servoDirecao.write(90);   // rodas alinhadas ao iniciar
}

void loop() {
  atualizaDirecao();
  tratarBotao();
  atualizaMotor();
  imprimeStatus();
}
