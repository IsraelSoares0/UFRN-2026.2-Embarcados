/*
 * Relógio Digital com Arduino Uno + LCD 16x2 (I2C) - versão para Tinkercad
 * Sistemas Embarcados - UFRN
 *
 * Ligações:
 *   LCD I2C         : SDA -> A4 | SCL -> A5 | VCC -> 5V | GND -> GND
 *   Botão de ajuste : D2 <-> 5V, com resistor de 10k de D2 para GND (pull-down)
 *   Botão de avanço : D3 <-> 5V, com resistor de 10k de D3 para GND (pull-down)
 *
 *   Com pull-down, o pino lê HIGH quando o botão está pressionado.
 *   (Se ligar os botões em GND sem resistor, use INPUT_PULLUP e NIVEL_PRESSIONADO = LOW.)
 */

#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x20, 16, 2);  // endereço 0x20 (padrão do Tinkercad), 16 colunas, 2 linhas

// ======================= Configurações =======================
const uint8_t PINO_AJUSTE = 2;
const uint8_t PINO_AVANCO = 3;
const uint8_t NIVEL_PRESSIONADO = HIGH;  // HIGH para pull-down externo; LOW para INPUT_PULLUP

const unsigned long INTERVALO_SEGUNDO   = 1000UL;  // base de tempo do relógio
const unsigned long TEMPO_DEBOUNCE      = 30UL;    // entre 20 e 50 ms
const unsigned long TEMPO_PRESSAO_LONGA = 1000UL;  // >= 1 s = pressão longa
const unsigned long INTERVALO_PISCA     = 500UL;   // meio período do pisca

const uint8_t COLUNA_HORA = 4;  // "HH:MM:SS" (8 caracteres) centralizado em 16 colunas

// ======================= Estados =======================
const uint8_t MODO_RELOGIO = 0;
const uint8_t MODO_AJUSTE  = 1;

const uint8_t CAMPO_SEGUNDOS = 0;
const uint8_t CAMPO_MINUTOS  = 1;
const uint8_t CAMPO_HORAS    = 2;

uint8_t modo  = MODO_RELOGIO;
uint8_t campo = CAMPO_SEGUNDOS;

// ======================= Hora =======================
uint8_t horas = 0, minutos = 0, segundos = 0;  // inicia em 00:00:00
unsigned long tAnterior = 0;                   // instante da última atualização

// ======================= Pisca / tela =======================
bool piscaVisivel = true;
unsigned long tPisca = 0;
bool redesenharTitulo = true;
bool redesenharHora   = true;

// ======================= Botões (com debounce) =======================
struct Botao {
    uint8_t pino;
    bool ultimaLeitura;          // última leitura crua (true = pressionado)
    bool estavel;                // estado validado após debounce
    unsigned long tMudanca;      // instante da última variação da leitura crua
    unsigned long tPressionado;  // instante em que o estado estável virou "pressionado"
    unsigned long duracao;       // duração do último pressionamento (válida em evSoltou)
    bool evPressionou;           // transição liberado -> pressionado (1 ciclo)
    bool evSoltou;               // transição pressionado -> liberado (1 ciclo)
};

// Índices no vetor de botões. As funções recebem o índice (uint8_t) em vez
// de "Botao&", pois o Tinkercad gera os protótipos antes da struct existir.
const uint8_t BTN_AJUSTE = 0;
const uint8_t BTN_AVANCO = 1;

Botao botoes[2] = {
    { PINO_AJUSTE, false, false, 0, 0, 0, false, false },
    { PINO_AVANCO, false, false, 0, 0, 0, false, false }
};

// Lê o botão e gera eventos sem bloquear.
void atualizarBotao(uint8_t indice) {
    Botao &b = botoes[indice];
    unsigned long agora = millis();
    bool leitura = (digitalRead(b.pino) == NIVEL_PRESSIONADO);

    b.evPressionou = false;
    b.evSoltou = false;

    if (leitura != b.ultimaLeitura) {  // houve variação: reinicia a janela de debounce
        b.tMudanca = agora;
        b.ultimaLeitura = leitura;
    }

    if ((agora - b.tMudanca) >= TEMPO_DEBOUNCE && leitura != b.estavel) {
        b.estavel = leitura;  // leitura estável: aceita a mudança

        if (b.estavel) {
            b.tPressionado = agora;
            b.evPressionou = true;
        } else {
            b.duracao = agora - b.tPressionado;  // Δt = t_solto - t_pressionado
            b.evSoltou = true;
        }
    }
}

// ======================= Display =======================
void desenharTitulo() {
    lcd.setCursor(0, 0);

    if (modo == MODO_AJUSTE) {
        lcd.print(F("AJUSTE DE HORA  "));
    } else {
        lcd.print(F("RELOGIO DIGITAL "));
    }
}

