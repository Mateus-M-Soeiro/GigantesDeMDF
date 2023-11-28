#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 16000000UL  ///<Define frequência de operação

//Definição das macros
#define _BV(bit) (1 << (bit))                     ///<Macro bit value
#define SET_BIT(PORT, BIT) PORT |= (1 << BIT)     ///< Define como 1 o bit na respectiva porta
#define CLEAR_BIT(PORT, BIT) PORT &= ~(1 << BIT)  ///< limpa o bit na respectiva porta
#define FLIP_BIT(PORT, BIT) PORT ^= (1 << BIT)    ///< Alterna o valor do bit na respectiva porta

// Definição dos pinos
#define MOTOR_D_FRENTE PB1
#define MOTOR_D_TRAS PB2
#define MOTOR_E_FRENTE PB3
#define MOTOR_E_TRAS PB4
#define LASER PD0
#define LED_FUNC PB5
#define LED_VIDA_1 PB0
#define LED_VIDA_2 PD3
#define LED_VIDA_3 PD4
#define BOTAO PD5
#define LDR PC2

// Variáveis globais
volatile uint8_t vidas = 3;
volatile uint8_t danoObtido = 0;
volatile uint8_t laserLigado = 0;
volatile uint8_t flag = 1;

// Canais ADC para LDR e joysticks
uint8_t adcChannelLDR = 2;  // A2
uint8_t adcChannelX = 1;    // A1
uint8_t adcChannelY = 0;    // A0

uint16_t adcValor;



void Func_atraso(unsigned int ms) {
  for (unsigned int i = 0; i < ms; i++) {
    // Ajuste a contagem para um atraso de aproximadamente 1 milissegundo
    for (unsigned int j = 0; j < F_CPU / 4000; j++) {
      asm volatile("nop");  // Operação nula para atraso
    }
  }
}



// Função de inicialização
void init() {
  UCSR0B |= (0 << RXEN0) | (0 << TXEN0);  // Desabilitando funções RX e TX

  // Configuração dos pinos de saída
  DDRB |= (1 << MOTOR_D_FRENTE) | (1 << MOTOR_D_TRAS) | (1 << MOTOR_E_FRENTE) | (1 << MOTOR_E_TRAS) | (1 << LED_FUNC) | (1 << LED_VIDA_1);
  DDRD |= (1 << LASER) | (1 << LED_VIDA_2) | (1 << LED_VIDA_3);

  // Configuração do pino de entrada
  DDRD &= ~(1 << BOTAO);  // Pino do botão como entrada

  // Configuração do ADC para leitura do LDR
  ADMUX = (1 << REFS0);                                                             // Referência de tensão AVcc
  ADCSRA = (1 << ADEN) | (1 << ADIE) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);  // Habilita ADC e define o prescaler
}

// inicializando as variáveis e LED acesos
void inicioVidas() {

  vidas = 3;
  danoObtido = 0;

  // LEDs acesos
  SET_BIT(PORTB, LED_VIDA_1);
  SET_BIT(PORTD, LED_VIDA_2);
  SET_BIT(PORTD, LED_VIDA_3);
}



// desliga o funcionamento do robo
void desligaRobo() {

  cli();                       //desativa as interrupções
  CLEAR_BIT(PORTD, LASER);     // desativa o laser
  CLEAR_BIT(PORTB, LED_FUNC);  // desliga o led de funcionamento


  CLEAR_BIT(PORTB, MOTOR_D_TRAS);    // Desativa motor esquerdo
  CLEAR_BIT(PORTB, MOTOR_E_TRAS);    // Desativa motor esquerdo
  CLEAR_BIT(PORTB, MOTOR_E_FRENTE);  // Desativa motor esquerdo
  CLEAR_BIT(PORTB, MOTOR_D_FRENTE);  // Desativa motor esquerdo

  vidas = 3;
  danoObtido = 0;
  flag = 0;
  sei();
}

