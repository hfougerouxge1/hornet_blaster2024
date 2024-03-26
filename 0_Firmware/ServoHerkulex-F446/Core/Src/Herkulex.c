/*
 Herkulex.c - Library for Herkulex DRS-0101/DRS-0201
 Created by Kailan Paul on 31/05/2023
 Ported from original libraries by Alessandro Giacomel for Arduino
 
 *****************************************************************************
    Herkulex Servo Manual: http://hovis.co.kr/guide/herkulex/drs-0101/%5BENG%5D%20Herkulex%20Manual_20140218.pdf
 *****************************************************************************

  This library was developed for use with an STM32F405 development board and uses the stm32f4xx HAL library.
*/
#include "Herkulex.h"
#include "main.h"
//#include "stm32l0xx_hal.h"
//#include "stm32f4xx_hal_uart.h"

#define T_TIME_OUT 100
#define R_TIME_OUT 5000
#define BROADCAST_ID 0xFE

uint8_t checksum1(uint8_t *data, uint8_t data_size, uint8_t ID, uint8_t cmd);
uint8_t checksum2(uint8_t XOR);
void set_ACK(uint8_t servo_ID, uint8_t val);
void write_register(uint8_t memory, uint8_t servo_ID, uint8_t address,
		uint8_t write_byte);
HAL_StatusTypeDef send_data(uint8_t cmd, uint8_t servo_ID, uint8_t *data,
		uint8_t data_size);
HAL_StatusTypeDef read_data(uint8_t *buffer, uint8_t size);

//initialise all connected servos
void herkulex_init(void) {
	HAL_Delay(100);
	clear_error(BROADCAST_ID); // clear error for all servos
	HAL_Delay(10);
	set_ACK(BROADCAST_ID, 1); // set ACK
	HAL_Delay(10);
	torque_on(BROADCAST_ID); // torqueON for all servos
	HAL_Delay(10);
}

// reboot single servo
void reboot(uint8_t servo_ID) {
	uint8_t data[0];

	send_data(H_REBOOT, servo_ID, data, sizeof(data));
}

// clear errors on the specified servo
void clear_error(uint8_t servo_ID) {
	uint8_t data[4];
	data[0] = 0x30; // Address
	data[1] = 0x02; // Length
	data[2] = 0x00; // Write error=0
	data[3] = 0x00; // Write detail error=0

	send_data(H_RAM_WRITE, servo_ID, data, sizeof(data));

}

// start up the motor on the specified servo
void torque_on(uint8_t servo_ID) {
	uint8_t data[3];
	data[0] = 0x34; // Address
	data[1] = 0x01; // Length
	data[2] = 0x60; // Torque ON

	send_data(H_RAM_WRITE, servo_ID, data, sizeof(data));
}

// turn off the motor on the specified servo
void torque_off(uint8_t servo_ID) {
	uint8_t data[3];
	data[0] = 0x34; // Address
	data[1] = 0x01; // Length
	data[2] = 0x00; // Torque Off

	send_data(H_RAM_WRITE, servo_ID, data, sizeof(data));
}

// set the servo LED to the specified colour
void set_LED(uint8_t servo_ID, uint8_t LED_val) {
	uint8_t data[3];
	data[0] = 0x35;	   // Address
	data[1] = 0x01;	   // Length
	data[2] = LED_val; // LED value

	send_data(H_RAM_WRITE, servo_ID, data, sizeof(data));
}

// set the servo ID - must restart servo to take effect
void set_ID(uint8_t ID_old, uint8_t ID_new) {
	uint8_t data[3];
	data[0] = 0x06;	  // Address
	data[1] = 0x01;	  // Length
	data[2] = ID_new; // New ID

	send_data(H_EEP_WRITE, ID_old, data, sizeof(data));
}


// move continuously
void move_continuous(uint8_t servo_ID, int speed, uint8_t i_LED) {
	uint8_t data[5];
	data[0] = 0; // redundant

	if (speed > 1023 || speed < -1023)
		return; // error

	uint16_t goal_speed;
	if (speed < 0) {
		goal_speed = (-1) * speed;
		goal_speed |= 0x4000; // bit no. 14
	} else {
		goal_speed = speed;
	}
	data[1] = goal_speed & 0x00FF;		  // MSB speedGoal
	data[2] = (goal_speed & 0xFF00) >> 8; // LSB speedGoal

	data[3] = 0b10; // continuous mode

	// led
	switch (i_LED) {
	case 1:
		data[3] += 0b100;
		break;
	case 2:
		data[3] += 0b1000;
		break;
	case 4:
		data[3] += 0b10000;
		break;
	case 7:
		data[3] += 0b11100;
		break;
	}

	data[4] = servo_ID;

	send_data(H_SJOG, servo_ID, data, sizeof(data));
}

