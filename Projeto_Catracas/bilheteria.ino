/*
  ============================================================
  BILHETERIA.INO - Arduino MESTRE (I2C)
  ============================================================
  Projeto: Controle de fluxo de passageiros (catracas) via I2C
  Placa: Arduino UNO - "UBILHETERIA"

  Funcoes:
   - Botao de SELECAO: alterna a catraca alvo (1 -> 2 -> 3 -> 1...)
   - Display de 7 segmentos: mostra a catraca selecionada
   - Botao de CONFIRMAR: envia comando de abertura via I2C
     exclusivamente para o endereco da catraca selecionada

  AJUSTE OS PINOS ABAIXO CONFORME A FIACAO REAL DO SEU CIRCUITO.
*/

#include <Wire.h>

// ---------- Configuracao de pinos ----------
const uint8_t PIN_BTN_SELECAO   = 9; // botao que escolhe a catraca (1,2,3)
const uint8_t PIN_BTN_CONFIRMAR = 10; // botao que confirma/libera o acesso

// Display de 7 segmentos - ordem: a, b, c, d, e, f, g
const uint8_t SEGMENTOS[7] = {2, 3, 4, 5, 6, 7, 8};
const bool CATODO_COMUM = false; // false = display de anodo comum

// ---------- Enderecos I2C dos escravos ----------
const uint8_t ENDERECO_CATRACA[3] = {0x08, 0x09, 0x0A}; // catraca 1, 2, 3
const uint8_t COMANDO_ABRIR = 0x01;

// ---------- Estado ----------
uint8_t catracaSelecionada = 0; // indice 0..2 -> catraca 1..3

bool ultimoEstadoSelecao = LOW;
bool ultimoEstadoConfirmar = LOW;
unsigned long ultimoDebounceSelecao = 0;
unsigned long ultimoDebounceConfirmar = 0;
const unsigned long DEBOUNCE_MS = 50;

// Padroes do display para os digitos 1, 2 e 3 (a,b,c,d,e,f,g)
const bool DIGITOS[3][7] = {
  {0, 1, 1, 0, 0, 0, 0}, // 1
  {1, 1, 0, 1, 1, 0, 1}, // 2
  {1, 1, 1, 1, 0, 0, 1}, // 3
};

void setup() {
  Wire.begin(); // mestre nao precisa de endereco proprio
  Serial.begin(9600);

  pinMode(PIN_BTN_SELECAO, INPUT);
  pinMode(PIN_BTN_CONFIRMAR, INPUT);

  for (uint8_t i = 0; i < 7; i++) {
    pinMode(SEGMENTOS[i], OUTPUT);
  }

  mostrarCatraca(catracaSelecionada);
  Serial.println(F("Bilheteria pronta."));
}

void loop() {
  verificarBotaoSelecao();
  verificarBotaoConfirmar();
}

void verificarBotaoSelecao() {
  bool leitura = digitalRead(PIN_BTN_SELECAO);
  if (leitura != ultimoEstadoSelecao) {
    ultimoDebounceSelecao = millis();
  }
  if ((millis() - ultimoDebounceSelecao) > DEBOUNCE_MS) {
    if (leitura == HIGH && ultimoEstadoSelecao == LOW) {
      catracaSelecionada = (catracaSelecionada + 1) % 3;
      mostrarCatraca(catracaSelecionada);
      Serial.print(F("Catraca selecionada: "));
      Serial.println(catracaSelecionada + 1);
    }
  }
  ultimoEstadoSelecao = leitura;
}

void verificarBotaoConfirmar() {
  bool leitura = digitalRead(PIN_BTN_CONFIRMAR);
  if (leitura != ultimoEstadoConfirmar) {
    ultimoDebounceConfirmar = millis();
  }
  if ((millis() - ultimoDebounceConfirmar) > DEBOUNCE_MS) {
    if (leitura == HIGH && ultimoEstadoConfirmar == LOW) {
      liberarAcesso(catracaSelecionada);
    }
  }
  ultimoEstadoConfirmar = leitura;
}

void liberarAcesso(uint8_t indice) {
  uint8_t endereco = ENDERECO_CATRACA[indice];

  Wire.beginTransmission(endereco);
  Wire.write(COMANDO_ABRIR);
  uint8_t resultado = Wire.endTransmission();

  Serial.print(F("Comando de abertura enviado para catraca "));
  Serial.print(indice + 1);
  Serial.print(F(" (end. 0x"));
  Serial.print(endereco, HEX);
  Serial.print(F(") - status: "));
  Serial.println(resultado == 0 ? F("OK") : F("FALHA"));
}

void mostrarCatraca(uint8_t indice) {
  for (uint8_t i = 0; i < 7; i++) {
    bool aceso = DIGITOS[indice][i];
    digitalWrite(SEGMENTOS[i], (aceso == CATODO_COMUM) ? HIGH : LOW);
  }
}