void giro() {
  CLEAR_BIT(PORTB, MOTOR_D_TRAS);    // Desativa motor esquerdo
  CLEAR_BIT(PORTB, MOTOR_E_TRAS);    // Desativa motor esquerdo
  CLEAR_BIT(PORTB, MOTOR_E_FRENTE);  // Desativa motor esquerdo
  SET_BIT(PORTB, MOTOR_D_FRENTE);    // Ativa motor direito


  cli();
  Func_atraso(1000);

  CLEAR_BIT(PORTB, MOTOR_D_FRENTE);  // Desativa motor esquerdo

  Func_atraso(5000);

  sei();
}

// Função para controle dos LEDs de vida
void dano() {
  danoObtido++;  //incrementa 1 no danoObtido

  int vidasAtuais = vidas - danoObtido;  //calcula a vida atual após o danoObtido

  switch (vidasAtuais) {
    case 2:
      CLEAR_BIT(PORTD, LED_VIDA_3);
      Func_atraso(1000);
      giro();
      break;
    case 1:
      CLEAR_BIT(PORTD, LED_VIDA_2);
      Func_atraso(1000);
      giro();
      break;
    case 0:
      CLEAR_BIT(PORTB, LED_VIDA_1);
      Func_atraso(1000);
      desligaRobo();
      break;
    default:
      // nada
      break;
  }
}

