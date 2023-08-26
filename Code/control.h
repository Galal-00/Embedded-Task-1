/*
 * control.h
 *
 * Created: 8/24/2023
 * Author : Galal
 */ 

#include <stdio.h>
#include <stdlib.h>
#include "uart.h"
#include "lcd.h"
#include "PWM.h"

#define BUFFER_SIZE 8

char str[BUFFER_SIZE];	//message string
int ch_index = 0;	//current character in message index

/* time controls */
char hour [3] = {'0', '0', 0};
char min [3] = {'0', '0', 0};
char sec [3] = {'0', '0', 0};

/* old position data*/
int last_stepper_step = 0;	
char last_stepper_dir = 0;
int next_stepper_port = -1;

void message_handler(char ch)
{
	if(ch == '\r')
	{
		if(ch_index > BUFFER_SIZE)
		{
			UART_SendString("INVALID MESSAGE\r");
		}
		else if (isdigit(str[0]) && isdigit(str[1]) && isdigit(str[2]) && isdigit(str[4]) && isdigit(str[5]) && (str[3] == 'F' || str[3] == 'B') && (str[6] == 'R' || str[6] == 'L') && str[7] == 'E')
		{
			char ch_stepper[] = {str[4], str[5], 0};
			char ch_dc[] = {str[0], str[1], str[2], 0};
			int dc_val = atoi(ch_dc);
			int stepper_val = atoi(ch_stepper);
			if (dc_val <= 100 && stepper_val <= 45)
			{
				//TODO:: Handle correct messsage
				lcd_print_data(dc_val, stepper_val);
				set_dc(dc_val, str[3]);
				if((last_stepper_dir != str[6]) | (last_stepper_step != stepper_val) && stepper_val != 0)set_stepper(stepper_val, str[6], 0);
			}
			else
			{
				UART_SendString("INVALID MESSAGE\r");
			}
		}
		else
		{
			UART_SendString("INVALID MESSAGE\r");
		}
		clear_str();
	}
	else if (ch_index < BUFFER_SIZE)
	{
		str[ch_index] = ch;
		ch_index++;
	}
	else
	{
		ch_index++;
	}
	
}

void clear_str()
{
	for (int i = 0; i < BUFFER_SIZE; i++)
	{
		str[i] = 0;
	}
	ch_index = 0;
}

void lcd_print_data(int dc_val, int stepper_val)
{
	LCD_clear_line(1);
	LCD_cmd(0x80);	//force cursor to beginning of 1st line
	char str2[8];
	LCD_write_string("Speed: ");
	if(str[3] == 'B' && dc_val != 0) LCD_write('-');
	sprintf(str2, "%d", dc_val);
	LCD_write_string(str2);
	LCD_write_string("% ");
	LCD_write_string("Direction: ");
	if(str[6] == 'L' && stepper_val != 0) LCD_write('-');
	sprintf(str2, "%d", stepper_val);
	LCD_write_string(str2);
}

void lcd_print_time()
{
	sprintf(sec, "%d", atoi(sec) + 1);	//increment seconds
	// the following is to preserve the 00:00:00 format
	char temp;
	if(atoi(sec) < 10)
	{
		temp = sec[0];
		sec[0] = '0';
		sec[1] = temp;
	}
	if(atoi(sec) == 60)
	{
		//full minute passed
		sec[0] = '0';
		sec[1] = '0';
		sprintf(min, "%d", atoi(min) + 1);	//increment minutes
		if(atoi(min) < 10)
		{
			temp = min[0];
			min[0] = '0';
			min[1] = temp;
		}
		if (atoi(min) == 60)
		{
			//full hour passed
			min[0] = '0';
			min[1] = '0';
			sprintf(hour, "%d", atoi(hour) + 1); //increment hours
			if(atoi(hour) < 10)
			{
				temp = hour[0];
				hour[0] = '0';
				hour[1] = temp;
			}
		}
	}
	//print to LCD
	LCD_cmd(0xC6);	// go to second line, 6th character
	LCD_write_string(hour);
	LCD_write(':');
	LCD_write_string(min);
	LCD_write(':');
	LCD_write_string(sec);
}

void set_dc(int speed, char dir)
{
	if(dir == 'F')
	{
		PORTB &= ~(1<<PB1);
		PORTB |= (1<<PB2);
	}
	else if(dir == 'B')
	{
		PORTB &= ~(1<<PB2);
		PORTB |= (1<<PB1);
	}
	unsigned char duty_cycle = speed * 255 / 100;
	OCR0 = duty_cycle;
}