void desenharHora() {
    char buf[12];  // "HH:MM:SS" + folga
    snprintf(buf, sizeof(buf), "%02u:%02u:%02u", horas, minutos, segundos);

    // Na fase "apagada" do pisca, os dois dígitos do campo selecionado viram espaços
    if (modo == MODO_AJUSTE && !piscaVisivel) {
        uint8_t pos;
        if (campo == CAMPO_HORAS) {
            pos = 0;
        } else if (campo == CAMPO_MINUTOS) {
            pos = 3;
        } else {
            pos = 6;
        }

        buf[pos] = ' ';
        buf[pos + 1] = ' ';
    }

    lcd.setCursor(COLUNA_HORA, 1);
    lcd.print(buf);
}

void atualizarDisplay() {
    if (redesenharTitulo) {
        desenharTitulo();
        redesenharTitulo = false;
    }

    if (redesenharHora) {
        desenharHora();
        redesenharHora = false;
    }
}

// Mantém os dígitos visíveis (usado ao mudar de campo ou incrementar)
void reiniciarPisca() {
    piscaVisivel = true;
    tPisca = millis();
    redesenharHora = true;
}

// ======================= Lógica do relógio =======================
void avancarUmSegundo() {
    segundos++;
    if (segundos >= 60) {
        segundos = 0;
        minutos++;

        if (minutos >= 60) {
            minutos = 0;
            horas++;

            if (horas >= 24) {
                horas = 0;
            }
        }
    }
}

void atualizarRelogio() {
    unsigned long agora = millis();

    while ((agora - tAnterior) >= INTERVALO_SEGUNDO) {
        tAnterior += INTERVALO_SEGUNDO;
        avancarUmSegundo();
        redesenharHora = true;
    }
}

void alternarModo() {
    if (modo == MODO_RELOGIO) {
        modo = MODO_AJUSTE;      // entra no ajuste: contagem pausada
        campo = CAMPO_SEGUNDOS;  // campo inicial = segundos
        reiniciarPisca();
    } else {
        modo = MODO_RELOGIO;     // sai do ajuste: retoma a contagem
        tAnterior = millis();    // novo segundo começa agora, a partir da hora ajustada
        piscaVisivel = true;
        redesenharHora = true;
    }

    redesenharTitulo = true;
}

void proximoCampo() {
    campo = (campo + 1) % 3;  // Segundos -> Minutos -> Horas -> Segundos
    reiniciarPisca();
}

void incrementarCampo() {
    if (campo == CAMPO_HORAS) {
        horas = (horas + 1) % 24;
    } else if (campo == CAMPO_MINUTOS) {
        minutos = (minutos + 1) % 60;
    } else {
        segundos = (segundos + 1) % 60;
    }

    reiniciarPisca();
}

void atualizarPisca() {
    unsigned long agora = millis();

    if ((agora - tPisca) >= INTERVALO_PISCA) {
        tPisca = agora;
        piscaVisivel = !piscaVisivel;
        redesenharHora = true;
    }
}

// ======================= Arduino =======================
void setup() {
    pinMode(PINO_AJUSTE, INPUT);
    pinMode(PINO_AVANCO, INPUT);

    // Estado inicial dos botões (evita evento falso na partida)
    for (uint8_t i = 0; i < 2; i++) {
        bool pressionado = (digitalRead(botoes[i].pino) == NIVEL_PRESSIONADO);
        botoes[i].ultimaLeitura = pressionado;
        botoes[i].estavel = pressionado;
        botoes[i].tPressionado = millis();
    }

    lcd.init();
    lcd.clear();
    lcd.backlight();  // garante que a luz de fundo esteja ligada

    tAnterior = millis();
}

void loop() {
    atualizarBotao(BTN_AJUSTE);
    atualizarBotao(BTN_AVANCO);

    // Botão de ajuste: decide pela duração, ao soltar
    if (botoes[BTN_AJUSTE].evSoltou) {
        if (botoes[BTN_AJUSTE].duracao >= TEMPO_PRESSAO_LONGA) {
            alternarModo();  // entra/sai do modo de ajuste
        } else if (modo == MODO_AJUSTE) {
            proximoCampo();  // clique rápido: troca de campo
        }
    }

    // Botão de avanço: ignorado fora do modo de ajuste
    if (botoes[BTN_AVANCO].evPressionou && modo == MODO_AJUSTE) {
        incrementarCampo();
    }

    if (modo == MODO_RELOGIO) {
        atualizarRelogio();  // contagem só roda fora do ajuste
    } else {
        atualizarPisca();
    }

    atualizarDisplay();
}
