/*

	19-11-13
	Christopher Cartwright

	2-24-11
	by Jim Lindblom
	
    3-4-09
    Copyright Spark Fun Electronics© 2009
    Nathan Seidle

*/

//#define TEST_SEGMENT

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>

#define SBI(port, pin)	((port) |= (uint8_t)(1 << pin))
#define CBI(port, pin)	((port) &= (uint8_t)~(1 << pin))
#define MODE_SET		(PINB & (1<<BUT_ALARM))

#define FOSC 16000000 //16MHz internal osc
//#define FOSC 1000000 //1MHz internal osc
//#define BAUD 9600
//#define MYUBRR (((((FOSC * 10) / (16L * BAUD)) + 5) / 10) - 1)

#define STATUS_LED	5 //PORTB

#define BRIGHT_LEVEL 50

#define SEG_A	PORTC3
#define SEG_B	PORTC5
#define SEG_C	PORTC2
#define SEG_D	PORTD2
#define SEG_E	PORTC0
#define SEG_F	PORTC1

#define DIG_1	PORTD0
#define DIG_2	PORTD1
#define DIG_3	PORTD4
#define DIG_4	PORTD6

#define DP		PORTD5
#define COL		PORTD3
#define AMPM		PORTB3

#define BUT_UP		PORTB5
#define BUT_DOWN	PORTB4
#define BUT_SNOOZE	PORTD7
#define BUT_ALARM	PORTB0

#define BUZZ1	PORTB1
#define BUZZ2	PORTB2

enum maxunit_t
{
	HOURS = 0,
	MINUTES = 1
};

typedef struct {
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
} time_t;

//Declare functions
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void ioinit (void);
void delay_ms(uint16_t x); // general purpose delay
void delay_us(uint16_t x);

void siren(int duration);
void half_siren(int duration);
void display_number(uint8_t number, uint8_t digit);
void display_time(uint16_t time_on, time_t time);
void clear_display(void);
void check_buttons(void);
void decrement(time_t* time, bool minutes);
void increment(time_t* time, bool minutes);
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Declare global variables
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
bool flip;
time_t current, alarm;
enum maxunit_t maxunit;

bool cycle;
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

ISR (TIMER1_OVF_vect)
{
	//Prescalar of 1024
	//Clock = 16MHz
	//15,625 clicks per second
	//64us per click	
    
	TCNT1 = 49911; //65536 - 15,625 = 49,911 - Preload timer 1 for 49,911 clicks. Should be 1s per ISR call

	//Debug with faster time!
	//TCNT1 = 63581; //65536 - 1,953 = 63581 - Preload timer 1 for 63581 clicks. Should be 0.125s per ISR call - 8 times faster than normal time
	
	flip = !flip;
	
	if(cycle)
	{
		decrement(&current, false);
		if(current.hours == 0 && current.minutes == 0 && current.seconds == 0)
		{
			cycle = false;
			current = alarm;
			siren(500);
			siren(500);
		}
	}
}

ISR (TIMER2_OVF_vect)
{
	if(MODE_SET == 1)
	{
		display_time(10, alarm);
	}
	else
	{
		display_time(10, current);
	}
}


int main (void)
{
	ioinit();
	
	while(true)
	{
		check_buttons();
	}
	
	return(0);
}

//Checks buttons for system settings
void check_buttons(void)
{
	if(MODE_SET == 1)
	{
		// Setting timer
		if((PIND & (1 << BUT_SNOOZE)) == 0)
		{
			maxunit = !maxunit;
			delay_ms(300);
		}
		
		if((PINB & (1 << BUT_UP)) == 0)
		{
			increment(&alarm, maxunit == HOURS);
			delay_ms(100);
		}
		
		if((PINB & (1 << BUT_DOWN)) == 0)
		{
			decrement(&alarm, maxunit == HOURS);
			delay_ms(100);
		}
	}
	else
	{
		// Running timer
		if((PIND & (1 << BUT_SNOOZE)) == 0)
		{
			cycle = !cycle;
			if(cycle)
			{
				half_siren(50);
			}

			delay_ms(300);
		}
		
		if((PINB & (1 << BUT_UP)) == 0)
		{
			cycle = false;
			current = alarm;
			delay_ms(300);
		}
	}
	
	delay_ms(10);
}

void decrement(time_t* time, bool minutes)
{
	uint8_t amount = minutes ? 60 : 1;
	
	if(time->seconds < amount)
	{
		if(time->minutes == 0)
		{
			if(time->hours == 0)
			{
				return;
			}
			
			time->hours--;
			time->minutes += 60;
		}
		
		time->minutes--;
		time->seconds += 60;
	}

	time->seconds -= amount;
}