// move to specified position 0 - 1023
void move_positional(uint8_t servo_ID, int position, uint16_t p_time,
		uint8_t i_LED) {
	uint8_t data[5];

	if (position > 1023 || position < 0)
		return; // speed (goal) non correct
	if ((p_time < 0) || (p_time > 2856))
		return;
	data[0] = (uint8_t) ((float) p_time / 11.2); // 8. Execution time

	data[1] = position & 0X00FF;		// MSB Pos
	data[2] = (position & 0XFF00) >> 8; // LSB Pos

	data[3] = 0; // positional mode

	// led
	switch (i_LED) {
	case 1:
		data[3] += 0b100;
		break;
	case 2:
		data[3] += 0b1000;
		break;
	case 4:
		data[3] += 0b10000;
		break;
	case 7:
		data[3] += 0b11100;
		break;
	}

	data[4] = servo_ID; // 11. ServoID

	send_data(H_SJOG, servo_ID, data, sizeof(data));
}

// move to specified angle 160.0 -160.0
void move_angle(uint8_t servo_ID, float angle, uint16_t p_time, uint8_t i_LED) {
	if (angle > 160.0 || angle < -160.0)
		return;
	int position = (int) (angle / 0.325) + 512;
	move_positional(servo_ID, position, p_time, i_LED);
}

//######################################## UNFINISHED ########################################

// move all servo at the same time to a position: servo list building
// void move_all_angle(uint8_t servo_ID, float angle, uint8_t i_LED)
//{
//	if (angle > 160.0 || angle < -160.0)
//		return; // out of the range
//	int position = (int)(angle / 0.325) + 512;
//	move_all(servo_ID, position, i_LED);
//}

// move all servo at the same time with different speeds: servo list building
// void move_speed_all(uint8_t servo_ID, int goal, uint8_t i_LED)
//{
//	if (goal > 1023 || goal < -1023)
//		return; //-1023 <--> 1023 range
//
//	uint8_t i_mode = 1; // mode=continous rotation
//	uint8_t i_stop = 0; // Stop=0
//
//	// Speed definition
//	uint16_t goal_speed_sign;
//	if (goal < 0)
//	{
//		goal_speed_sign = (-1) * goal;
//		goal_speed_sign |= 0x4000; // bit n�14
//	}
//	else
//	{
//		goal_speed_sign = goal;
//	}
//
//	uint16_t goal_speed_LSB = goal_speed_sign & 0X00FF;		   // MSB speedGoal
//	uint16_t goal_speed_MSB = (goal_speed_sign & 0xFF00) >> 8; // LSB speedGoal
//
//	// led
//	uint8_t i_blue = 0;
//	uint8_t i_green = 0;
//	uint8_t i_red = 0;
//	switch (i_LED)
//	{
//	case 1:
//		i_green = 1;
//		break;
//	case 2:
//		i_blue = 1;
//		break;
//	case 3:
//		i_red = 1;
//		break;
//	}
//
//	uint8_t set_value = i_stop + i_mode * 2 + i_green * 4 + i_blue * 8 + i_red * 16; // assign led value

//	add_data(goal_speed_LSB, goal_speed_MSB, set_value, servo_ID, count, move_data); // add servo data to list, speed mode
//}

// move all servo with the same execution time
// void action_all(uint16_t p_time)
//{
//	if ((p_time < 0) || (p_time > 2856))
//		return;

//	p_size = 0x08 + count;					  // 3.Packet size 7-58
//	cmd = HSJOG;							  // 5. CMD SJOG Write n servo with same execution time
//	x_time = (uint8_t)((float)p_time / 11.2); // 8. Execution time
//
//	p_ID = 0xFE ^ x_time;
//	ck1 = checksum1(move_data, count); // 6. Checksum1
//	ck2 = checksum2(ck1);			   // 7. Checksum2
//
//	p_ID = 0xFE;
//	data_ex[0] = 0xFF;	 // Packet Header
//	data_ex[1] = 0xFF;	 // Packet Header
//	data_ex[2] = p_size; // Packet Size
//	data_ex[3] = p_ID;	 // Servo ID
//	data_ex[4] = cmd;	 // Command Ram Write
//	data_ex[5] = ck1;	 // Checksum 1
//	data_ex[6] = ck2;	 // Checksum 2
//	data_ex[7] = x_time; // Execution time
//
//	for (uint8_t i = 0; i < count; i++)
//		data_ex[i + 8] = move_data[i]; // Variable servo data
//
//	send_data(data_ex, p_size);
//
//	count = 0; // reset counter
//}

//############################################################################################