// config para adc
void adc_init() {

  // Configuração do registrador ADMUX para referência de tensão AVcc e canal ADC0
  ADMUX |= (1 << REFS0);
  // Habilita o ADC e define o prescaler para 128
  ADCSRA |= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

// Função para realizar uma leitura ADC sincrona no canal especificado
uint16_t readADC(uint8_t channel) {
  // Limpa os bits de seleção de canal no registrador ADMUX
  ADMUX &= ~((1 << MUX3) | (1 << MUX2) | (1 << MUX1) | (1 << MUX0));
  // Define o canal desejado
  ADMUX |= channel;
  // Inicia a conversão
  ADCSRA |= (1 << ADSC);
  // Aguarda a conversão ser concluída
  while (ADCSRA & (1 << ADSC))
    ;
  // Retorna o resultado da conversão
  return ADC;
}

// Inicialização do Timer0 para PWM
void initTimer0() {
  TCCR0A |= (1 << WGM00) | (1 << WGM01) | (1 << COM0A1) | (1 << COM0B1);  // Modo Fast PWM, non-inverting
  TCCR0B |= (1 << CS02);                                                  // Prescaler 256
  OCR0A = 64;                                                              // Valor inicial de PWM para o motor direito
  OCR0B = 64;                                                              // Valor inicial de PWM para o motor esquerdo
}

// Inicialização do Timer1 para laser
void initTimer1() {
  TCCR1B |= (1 << WGM12) | (1 << CS12);  // Modo CTC, prescaler 256
  OCR1A = 62500;                         // Valor para gerar interrupção a cada 1 segundo (considerando um clock de 16MHz)
  TIMSK1 |= (1 << OCIE1A);               // Habilita interrupção por comparação
}

// Inicialização do funcionamento do botão
void trigger_init() {

  cli();
  EIMSK |= (1 << INT0);
  EICRA |= (1 << ISC00) | (1 << ISC01);
  sei();
}


// Função para tratar o botão de inicialização
ISR(INT0_vect) {



  // Inicializa o funcionamento
  SET_BIT(PORTB, LED_FUNC);  // Liga o LED de funcionamento

  initTimer1();   //ativa o laser
  inicioVidas();  // Adiciona a inicialização dos LEDs de vida
  initTimer0();   // PWM
  flag = 1;
}

// Função para tratar a interrupção do ADC
ISR(ADC_vect) {
  // Leitura do valor do ADC
  adcValor = ADC;
}

// Função para tratar a interrupção do Timer1 (piscar o laser)
ISR(TIMER1_COMPA_vect) {


  // Inverte o estado do laser
  muda_laser();
}

void muda_laser() {

  if (flag == 1) {

    FLIP_BIT(PORTD, LASER);

  } else {

    CLEAR_BIT(PORTD, LASER);
  }
}


int main(void) {

  UCSR0B |= (0 << RXEN0) | (0 << TXEN0);  // Desabilitando funções RX e TX
  // Inicialização
  init();

  // Habilita as interrupções globais
  sei();

  // Configuração da interrupção do botão (INT0)
  trigger_init();

  // Configuração do adc
  adc_init();

  Serial.begin(9600);  // Inicialização da comunicação serial


  while (1) {
    // Realiza leituras ADC
    uint16_t adcValueLDR = readADC(adcChannelLDR);  //valor ldr
    uint16_t adcValueX = readADC(adcChannelX);      //valor do eixo X do joystck
    uint16_t adcValueY = readADC(adcChannelY);      //valor do eixo y do joytick

    Serial.println(adcValueX);
    //Serial.println(adcValueY);
    //Serial.println(adcValueLDR);

    if (flag == 1) {

      if (adcValueLDR < 250) {

        dano();
      }
      if (adcValueY > 800) {

        CLEAR_BIT(PORTB, MOTOR_E_TRAS);  // Desativa motor direito
        CLEAR_BIT(PORTB, MOTOR_D_TRAS);  // Desativa motor direito
        SET_BIT(PORTB, MOTOR_E_FRENTE);  // Ativa motor esquerdo
        SET_BIT(PORTB, MOTOR_D_FRENTE);  // Ativa motor esquerdo


      } else if (adcValueY < 100) {


        SET_BIT(PORTB, MOTOR_E_TRAS);      // Ativa motor esquerdo
        SET_BIT(PORTB, MOTOR_D_TRAS);      // Ativa motor esquerdo
        CLEAR_BIT(PORTB, MOTOR_D_FRENTE);  // Desativa motor direito
        CLEAR_BIT(PORTB, MOTOR_E_FRENTE);  // Desativa motor direito


      } else if (adcValueX > 500 && adcValueX < 600) {


        CLEAR_BIT(PORTB, MOTOR_E_FRENTE);  // Desativa motor esquerdo
        CLEAR_BIT(PORTB, MOTOR_D_FRENTE);  // Desativa motor esquerdo
        CLEAR_BIT(PORTB, MOTOR_E_TRAS);    // Desativa motor esquerdo
        CLEAR_BIT(PORTB, MOTOR_D_TRAS);    // Desativa motor esquerdo
      }

      // Verifica o valor de adcValueX para decidir a direção do movimento
      if (adcValueX < 100) {
        // Vira para a esquerda
        SET_BIT(PORTB, MOTOR_E_FRENTE);    // Ativa motor esquerdo
        CLEAR_BIT(PORTB, MOTOR_D_FRENTE);  // Desativa motor direito

      } else if (adcValueX > 900) {
        // Vira para a direita
        CLEAR_BIT(PORTB, MOTOR_E_FRENTE);  // Desativa motor esquerdo
        SET_BIT(PORTB, MOTOR_D_FRENTE);    // Ativa motor direito
      } else if (adcValueX > 500 && adcValueX < 600) {


        CLEAR_BIT(PORTB, MOTOR_E_FRENTE);  // Desativa motor esquerdo
        CLEAR_BIT(PORTB, MOTOR_D_FRENTE);  // Desativa motor esquerdo
      }
    } else {


      CLEAR_BIT(PORTB, MOTOR_D_TRAS);    // Desativa motor esquerdo
      CLEAR_BIT(PORTB, MOTOR_E_TRAS);    // Desativa motor esquerdo
      CLEAR_BIT(PORTB, MOTOR_E_FRENTE);  // Desativa motor esquerdo
      CLEAR_BIT(PORTB, MOTOR_D_FRENTE);  // Desativa motor esquerdo
    }
  }

  return 0;
}