void increment(time_t* time, bool minutes)
{
	uint8_t amount = minutes ? 60 : 1;
	
	time->seconds += amount;
	if(time->seconds > 59)
	{
		time->seconds -= 60;
		
		time->minutes++;
		if(time->minutes > 59)
		{
			time->minutes -= 60;
			
			time->hours++;
			
			if(time->hours > 59)
			{
				time->hours = 59;
			}
		}
	}
}

void clear_display(void)
{
	CBI(PORTB, AMPM); // AMPM anode off=0
	PORTC = 0b00111111; // Set BGACFE cathode off=1
	PORTD &= 0b10100100; // Set DIG4,DIG3,COL,DIG2,DIG1 anodes off=0 PD752=NC=1
	PORTD |= 0b00100100; // Set DP,D cathode off=1 PD764310=NC=0
}

void display_number(uint8_t number, uint8_t digit)
{
	clear_display();
	
	switch(digit)
	{
		case 1:
			PORTD |= (1<<DIG_1);//Select Digit 1
			break;
		case 2:
			PORTD |= (1<<DIG_2);//Select Digit 2
			break;
		case 3:
			PORTD |= (1<<DIG_3);//Select Digit 3
			break;
		case 4:
			PORTD |= (1<<DIG_4);//Select Digit 4
			break;
		case 5:
			PORTD |= (1<<COL);//Select Digit COL
			break;
		case 6:
			PORTB |= (1<<3);	// Select Apostrophe
		default: 
			break;
	}
	
	switch(number)
	{
		case 0:
			PORTC &= 0b11010000; //Segments A, B, C, D, E, F
			PORTD &= 0b11111011;
			break;
		case 1:
			PORTC &= 0b11011011; //Segments B, C
			//PORTD |= 0b00000000;
			break;
		case 2:
			PORTC &= 0b11000110; //Segments A, B, D, E, G
			PORTD &= 0b11111011;
			break;
		case 3:
			PORTC &= 0b11000011; //Segments ABCDG
			PORTD &= 0b11111011;
			break;
		case 4:
			PORTC &= 0b11001001; //Segments BCGF
			PORTD &= 0b11111111;
			break;
		case 5:
			PORTC &= 0b11100001; //Segments AFGCD
			PORTD &= 0b11111011;
			break;
		case 6:
			PORTC &= 0b11100000; //Segments AFGDCE
			PORTD &= 0b11111011;
			break;
		case 7:
			PORTC &= 0b11010011; //Segments ABC
			PORTD &= 0b11111111;
			break;
		case 8:
			PORTC &= 0b11000000; //Segments ABCDEFG
			PORTD &= 0b11111011;
			break;
		case 9:
			PORTC &= 0b11000001; //Segments ABCDFG
			PORTD &= 0b11111011;
			break;

		case 10:
			//Colon
			PORTC &= 0b11111011;
			PORTD &= 0b11111111;
			break;

		case 11:
			//Alarm dot
			PORTD &= 0b11011111;
			break;

		case 12:
			//AM dot
			PORTC &= 0b11111101; //Segments C
			break;

		default: 
			PORTC = 0;
			break;
	}
	
}

//Displays current time
//Brightness level is an amount of time the LEDs will be in - 200us is pretty dim but visible.
//Amount of time during display is around : [ BRIGHT_LEVEL(us) * 5 + 10ms ] * 10
//Roughly 11ms * 10 = 110ms
//Time on is in (ms)
void display_time(uint16_t time_on, time_t time)
{
	//time_on /= 11; //Take the time_on and adjust it for the time it takes to run the display loop below
	
	for(uint16_t j = 0 ; j < time_on ; j++)
	{
		if(maxunit == HOURS)
		{
			//Display normal hh:mm time
			if(time.hours > 9)
			{
				display_number(time.hours / 10, 1); //Post to digit 1
				delay_us(BRIGHT_LEVEL);
			}

			display_number(time.hours % 10, 2); //Post to digit 2
			delay_us(BRIGHT_LEVEL);

			display_number(time.minutes / 10, 3); //Post to digit 3
			delay_us(BRIGHT_LEVEL);

			display_number(time.minutes % 10, 4); //Post to digit 4
			delay_us(BRIGHT_LEVEL);
			
			display_number(11, 4); //Turn on dot on digit 4
			delay_us(BRIGHT_LEVEL);
		}
		else if(maxunit == MINUTES)
		{
			//During debug, display mm:ss
			display_number(time.minutes / 10, 1); 
			delay_us(BRIGHT_LEVEL);

			display_number(time.minutes % 10, 2); 
			delay_us(BRIGHT_LEVEL);

			display_number(time.seconds / 10, 3); 
			delay_us(BRIGHT_LEVEL);

			display_number(time.seconds % 10, 4); 
			delay_us(BRIGHT_LEVEL);
		}
		
		//Flash colon for each second
		if(flip == 1) 
		{
			display_number(10, 5); //Post to digit COL
			delay_us(BRIGHT_LEVEL);
		}

		if(cycle)
		{
			display_number(12, 6);
			delay_us(BRIGHT_LEVEL);
		}
		
		if(maxunit == HOURS)
		{
			display_number(11, 6);
			delay_us(BRIGHT_LEVEL);
		}

		clear_display();
		delay_us(BRIGHT_LEVEL);
	}
}

