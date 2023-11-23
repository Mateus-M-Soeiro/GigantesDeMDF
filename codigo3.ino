// Projeto Gigantes de MDF

#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>

#define _BV(bit) (1 << (bit))
#define SET_BIT(PORT,BIT) PORT |= (1<<BIT) 
#define CLEAR_BIT(PORT,BIT) PORT &= ~(1<<BIT)
#define FLIP_BIT(PORT,BIT) PORT ^= (1<<BIT)

volatile uint8_t tot_overflowTimer0; // Variável de controle do TIMER0
volatile uint8_t vidasTotais;
volatile uint8_t danoObtido;

int flag;

uint16_t adcValor;


volatile uint16_t adc_value_A0;


ISR(TIMER1_COMPA_vect) {
	FLIP_BIT(PORTD,0);
}

ISR(INT0_vect) { // função da interrupção do botão para ativar funcionamento

    	SET_BIT(PORTB,5); // Liga o led de funcionamento

    	inicioVidas();
	//flag = 1;

	timer1_init();//Ativa o laser
	//inicializa pwmMotorA
	//inicializa pwmMotorB
}

ISR(ADC_vect) {
    	// Leitura do valor do ADC
    	adcValor = ADC;

	flag = 1;
}

ISR(TIMER0_COMPA_vect) {
   
    	adc_value_A0 = ADC;

	Serial.println(adc_value_A0);

    // Reinicia a conversão ADC
     ADCSRA |= (1 << ADSC);
	Serial.println(adc_value_A0);
}




void ativaPWM1() {
    // Configuração do Timer0
    TCCR0A |= (1 << WGM01); // Modo CTC
    OCR0A = 124; // Valor para interrupção a cada 1 ms (considerando prescaler 64)
    TIMSK0 |= (1 << OCIE0A); // Habilita interrupção por comparação A
    TCCR0B |= (1 << CS01) | (1 << CS00); // Prescaler 64
}



void adc_init() {
    	// Configuração do registrador ADMUX
    	ADMUX = 0;
	//ADMUX |= _BV(REFS0); // Referência de tensão: AVcc
	ADMUX |= (0 << MUX0) | (0 << MUX1) | _BV(REFS0); // Seleciona o A0 (potenciometro)
	
	// Configuração do registrador ADCSRA
	ADCSRA = 0;
	ADCSRA |= _BV(ADEN) | _BV(ADPS1) | _BV(ADIE); // Habilita o ADC e configura o prescaler
	//ADCSRA &= ~_BV(ADPS2) & ~_BV(ADPS0); //garante que estes sejam definidos como 0
}


void timer1_init() {
	TCCR1B |= (1 << WGM12) | (1 << CS12); //coloco em modo comparação e config o prescaler
	TIMSK1 |= (1 << OCIE1A); //habilitando a interrupção quando compara o valor
	OCR1A = 62500; //prescaler de 256 para 1seg
}

void trigger_init() {
	
    	cli();
    	EIMSK |= (1 << INT0);
    	EICRA |= (1 << ISC00) | (1 << ISC01);
    	sei();
}

void inicioVidas() { // inicializando as variáveis e LED acesos
	
    	vidasTotais = 3;
    	danoObtido = 0;

    	// LEDs acesos
    	SET_BIT(PORTB, 0);
    	SET_BIT(PORTD, 4);
    	SET_BIT(PORTD, 3);
}

void leituraLDR() {
	
	
    	ADCSRA |= _BV(ADSC); //começa a conversão
	while((ADCSRA & _BV(ADIF))); //fica travado até a conversão for feita
	ADCSRA |= _BV(ADIF); //ativar interrupção
	//Serial.println(adcValor);
	if(adcValor <= 300){ //compara o valor do LDR
		dano(); //se a condição for verdadeira, chama a função
	}
}

void desligaRobo() {
	
	cli(); //desativa as interrupções
	CLEAR_BIT(PORTD,0); // desativa o laser
    	CLEAR_BIT(PORTB,5); // desliga o led de funcionamento
	flag = 0;
	
}
void giro(){
	//ativa in2
	//while(TCNT1 <= 1seg);
	//desativa in2   

    TCCR1A = 0;
    TCNT1 = 0;
    while(TCNT1 <= 15625);
    PORTD &= 0x0C;//cancela o giro
    while(TCNT1 <= 62500);
    timer1_init();    
	
    sei();//ativa as interrupções globais
}

void dano() {
	
		danoObtido++; //incrementa 1 no dano
		
		int vidasAtuais = vidasTotais - danoObtido; //calcula a vida atual após o dano
		
		switch (vidasAtuais) {
	        	case 2:
				
	            	CLEAR_BIT(PORTD, 3);
				giro();
	            	break;
	
	        	case 1:
	            	CLEAR_BIT(PORTD, 4);
				giro();
	            	break;
	
	        	case 0:
	            	CLEAR_BIT(PORTB, 0);
				desligaRobo();
	            	break;
	
	        	default:
	            	// nada
	            	break;
	    	}
}

int main(void) {
		
    	UCSR0B |= (0 << RXEN0) | (0 << TXEN0); // Desabilitando funções RX e TX

    	DDRD |= (1 << PD0); // Setando PD0 como saída ( Laser )
    	DDRD |= (1 << PD3) | (1 << PD4); // setando LED das vidas como saída
    	DDRB |= (1 << PB0); // setando LED da vida como saída
    	DDRB |= (1 << PB5); // setando PB5 como saída (LED funcionamento)
	
	
    	trigger_init();
    	adc_init(); //chama a função que configura e ativa a conversão
	ativaPWM1();
	//ativaPWM2();

	sei(); // habilita as interrupções globais
	
	flag = 1;
	
	Serial.begin(9600);
	
    	while(1) {
        	while(flag == 1) {
			//Serial.println("AAAAAAAAAAH");
		  	leituraLDR();
        	}
    	}
}