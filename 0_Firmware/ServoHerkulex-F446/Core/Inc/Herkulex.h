/*
 Herkulex.h - Library for Herkulex DRS-0101/DRS-0201
 Created by Kailan Paul on 31/05/2023
 Ported from original libraries by Alessandro Giacomel for Arduino

 *****************************************************************************
    Herkulex Servo Manual: http://hovis.co.kr/guide/herkulex/drs-0101/%5BENG%5D%20Herkulex%20Manual_20140218.pdf
 *****************************************************************************

  This library was developed for use with an STM32F405 development board and uses the stm32f4xx HAL library.
*/
#ifndef Herkulex_h
#define Herkulex_h

#include "stm32f4xx_hal.h"

enum H_COMMANDS {
	H_EEP_WRITE = 1, // Rom write
	H_EEP_READ,      // Rom read
	H_RAM_WRITE,     // Ram write
	H_RAM_READ,      // Ram read
	H_IJOG,          // Write n servo with different timing
	H_SJOG,          // Write n servo with same time
	H_STAT,          // Read error
	H_ROLLBACK,      // Back to factory value
	H_REBOOT         // Reboot
};

enum H_LEDS {
	H_LED_OFF,
	H_LED_GREEN,
	H_LED_BLUE,
	H_LED_CYAN,
	H_LED_RED,
	H_LED_GREEN2,
	H_LED_PINK,
	H_LED_WHITE
};


extern UART_HandleTypeDef huart1;

void herkulex_init(void);

void reboot(uint8_t servo_ID);
void clear_error(uint8_t servo_ID);
void torque_on(uint8_t servo_ID);
void torque_off(uint8_t servo_ID);
void set_LED(uint8_t servo_ID, uint8_t LED_val);
void set_ID(uint8_t ID_old, uint8_t ID_new);

void move_continuous(uint8_t servo_ID, int speed, uint8_t i_LED);
void move_positional(uint8_t servo_ID, int position, uint16_t p_time,
		uint8_t i_LED);
void move_angle(uint8_t servo_ID, float angle, uint16_t p_time, uint8_t i_LED);

uint8_t get_status(uint8_t servo_ID);
uint16_t get_position(uint8_t servo_ID);
float get_angle(uint8_t servo_ID);
uint16_t get_speed(uint8_t servo_ID);

#endif // Herkulex_h
