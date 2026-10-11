/*
  ============================================================
  CATRACA.INO - Arduino ESCRAVO (I2C)
  ============================================================
  Grave o MESMO arquivo nas 3 placas "escravo" (catracas),
  alterando apenas o valor de NUMERO_DA_CATRACA abaixo (1, 2 ou 3)
  antes de cada upload.

  Funcoes:
   - Recebe comando de abertura do mestre via Wire.onReceive()
   - LED RGB: vermelho = bloqueado, verde = liberado
   - Servo: 0 graus fechado, 90 graus aberto
   - Botao local simula a passagem do usuario: ao ser pressionado
     (com a catraca aberta), fecha novamente (servo -> 0,
     LED -> vermelho) e volta a aguardar um novo comando

  AJUSTE OS PINOS CONFORME A FIACAO REAL DO SEU CIRCUITO.
*/

#include <Wire.h>
#include <Servo.h>

// ALTERE PARA 1, 2 OU 3 EM CADA PLACA ANTES DE GRAVAR
const uint8_t NUMERO_DA_CATRACA = 1;

// Enderecos I2C - precisam bater com ENDERECO_CATRACA[] do bilheteria.ino
const uint8_t ENDERECOS[3] = {0x08, 0x09, 0x0A};
const uint8_t MEU_ENDERECO = ENDERECOS[NUMERO_DA_CATRACA - 1];

const uint8_t COMANDO_ABRIR = 0x01;

// ---------- Pinos ----------
const uint8_t PIN_SERVO = 5;
const uint8_t PIN_LED_R = 12;
const uint8_t PIN_LED_G = 11;
const uint8_t PIN_LED_B = 6;
const uint8_t PIN_BOTAO = 13; // sensor de passagem (local)

const uint8_t ANGULO_FECHADO = 0;
const uint8_t ANGULO_ABERTO  = 90;

Servo servoCatraca;

volatile bool comandoAbrirRecebido = false; // sinalizado dentro do onReceive
bool catracaAberta = false;

bool ultimoEstadoBotao = LOW;
unsigned long ultimoDebounce = 0;
const unsigned long DEBOUNCE_MS = 50;

void setup() {
  Serial.begin(9600);

  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  pinMode(PIN_BOTAO, INPUT);

  servoCatraca.attach(PIN_SERVO);
  fecharCatraca(); // estado inicial: bloqueado (vermelho / 0 graus)

  Wire.begin(MEU_ENDERECO);
  Wire.onReceive(aoReceberComando);

  Serial.print(F("Catraca "));
  Serial.print(NUMERO_DA_CATRACA);
  Serial.print(F(" pronta - endereco I2C 0x"));
  Serial.println(MEU_ENDERECO, HEX);
}

void loop() {
  // O comando I2C chega dentro de uma "interrupcao" do Wire; a acao
  // pesada (mover servo) e feita aqui no loop principal, nao no callback.
  if (comandoAbrirRecebido) {
    comandoAbrirRecebido = false;
    abrirCatraca();
  }

  verificarBotaoPassagem();
}

// Chamado automaticamente pela biblioteca Wire quando o mestre
// envia dados para o endereco desta placa
void aoReceberComando(int quantidadeBytes) {
  while (Wire.available()) {
    uint8_t comando = Wire.read();
    if (comando == COMANDO_ABRIR) {
      comandoAbrirRecebido = true;
    }
  }
}

void verificarBotaoPassagem() {
  bool leitura = digitalRead(PIN_BOTAO);
  if (leitura != ultimoEstadoBotao) {
    ultimoDebounce = millis();
  }
  if ((millis() - ultimoDebounce) > DEBOUNCE_MS) {
    if (leitura == HIGH && ultimoEstadoBotao == LOW && catracaAberta) {
      Serial.println(F("Passagem detectada - fechando catraca."));
      fecharCatraca();
    }
  }
  ultimoEstadoBotao = leitura;
}

void abrirCatraca() {
  Serial.println(F("Comando de abertura recebido - liberando."));
  setLedRGB(0, 255, 0); // verde
  servoCatraca.write(ANGULO_ABERTO);
  catracaAberta = true;
}

void fecharCatraca() {
  setLedRGB(255, 0, 0); // vermelho
  servoCatraca.write(ANGULO_FECHADO);
  catracaAberta = false;
}

void setLedRGB(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(PIN_LED_R, r);
  analogWrite(PIN_LED_G, g);
  analogWrite(PIN_LED_B, b);
}