void half_siren(int duration)
{
	for(int i = 0 ; i < duration ; i++)
	{
		CBI(PORTB, BUZZ1);
		SBI(PORTB, BUZZ2);
		delay_us(300);

		SBI(PORTB, BUZZ1);
		CBI(PORTB, BUZZ2);
		delay_us(300);
	}

	CBI(PORTB, BUZZ1);
	CBI(PORTB, BUZZ2);
}

//Make noise for time_on in (ms)
void siren(int duration)
{
	half_siren(duration);

	delay_ms(50);

	half_siren(duration);
}

void ioinit(void)
{
	//1 = output, 0 = input 
	DDRB = 0b11111111 & ~((1<<BUT_UP)|(1<<BUT_DOWN)|(1<<BUT_ALARM)); //Up, Down, Alarm switch  
	DDRC = 0b11111111;
	DDRD = 0b11111111 & ~(1<<BUT_SNOOZE); //Snooze button
	

	PORTB = (1<<BUT_UP)|(1<<BUT_DOWN)|(1<<BUT_ALARM); //Enable pull-ups on these pins
	PORTD = 0b10100100; //Enable pull-up on snooze button
	PORTC = 0b00111111;

	//Init Timer0 for delay_us
	TCCR0B = (1<<CS01); //Set Prescaler to clk/8 : 1click = 0.5us(assume we are running at external 16MHz). CS01=1 
	
	//Init Timer1 for second counting
	TCCR1B = (1<<CS12)|(1<<CS10); //Set prescaler to clk/1024 :1click = 64us (assume we are running at 16MHz)
	TIMSK1 = (1<<TOIE1); //Enable overflow interrupts
	TCNT1 = 49911; //65536 - 15,625 = 49,911 - Preload timer 1 for 49,911 clicks. Should be 1s per ISR call
	
	//Init Timer2 for updating the display via interrupts
	TCCR2B = (1<<CS22)|(1<<CS21)|(1<<CS20); //Set prescalar to clk/1024 : 1 click = 64us (assume 16MHz)
	TIMSK2 = (1<<TOIE2);
	//TCNT2 should overflow every 16.384 ms (256 * 64us)
	
	sei(); //Enable interrupts

	siren(200); //Make some noise at power up
	
	current = (time_t) { .hours = 0, .minutes = 1, .seconds = 0 };
	alarm = (time_t) { .hours = 0, .minutes = 1, .seconds = 0 };

	cycle = false;
	
	maxunit = MINUTES;

#ifdef TEST_SEGMENT
	while(1)
	{
		PORTD = 0;
		PORTC = 0xFF;
		delay_ms(1000);

		PORTD = 0xFF;
		PORTC = 0xFF;
		delay_ms(1000);
	}
#endif
}

//General short delays
void delay_ms(uint16_t x)
{
	for (; x > 0 ; x--)
	{
		delay_us(250);
		delay_us(250);
		delay_us(250);
		delay_us(250);
	}
}

//General short delays
void delay_us(uint16_t x)
{
	x *= 2; //Correction for 16MHz
	
	while(x > 256)
	{
		TIFR0 = (1<<TOV0); //Clear any interrupt flags on Timer2
		TCNT0 = 0; //256 - 125 = 131 : Preload timer 2 for x clicks. Should be 1us per click
		while( (TIFR0 & (1<<TOV0)) == 0);
		
		x -= 256;
	}

	TIFR0 = (1<<TOV0); //Clear any interrupt flags on Timer2
	TCNT0 = 256 - x; //256 - 125 = 131 : Preload timer 2 for x clicks. Should be 1us per click
	while( (TIFR0 & (1<<TOV0)) == 0);
}
