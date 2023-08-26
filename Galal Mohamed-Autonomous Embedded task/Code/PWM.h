/*
 * motor_dc.h
 *
 * Created: 8/25/2023
 * Author : Galal
 */ 

#include <avr/io.h>
#include <util/delay.h>

void pwm_init()
{
	pwm_lcd_init();
	pwm_dc_motor_init();
	pwm_stepper_motor_init();
};

void pwm_lcd_init()
{
	TCCR2 |= (1<<WGM21) | (1<<CS22) | (1<<CS21) | (1<<CS20);	//CTC, /1024 prescaling
	OCR2 = 78;	//set compare value, 0.01 seconds
	TIMSK |= (1<<OCIE2); //Enable interrupt on compare match
}

void pwm_dc_motor_init()
{
	TCCR0 |= (1<<WGM00) | (1<<WGM01) | (1<<COM01) | (1<<CS00);	//Fast PWM, 8-bit, non-inverted, no prescaling
	DDRB|=(1<<PB3);	//Set pin OC0 as output
}

void pwm_stepper_motor_init()
{
	TCCR1A |= (1<<COM1A1) | (1<<WGM11);	//Fast PWM, non-inverted
	TCCR1B |= (1<<COM1B1) | (1<<WGM13) | (1<<WGM12) | (1<<CS11) | (1<<CS10);	//Fast PWM, non-inverted, /64 prescaling
	ICR1 = 2499; //set compare value, given: fPWM=50Hz (period=20ms)
	DDRD |= (1<<PD4) | (1<<PD5);	//Set pins OC1A and OC1B as output
	OCR1A = 187;	//start at 0deg angle
	int delay = 120;
	//for (int i = 0; i < 11 ; i++)
	//{
		//PORTC = (1<<PC0);
		//_delay_ms(delay);
		//PORTC = (1<<PC1);
		//_delay_ms(delay);
		//PORTC = (1<<PC2);
		//_delay_ms(delay);
		//PORTC = (1<<PC3);
		//_delay_ms(delay);
		//// 11 loops -> -43.5 deg
	//}
	//PORTC = (1<<PC0);
	//_delay_ms(300);
	//PORTC = 0x00;
}