void set_stepper(int step, char dir, int cmd)
{
	/*	4 output ports each rotate motor by 1 deg
		11 loops are required to achieve +-43.5deg rotation
		(the 0.5deg is because the motor rotates in the first command by 0.5deg not 1deg)
		remainder loops are calculated to achieve +-45deg rotation (45 % 4 = 1 loop)
		the scale of rotation is linear so what can be applied 45deg can be applied to any angle
	*/
	int loop_count;
	int remainder;	//remaining steps to achieve desired step count
	int delay = 120;	//delay between stepper pulses
	if (dir == last_stepper_dir | last_stepper_dir == 0)
	{
		//formula when same dir
		loop_count = abs(step - last_stepper_step) / 4;
		remainder = abs(step - last_stepper_step) % 4;
	}
	else
	{
		//if dir changes
		loop_count = (step + last_stepper_step) / 4;
		remainder = (step + last_stepper_step) % 4;
	}
	// change direction if new step is smaller then old step and in same direction
	if(step - last_stepper_step < 0 && dir == last_stepper_dir && dir == 'L') dir = 'R';
	else if(step - last_stepper_step < 0 && dir == last_stepper_dir && dir == 'R') dir = 'L';
	
	/*	var: next_stepper_port:
		the variable is used to identify next position of the stepper port if the motor was to continue on the SAME
		direction
		if the direction changes the variable is corrected by incrementing or decrementing by 2 postions
		the var is useful to keep the smooth transition between the stepper coils and insure accurate steps
	*/
	
	if (dir == 'L')
	{
		if(last_stepper_dir != 'L' && next_stepper_port != -1)
		{
			next_stepper_port = (next_stepper_port - 2) % 4;
			if(next_stepper_port < 0) next_stepper_port = next_stepper_port + 4;
		}
		if(next_stepper_port == -1) next_stepper_port = 0;
		for (int i = 0; i < loop_count ; i++)
		{
			PORTC = (1<<((next_stepper_port) % 4));
			_delay_ms(delay);
			PORTC = (1<<((next_stepper_port + 1) % 4));
			_delay_ms(delay);
			PORTC = (1<<((next_stepper_port + 2) % 4));
			_delay_ms(delay);
			PORTC = (1<<((next_stepper_port + 3) % 4));
			_delay_ms(delay);
		}
		if (remainder == 1)
		{
			PORTC = (1<<((next_stepper_port) % 4));
			_delay_ms(delay);
			next_stepper_port = (next_stepper_port + 1) % 4;
		}
		else if(remainder == 2)
		{
			PORTC = (1<<((next_stepper_port) % 4));
			_delay_ms(delay);
			PORTC = (1<<((next_stepper_port + 1) % 4));
			_delay_ms(delay);
			next_stepper_port = (next_stepper_port + 2) % 4;
		}
		else if(remainder == 3)
		{
			PORTC = (1<<((next_stepper_port) % 4));
			_delay_ms(delay);
			PORTC = (1<<((next_stepper_port + 1) % 4));
			_delay_ms(delay);
			PORTC = (1<<((next_stepper_port + 2) % 4));
			_delay_ms(delay);
			next_stepper_port = (next_stepper_port + 3) % 4;
		}
	}
	else
	{
		if(last_stepper_dir != 'R' && next_stepper_port != -1) next_stepper_port = (next_stepper_port - 2) % 4;
		if(next_stepper_port == -1) next_stepper_port = 3;
		if(next_stepper_port < 3 && next_stepper_port > -1) next_stepper_port = (next_stepper_port + 4);	// prevent -ve numbers when counting modulus
		for (int i = 0; i < loop_count ; i++)
		{
			PORTC = (1<<((next_stepper_port) % 4));
			_delay_ms(delay);
			PORTC = (1<<((next_stepper_port - 1) % 4));
			_delay_ms(delay);
			PORTC = (1<<((next_stepper_port - 2) % 4));
			_delay_ms(delay);
			PORTC = (1<<((next_stepper_port - 3) % 4));
			_delay_ms(delay);
		}
		if (remainder == 1)
		{
			PORTC = (1<<((next_stepper_port) % 4));
			_delay_ms(delay);
			next_stepper_port = (next_stepper_port - 1) % 4;
		}
		else if(remainder == 2)
		{
			PORTC = (1<<((next_stepper_port) % 4));
			_delay_ms(delay);
			PORTC = (1<<((next_stepper_port - 1) % 4));
			_delay_ms(delay);
			next_stepper_port = (next_stepper_port - 2) % 4;
		}
		else if(remainder == 3)
		{
			PORTC = (1<<((next_stepper_port) % 4));
			_delay_ms(delay);
			PORTC = (1<<((next_stepper_port - 1) % 4));
			_delay_ms(delay);
			PORTC = (1<<((next_stepper_port - 2) % 4));
			_delay_ms(delay);
			next_stepper_port = (next_stepper_port - 3) % 4;
		}
	}
	last_stepper_step = step;
	last_stepper_dir = dir;
}

void reset_stepper()
{
	if (last_stepper_dir == 'R')
	{
		set_stepper(last_stepper_step, 'L', 1);
	}
	else
	{
		set_stepper(last_stepper_step, 'R', 1);
	}
	
}