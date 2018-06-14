#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

unsigned char segs_out = 0;
unsigned char state_counter = 8;
unsigned char output_change = 0;
unsigned char LCD_d_1 = 0; // counter starts at "000"
unsigned char LCD_d_2 = 0;
unsigned char LCD_d_3 = 0;
unsigned char LCD_d_4 = 0;
unsigned char Pt_1_sec = 0; // 0.1 second counter



// Prototypes defined below
void initialization(void) {

    // initialize ADC
    ADMUX = 0b11000100;	// Vref=1.1V, right-aligned, ADC4 input
    ADCSRB = 0x06; // auto trigger by timer1 overflow
    ADCSRA = 0b10111000;	// adc, auto-trigger by Timer0 ovf, interrupt enabled, prescale = 1/2

    // initialize timer1
    TCCR1A = 0b00000000;	// no oc-out, normal mode
    TCCR1B = 3;	// 1/64 prescale (overflow occurs every 0.52sec to start ADC)
    TIMSK1 = 0x00;	// no interrupts
    TCNT1 = 50;

	CLKPR=0x80;
	CLKPR=0x00;

	TCCR0A=0x00;
	TCCR0B = (0<<CS12)|(1<<CS11)|(1<<CS10);  //TCCR0B=0x03; // = 8MHz/64 3/22/05
	TCNT0=193;

	TIMSK0 = 0x01;

	EICRA=0x00;
	EIMSK=0x00;
	PCICR=0x00;
}

// Look Up Table (LUT) for 3 1/2 digit/4 COM LCD by Pacific  Displays, #PD-332
// The following table has 10 entries to display chars 0-9. Hex values are COM1-COM4 for LCD inputs A & B.
//display numbers 0-9
//0	  0x77	1	0x14	2	0x6d	3	0x5d	4	0x1e	5	0x5b	6	0x7b	7	0x15	8	0x7f	9	0x5F
//01110111	00010100	01101101	01011101	00011110	01011011	01111011	00010101	01111111	01011111

const unsigned char segment_table[] = {0x77,0x14,0x6d,0x5d,0x1e,0x5b,0x7b,0x15,0x7f,0x5F};

// A/D conversion result
volatile uint16_t analog_in;

// ADC interrupt: take in analog input value
ISR(ADC_vect) {
	analog_in = ADC;
	TIFR1 = 0x01;	// clear timer1 ovf to restart trigger
}

ISR(TIMER0_OVF_vect)
{
	// Re-load Timer 0 value
	TCNT0=5; //Timer0 period = 0.125 usec = 8MHZ/64. 2msec = 8usec*250 5 = 255-250 5/4/05
	state_counter++;
	output_change = 1; // This is a flag for main loop
	if (state_counter > 7) state_counter = 0;
	}

