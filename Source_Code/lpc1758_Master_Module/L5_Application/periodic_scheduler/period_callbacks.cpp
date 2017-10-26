/*
 *     SocialLedge.com - Copyright (C) 2013
 *
 *     This file is part of free software framework for embedded processors.
 *     You can use it and/or distribute it as long as this copyright header
 *     remains unmodified.  The code is free for personal use and requires
 *     permission to use in a commercial product.
 *
 *      THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 *      OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 *      MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 *      I SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
 *      CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 *     You can reach the author of this software at :
 *          p r e e t . w i k i @ g m a i l . c o m
 */

/**
 * @file
 * This contains the period callback functions for the periodic scheduler
 *
 * @warning
 * These callbacks should be used for hard real-time system, and the priority of these
 * tasks are above everything else in the system (above the PRIORITY_CRITICAL).
 * The period functions SHOULD NEVER block and SHOULD NEVER run over their time slot.
 * For example, the 1000Hz take slot runs periodically every 1ms, and whatever you
 * do must be completed within 1ms.  Running over the time slot will reset the system.
 */

#include <stdint.h>
#include "io.hpp"
#include "periodic_callback.h"
#include "_can_dbc/generated_can.h"
#include "can.h"
#include "switches.hpp"
#include "string.h"
#include "printf_lib.h"

Switches& SW_obj = Switches::getInstance();

/// This is the stack size used for each of the period tasks (1Hz, 10Hz, 100Hz, and 1000Hz)
const uint32_t PERIOD_TASKS_STACK_SIZE_BYTES = (512 * 4);

/**
 * This is the stack size of the dispatcher task that triggers the period tasks to run.
 * Minimum 1500 bytes are needed in order to write a debug file if the period tasks overrun.
 * This stack size is also used while calling the period_init() and period_reg_tlm(), and if you use
 * printf inside these functions, you need about 1500 bytes minimum
 */
const uint32_t PERIOD_MONITOR_TASK_STACK_SIZE_BYTES = (512 * 3);
const uint32_t            SENSOR_SONARS__MIA_MS = 1000;
const SENSOR_SONARS_t     SENSOR_SONARS__MIA_MSG = { 999,999,999,999 };
SENSOR_SONARS_t sensor_msg;

/// Called once before the RTOS is started, this is a good place to initialize things once
bool period_init(void)
{
	SW_obj.init();
	//initializing the CAN bus
	CAN_init(can2, 100, 5, 5, nullptr, nullptr);
	//if receiving enable this code
	CAN_bypass_filter_accept_all_msgs();
	//resetting
	CAN_reset_bus(can2);
	return true; // Must return true upon success
}

/// Register any telemetry variables
bool period_reg_tlm(void)
{
    // Make sure "SYS_CFG_ENABLE_TLM" is enabled at sys_config.h to use Telemetry
    return true; // Must return true upon success
}

/**
 * Below are your periodic functions.
 * The argument 'count' is the number of times each periodic task is called.
 */

void period_1Hz(uint32_t count)
{
	if(CAN_is_bus_off(can2))
		CAN_reset_bus(can2);
}

bool dbc_app_send_can_msg(uint32_t mid, uint8_t dlc, uint8_t bytes[8])
{
	can_msg_t msg;
	msg.msg_id = mid;
	msg.frame_fields.data_len = dlc;
	msg.frame_fields.is_29bit = 0;
	msg.frame_fields.is_rtr = 0;

	memcpy(msg.data.bytes, bytes, dlc);

	return CAN_tx(can2, &msg , 10);

}
void period_10Hz(uint32_t count)
{
	MOTOR_SIGNAL_t motor_msg = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	if(SW_obj.getSwitch(1))
	{
		motor_msg.MOTOR_STEER_ANGLE = 10.1;
		motor_msg.MOTOR_STEER_FULL_LEFT = 1;
	}
	else if(SW_obj.getSwitch(2))
	{
		motor_msg.MOTOR_STEER_ANGLE = 19.8;
		motor_msg.MOTOR_STEER_FULL_RIGHT = 1;
	}
	else
	{
		motor_msg.MOTOR_STEER_ANGLE = 15.1;
		motor_msg.MOTOR_STEER_STRAIGHT = 1;
	}

	if(SW_obj.getSwitch(3))
	{
		motor_msg.MOTOR_DRIVE_SPEED = 15;
		motor_msg.MOTOR_DRIVE_FORWARD = 1;
		u0_dbg_printf("forward");
	}
	else if(SW_obj.getSwitch(4))
	{
		motor_msg.MOTOR_DRIVE_SPEED = 10;
		motor_msg.MOTOR_DRIVE_REVERSE = 1;
	}
	else
	{
		motor_msg.MOTOR_DRIVE_SPEED = 20;
		motor_msg.MOTOR_DRIVE_NEUTRAL = 1;
	}

	dbc_encode_and_send_MOTOR_SIGNAL(&motor_msg);
}

void period_100Hz(uint32_t count)
{

	can_msg_t sensor;
	if (!CAN_is_bus_off(can2)) {
		while (CAN_rx(can2, &sensor, 0)) {
			dbc_msg_hdr_t can_msg_hdr;
			can_msg_hdr.dlc = sensor.frame_fields.data_len;
			can_msg_hdr.mid = sensor.msg_id;
			dbc_decode_SENSOR_SONARS(&sensor_msg, sensor.data.bytes,
					&can_msg_hdr);
			//u0_dbg_printf("L-M-R %d %d %d\n", sensor_msg.SENSOR_SONARS_left,
			//		sensor_msg.SENSOR_SONARS_middle,
			//		sensor_msg.SENSOR_SONARS_right);
			u0_dbg_printf("%d\n",sensor_msg.SENSOR_SONARS_left);
			 LE.set(1,0);
		}

		if (dbc_handle_mia_SENSOR_SONARS(&sensor_msg, 10)) {
			 LE.set(1,1);
			u0_dbg_printf("MIA\n");
		}
	}
	else
	{
		CAN_reset_bus(can2);
	}

	// LE.toggle(3);
}

// 1Khz (1ms) is only run if Periodic Dispatcher was configured to run it at main():
// scheduler_add_task(new periodicSchedulerTask(run_1Khz = true));
void period_1000Hz(uint32_t count)
{
  //  LE.toggle(4);
}
