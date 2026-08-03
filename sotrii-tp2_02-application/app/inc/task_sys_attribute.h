/*
 * Copyright (c) 2026 Sebastian Bedin <sebabedin@gmail.com> &
 * 					  Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @author : Sebastian Bedin <sebabedin@gmail.com> &
 * 			 Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>
 */

#ifndef TASK_SYS_ATTRIBUTE_H_
#define TASK_SYS_ATTRIBUTE_H_

/********************** CPP guard ********************************************/
#ifdef __cplusplus
extern "C" {
#endif

/********************** inclusions *******************************************/
#include "task_btn_attribute.h"

/********************** macros ***********************************************/
/* Events of Statechart */
typedef enum sys_ev {EV_SYS_OFF = EV_BTN_UP,
					 EV_SYS_ON = EV_BTN_DOWN,
					 EV_SYS_BLINK,
					 EV_SYS_NONE} sys_ev_t;

/* States of Statechart */
typedef enum sys_st {ST_SYS_IDLE,
					 ST_SYS_ACTIVE_0,
					 ST_SYS_ACTIVE_1} sys_st_t;

typedef enum sys_ao_id {SYS_AO_0,
						SYS_AO_QTY} sys_ao_id_t;

typedef enum
{
	SYS_AO_IOCTL_GET_STATE,
	SYS_AO_IOCTL_GET_ELAPSED_TIME
} sys_ao_ioctl_cmd_t;

typedef enum
{
	SYS_AO_OK = 0,
	SYS_AO_ERROR = -1,
	SYS_AO_TIMEOUT = -2,
	SYS_AO_NOT_OPEN = -3,
	SYS_AO_INVALID_ARG = -4
} sys_ao_status_t;

/* Input data required by TP2-02: event and time. */
typedef struct
{
	sys_ev_t event;
	TickType_t time;
	TaskHandle_t requester;
} sys_ao_msg_t;

/********************** typedef **********************************************/
/* Structure of Statechart */
typedef struct
{
	sys_st_t		state;
	sys_ev_t		ev_in;
	TickType_t		tick;
	sys_ev_t 		ev_out;
	TickType_t	 	tick_out;
} sys_sc_t;

/* Structure of Task */
typedef struct
{
	sys_sc_t *	sys_sc;
	sys_ao_id_t ao_id;
	QueueHandle_t ao_queue;
	TaskHandle_t ao_task;
	BaseType_t is_open;
} h_sys_t;

/********************** external data declaration ****************************/

/********************** external functions declaration ***********************/

/********************** End of CPP guard *************************************/
#ifdef __cplusplus
}
#endif

#endif /* TASK_SYS_ATTRIBUTE_H_ */

/********************** end of file ******************************************/
