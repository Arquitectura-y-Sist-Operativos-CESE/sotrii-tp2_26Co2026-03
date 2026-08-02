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

/********************** inclusions *******************************************/
/* Project includes */
#include "main.h"
#include "cmsis_os.h"

/* Demo includes */
#include "logger.h"
#include "dwt.h"

/* Application & Tasks includes */
#include "board.h"
#include "app.h"
#include "task_sys_attribute.h"

/********************** macros and definitions *******************************/
#define G_TASK_SYS_CNT_INI	0ul

#define DEL_SYS_MIN			(pdMS_TO_TICKS(50ul))
#define DEL_SYS_BLINK		(pdMS_TO_TICKS(500ul))

#define TASK_SYS_DEL_ZERO	(pdMS_TO_TICKS(0ul))
#define TASK_SYS_DEL_MAX	DEL_SYS_MIN

/********************** internal data declaration ****************************/
sys_sc_t sys_sc = {ST_SYS_IDLE, EV_SYS_OFF, ZERO, EV_SYS_NONE, ZERO};

/********************** internal functions declaration ***********************/
void task_sys_statechart(h_sys_t *h_sys_);
void task_sys(void *parameters);

/********************** internal data definition *****************************/

/********************** external data declaration ****************************/
uint32_t g_task_sys_cnt;

h_sys_t h_sys = {.sys_sc = &sys_sc, .ao_id = SYS_AO_0};

volatile uint32_t g_open_sys_ao_wcet_cycles;
volatile uint32_t g_release_sys_ao_wcet_cycles;
volatile uint32_t g_send_sys_ao_wcet_cycles;
volatile uint32_t g_ioctl_sys_ao_wcet_cycles;

static void update_wcet(volatile uint32_t *wcet, uint32_t start)
{
	uint32_t elapsed = cycle_counter_get() - start;
	if (elapsed > *wcet)
	{
		*wcet = elapsed;
	}
}

sys_ao_status_t open_sys_ao(h_sys_t *ao)
{
	uint32_t start = cycle_counter_get();
	sys_ao_status_t status = SYS_AO_INVALID_ARG;

	if ((NULL != ao) && (ao->ao_id < SYS_AO_QTY))
	{
		status = SYS_AO_ERROR;
		if (pdTRUE == ao->is_open)
		{
			status = SYS_AO_OK;
		}
		else if (NULL != ao->ao_queue)
		{
			ao->is_open = pdTRUE;
			status = SYS_AO_OK;
		}
	}
	update_wcet(&g_open_sys_ao_wcet_cycles, start);
	return status;
}

sys_ao_status_t release_sys_ao(h_sys_t *ao)
{
	uint32_t start = cycle_counter_get();
	sys_ao_status_t status = SYS_AO_INVALID_ARG;

	if (NULL != ao)
	{
		status = SYS_AO_NOT_OPEN;
		if (pdTRUE == ao->is_open)
		{
			if (NULL != ao->ao_task)
			{
				vTaskDelete(ao->ao_task);
			}
			vQueueUnregisterQueue(ao->ao_queue);
			vQueueDelete(ao->ao_queue);
			ao->ao_task = NULL;
			ao->ao_queue = NULL;
			ao->is_open = pdFALSE;
			status = SYS_AO_OK;
		}
	}
	update_wcet(&g_release_sys_ao_wcet_cycles, start);
	return status;
}

sys_ao_status_t send_sys_ao(h_sys_t *ao, sys_ev_t event,
		TickType_t time, TickType_t timeout)
{
	uint32_t start = cycle_counter_get();
	sys_ao_status_t status = SYS_AO_INVALID_ARG;
	sys_ao_msg_t message = {
		.event = event,
		.time = time,
		.requester = xTaskGetCurrentTaskHandle()};

	if ((NULL != ao) && (event < EV_SYS_NONE))
	{
		status = SYS_AO_NOT_OPEN;
		if (pdTRUE == ao->is_open)
		{
			status = SYS_AO_TIMEOUT;
			if (pdPASS == xQueueSend(ao->ao_queue, &message, timeout))
			{
				status = (0ul < ulTaskNotifyTake(pdTRUE, timeout)) ?
						SYS_AO_OK : SYS_AO_TIMEOUT;
			}
		}
	}
	update_wcet(&g_send_sys_ao_wcet_cycles, start);
	return status;
}