void lcdOut(){
	// The following state_counter generates the 4 COM output waveforms via PORTD, each with HI and LOW outputs
	switch (state_counter) {
		case 0: {
			segs_out = (segment_table[LCD_d_1]& 0x03); 					//getdigit_1's A & B bits
			segs_out = segs_out | ((segment_table[LCD_d_2]&0x03)*4); 	//get digit_10's A & B bits
			segs_out = segs_out | ((segment_table[LCD_d_3]&0x03)*16); 	//get digit_100's A & B bits
			segs_out = segs_out | ((segment_table[LCD_d_4]&0x03)*64); 	//get digit_1000's A & B bits
			DDRC = 0;
			PORTC = 0x00;
			PORTD = segs_out;
			DDRD = 0xFF; // always on
			DDRC = 0x01; //COM1 asserted LOW
		}
		break;
		case 1: {
			PORTC = 0x01;
			PORTD = segs_out ^ 0xFF; // Compliment segment outputs
			DDRD = 0xFF; // always on
			DDRC = 0x01; //COM1 asserted High
		}
		break;
		case 2: {
			segs_out = (segment_table[LCD_d_1]& 0x0C)/4; 				//getdigit_1's A & B bits
			segs_out = segs_out | (segment_table[LCD_d_2]&0x0C);		//get digit_10's A & B bits
			segs_out = segs_out | ((segment_table[LCD_d_3]&0x0C)*4); 	//get digit_100's A & B bits
			segs_out = segs_out | ((segment_table[LCD_d_4]&0x0C)*16); 	//get digit_1000's A & B bits
			DDRC = 0;
			PORTC = 0x00;
			PORTD = segs_out;
			DDRD = 0xFF; // always on
			DDRC = 0x02; //COM2 asserted LOW
		}
		break;
		case 3: {
			PORTC = 0x02;
			PORTD = segs_out ^ 0xFF; // Compliment segment  outputs
			DDRD =0xFF;
			DDRC = 0x02; //COM2 asserted High
		}
		break;
		case 4: {
			segs_out = (segment_table[LCD_d_1]& 0x30)/16;				//get digit_1's A & B bits
			segs_out = segs_out | ((segment_table[LCD_d_2]& 0x30)/4);	//get digit_10's A & B bits
			segs_out = segs_out | (segment_table[LCD_d_3]&0x30);		//get digit_100's A & B bits
			segs_out = segs_out | ((segment_table[LCD_d_4]&0x30)*4);	//get digit_1000's A & B bits
			DDRC = 0;
			PORTC = 0x00;
			PORTD = segs_out;
			DDRD = 0xFF;
			DDRC = 0x04; //COM3 asserted LOW
		}
		break;
		case 5: {
			PORTC = 0x04;
			PORTD = segs_out ^ 0xFF; // Compliment segment outputs
			DDRD = 0xFF;
			DDRC = 0x04; //COM3 asserted High
		}
		break;
		case 6: {
			segs_out = (segment_table[LCD_d_1]& 0xC0)/64;				//get digit_1's A & B bits
			segs_out = segs_out | ((segment_table[LCD_d_2]&0xC0)/16); 	//get digit_10's A & B bits
			segs_out = segs_out | ((segment_table[LCD_d_3]&0xC0)/4); 	//get digit_100's A & B bits
			segs_out = segs_out | (segment_table[LCD_d_4]&0xC0); 		//get digit_1000's A & B bits
			DDRC = 0;
			PORTC = 0x00;
			// PORTC = 0x00;//LCD_d_3 ^ 0xFF; // Compliment  segment outputs
			PORTD = segs_out;
			DDRD = 0xFF;
			DDRC = 0x08; //COM4 asserted LOW
		}
		break;
		case 7: {
			PORTC = 0x08;
			PORTD = 0x55;    // PORTC = 0xFF; //LCD_d_3;
			PORTD = segs_out ^ 0xFF; // Compliment segment outputs
			DDRD = 0xFF;
			DDRC = 0x08; //COM4 asserted High
		}
		break;
		default: DDRD = 0;
		DDRC = 0; // COM1-COM4 float
		}
	}

void timer(){
	// Increment a counter to measure out 0.1 sec
	Pt_1_sec++;
	if (Pt_1_sec >=50){//.1 sec
		Pt_1_sec = 0;
		LCD_d_1++; // 3 1/2 digit ripple BCD counter for LCD digits
			if (LCD_d_1 >=10){
				LCD_d_1 = 0;
				LCD_d_2++;}
					if (LCD_d_2 >=10){
						LCD_d_2 = 0;
						LCD_d_3++;}
							if (LCD_d_3 >=10){
								LCD_d_3 = 0;
								LCD_d_4++;
							}
					}// end .1 sec
}

void adcToLCD(){
	uint16_t i;
	i = (analog_in * 1100l) / 1024;
	LCD_d_1 = i%10;
	i /= 10;
	LCD_d_2 = i%10;
	i /= 10;
	LCD_d_3 = i%10;
	i /= 10;
	LCD_d_4 = i%10;
}

int main(void) {

	initialization();
	sei();

	while (1)
	{
		_delay_us(500);
		if(output_change){
				output_change = 0;
				lcdOut();
				//timer();
				adcToLCD();

			}

	}
}