//returns the status of the servo
uint8_t get_status(uint8_t servo_ID) {
	uint8_t data[0];
	send_data(H_STAT, servo_ID, data, sizeof(data));

	uint8_t r_data[2];
	read_data(r_data, sizeof(r_data));

	return r_data[0];
}

// get position
uint16_t get_position(uint8_t servo_ID) {
	uint16_t position = 0;

	uint8_t data[2];
	data[0] = 0x3A; // Address
	data[1] = 0x02; // Length

	send_data(H_RAM_READ, servo_ID, data, sizeof(data));

	uint8_t r_data[6];
	read_data(r_data, sizeof(r_data));

	position = ((r_data[3] & 0x03) << 8) | r_data[2];

	return position;
}

float get_angle(uint8_t servo_ID) {
	int pos = (int) get_position(servo_ID);
	return (pos - 512) * 0.325;
}

// get the speed for one servo -1023 - 1023
uint16_t get_speed(uint8_t servo_ID) {
	uint16_t speed = 0;

	uint8_t data[2];
	data[0] = 0x40; // Address
	data[1] = 0x02; // Length

	send_data(H_RAM_READ, servo_ID, data, sizeof(data));

	uint8_t r_data[6];
	read_data(r_data, sizeof(r_data));

	speed = ((r_data[3] & 0xFF) << 8) | r_data[2];
	return speed;
}

uint8_t checksum1(uint8_t *data, uint8_t data_size, uint8_t ID, uint8_t cmd) {
	uint8_t XOR = 0;
	XOR = XOR ^ (data_size + 7);
	XOR = XOR ^ ID;
	XOR = XOR ^ cmd;
	for (uint8_t i = 0; i < data_size; i++) {
		XOR = XOR ^ data[i];
	}
	return XOR & 0xFE;
}

uint8_t checksum2(uint8_t XOR) {
	return (~XOR) & 0xFE;
}

// ACK  - 0=No reply, 1=Only reply to READ CMD, 2=Always reply
void set_ACK(uint8_t servo_ID, uint8_t val) {
	uint8_t data[3];
	data[0] = 0x34; // Address
	data[1] = 0x01; // Length
	data[2] = val;

	send_data(H_RAM_WRITE, servo_ID, data, sizeof(data));
}

// write to servo memory
void write_register(uint8_t memory, uint8_t servo_ID, uint8_t address,
		uint8_t write_byte) {
	uint8_t data[3];
	data[0] = address;	  // Address
	data[1] = 0x01;		  // Length
	data[2] = write_byte; // Write error=0

	send_data(memory, servo_ID, data, sizeof(data));
}

// add data to variable list servo for syncro execution
// void add_data(uint16_t goal_LSB, uint16_t goal_MSB, uint8_t set, uint8_t servo_ID, uint16_t count, uint8_t* move_data)
//{
//	move_data[count++] = goal_LSB;
//	move_data[count++] = goal_MSB;
//	move_data[count++] = set;
//	move_data[count++] = servo_ID;
//}

HAL_StatusTypeDef send_data(uint8_t cmd, uint8_t servo_ID, uint8_t *data,
		uint8_t data_size) {
	uint8_t buffer_size = data_size + 7;
	uint8_t buffer[buffer_size];
	buffer[0] = 0xFF;									   // Packet Header
	buffer[1] = 0xFF;									   // Packet Header
	buffer[2] = buffer_size;							   // Packet Size 7-58
	buffer[3] = servo_ID;								   // Servo ID
	buffer[4] = cmd;									   // command
	buffer[5] = checksum1(data, data_size, servo_ID, cmd); // Checksum 1
	buffer[6] = checksum2(buffer[5]);					   // Checksum 2

	for (uint8_t i = 0; i < data_size; i++)
		buffer[i + 7] = data[i];

	if (HAL_UART_Transmit(&huart1, buffer, buffer_size, T_TIME_OUT) != HAL_OK)
		return HAL_ERROR;

	return HAL_OK;
}

HAL_StatusTypeDef read_data(uint8_t *data, uint8_t data_size) {
	uint8_t buffer_size = data_size + 7;
	uint8_t buffer[buffer_size];

	HAL_StatusTypeDef status = HAL_BUSY;
	while (status != HAL_OK) {
		status = HAL_UART_Receive(&huart1, buffer, buffer_size, R_TIME_OUT);
		if ((status != HAL_OK) && (status != HAL_BUSY))
			return status;
	}

	for (uint8_t i = 0; i < data_size; i++)
		data[i] = buffer[i + 7];

	uint8_t ck1 = checksum1(data, data_size, buffer[3], buffer[4]); // 6. Checksum1
	uint8_t ck2 = checksum2(ck1);				  // 7. Checksum2

	if (ck1 != buffer[5])
		return HAL_ERROR;
	if (ck2 != buffer[6])
		return HAL_ERROR;

	return HAL_OK;
}