sys_ao_status_t ioctl_sys_ao(h_sys_t *ao,
		sys_ao_ioctl_cmd_t command, void *argument)
{
	uint32_t start = cycle_counter_get();
	sys_ao_status_t status = SYS_AO_INVALID_ARG;

	if ((NULL != ao) && (NULL != argument) && (pdTRUE == ao->is_open))
	{
		taskENTER_CRITICAL();
		switch (command)
		{
			case SYS_AO_IOCTL_GET_STATE:
				*(sys_st_t *)argument = ao->sys_sc->state;
				status = SYS_AO_OK;
				break;
			case SYS_AO_IOCTL_GET_ELAPSED_TIME:
				*(TickType_t *)argument = ao->sys_sc->tick;
				status = SYS_AO_OK;
				break;
			default:
				break;
		}
		taskEXIT_CRITICAL();
	}
	else if ((NULL != ao) && (pdFALSE == ao->is_open))
	{
		status = SYS_AO_NOT_OPEN;
	}
	update_wcet(&g_ioctl_sys_ao_wcet_cycles, start);
	return status;
}

/********************** external functions definition ************************/
/* Task thread */
void task_sys(void *parameters)
{
	/*  Declare & Initialize Task Function variables */
	g_task_sys_cnt = G_TASK_SYS_CNT_INI;
	h_sys_t *p_h_sys = (h_sys_t *)parameters;
	sys_ao_msg_t message;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", pcTaskGetName(NULL), xTaskGetTickCount());

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
    {
		/* Update Task Counter */
		g_task_sys_cnt++;

		/* Get Events to excite Statechart */
		if (pdPASS == xQueueReceive(p_h_sys->ao_queue, &message, (TickType_t)ZERO))
		{
			p_h_sys->sys_sc->ev_in = message.event;
			p_h_sys->sys_sc->tick_out = message.time;
			LOGGER_INFO("SYS AO recv ev=%u time=%lu",
					(unsigned int)message.event, (unsigned long)message.time);
		}
		else
		{
			p_h_sys->sys_sc->ev_in = EV_SYS_NONE;
			message.requester = NULL;
		}

		/* Run Statechart */
    	task_sys_statechart(p_h_sys);

		if (NULL != message.requester)
		{
			LOGGER_INFO("SYS AO processed ev=%u state=%u",
					(unsigned int)message.event,
					(unsigned int)p_h_sys->sys_sc->state);
			xTaskNotifyGive(message.requester);
			LOGGER_INFO("SYS->BTN confirmation sent");
		}

    	/* We want this task to execute every 50 milliseconds. */
		vTaskDelay(TASK_SYS_DEL_MAX);
	}
}

void task_sys_statechart(h_sys_t *h_sys_)
{
	switch (h_sys_->sys_sc->state)
	{
		case ST_SYS_IDLE:

			if (EV_SYS_ON == h_sys_->sys_sc->ev_in)
			{
				h_sys_->sys_sc->state = ST_SYS_ACTIVE_0;
				h_sys_->sys_sc->tick = ZERO;
				h_sys_->sys_sc->ev_out = EV_SYS_ON;

				xQueueSend(h_led_task_q, (void *)&h_sys_->sys_sc->ev_out, (TickType_t)ZERO);
			}
			else
			{
				h_sys_->sys_sc->tick += DEL_SYS_MIN;
			}

			break;

		case ST_SYS_ACTIVE_0:

			if (EV_SYS_ON == h_sys_->sys_sc->ev_in)
			{
				h_sys_->sys_sc->state = ST_SYS_ACTIVE_1;
				h_sys_->sys_sc->tick = ZERO;
				h_sys_->sys_sc->ev_out = EV_SYS_BLINK;

				xQueueSend(h_led_task_q, (void *)&h_sys_->sys_sc->ev_out, (TickType_t)ZERO);
			}
			else
			{
				h_sys_->sys_sc->tick += DEL_SYS_MIN;
			}


			break;

		case ST_SYS_ACTIVE_1:

			if (EV_SYS_ON == h_sys_->sys_sc->ev_in)
			{
				h_sys_->sys_sc->state = ST_SYS_IDLE;
				h_sys_->sys_sc->tick = ZERO;
				h_sys_->sys_sc->ev_out = EV_SYS_OFF;

				xQueueSend(h_led_task_q, (void *)&h_sys_->sys_sc->ev_out, ZERO);
			}
			else
			{
				h_sys_->sys_sc->tick += DEL_SYS_MIN;
			}


			break;
	}
}

/********************** end of file ******************************************/
