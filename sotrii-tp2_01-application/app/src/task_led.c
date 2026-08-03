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
#include "task_led_attribute.h"

/********************** macros and definitions *******************************/
#define G_TASK_LED_CNT_INI	0ul

#define DEL_LED_MIN			(pdMS_TO_TICKS(50ul))
#define DEL_LED_BLINK		(pdMS_TO_TICKS(500ul))

#define TASK_LED_DEL_ZERO	(pdMS_TO_TICKS(0ul))
#define TASK_LED_DEL_MAX	DEL_LED_MIN
#define LED_AO_QUEUE_LENGTH (1u)

/********************** internal data declaration ****************************/
led_t led[LED_QTY] = {{LED_A, LED_A_PORT, LED_A_PIN, LED_A_OFF},
			     	  {LED_B, LED_B_PORT, LED_B_PIN, LED_B_OFF},
					  {LED_C, LED_C_PORT, LED_C_PIN, LED_C_OFF}};

led_sc_t led_sc[LED_QTY] = {{ST_LED_OFF, EV_LED_NONE, ZERO},
							{ST_LED_OFF, EV_LED_NONE, ZERO},
							{ST_LED_OFF, EV_LED_NONE, ZERO}};

/********************** internal functions declaration ***********************/
void task_led_statechart(h_led_t *h_led_);
void task_led(void *parameters);

/********************** internal data definition *****************************/

/********************** external data declaration ****************************/
uint32_t g_task_led_cnt;

h_led_t h_led[LED_QTY] = {
	{.led = &led[LED_A], .led_sc = &led_sc[LED_A], .ao_id = LED_A, .blink_period = DEL_LED_BLINK},
	{.led = &led[LED_B], .led_sc = &led_sc[LED_B], .ao_id = LED_B, .blink_period = DEL_LED_BLINK},
	{.led = &led[LED_C], .led_sc = &led_sc[LED_C], .ao_id = LED_C, .blink_period = DEL_LED_BLINK}};

volatile uint32_t g_open_led_ao_wcet_cycles;
volatile uint32_t g_release_led_ao_wcet_cycles;
volatile uint32_t g_send_led_ao_wcet_cycles;
volatile uint32_t g_ioctl_led_ao_wcet_cycles;

static void update_wcet(volatile uint32_t *wcet, uint32_t start)
{
	uint32_t elapsed = cycle_counter_get() - start;
	if (elapsed > *wcet)
	{
		*wcet = elapsed;
	}
}

led_ao_status_t open_led_ao(h_led_t *ao)
{
	uint32_t start = cycle_counter_get();
	led_ao_status_t status = LED_AO_ERROR;

	if ((NULL == ao) || (ao->ao_id >= LED_QTY))
	{
		status = LED_AO_INVALID_ARG;
	}
	else if (pdTRUE == ao->is_open)
	{
		status = LED_AO_OK;
	}
	else
	{
		if (NULL != ao->ao_queue)
		{
			ao->is_open = pdTRUE;
			status = LED_AO_OK;
		}
	}
	update_wcet(&g_open_led_ao_wcet_cycles, start);
	return status;
}

led_ao_status_t release_led_ao(h_led_t *ao)
{
	uint32_t start = cycle_counter_get();
	led_ao_status_t status = LED_AO_INVALID_ARG;

	if (NULL != ao)
	{
		status = LED_AO_NOT_OPEN;
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
			status = LED_AO_OK;
		}
	}
	update_wcet(&g_release_led_ao_wcet_cycles, start);
	return status;
}

led_ao_status_t send_led_ao(h_led_t *ao, led_ev_t event, TickType_t timeout)
{
	uint32_t start = cycle_counter_get();
	led_ao_status_t status = LED_AO_INVALID_ARG;
	led_ao_msg_t message = {.event = event, .requester = xTaskGetCurrentTaskHandle()};

	if ((NULL != ao) && (event < EV_LED_NONE))
	{
		status = LED_AO_NOT_OPEN;
		if (pdTRUE == ao->is_open)
		{
			status = LED_AO_TIMEOUT;
			if (pdPASS == xQueueSend(ao->ao_queue, &message, timeout))
			{
				status = (0ul < ulTaskNotifyTake(pdTRUE, timeout)) ? LED_AO_OK : LED_AO_TIMEOUT;
			}
		}
	}
	update_wcet(&g_send_led_ao_wcet_cycles, start);
	return status;
}

led_ao_status_t ioctl_led_ao(h_led_t *ao, led_ao_ioctl_cmd_t command, void *argument)
{
	uint32_t start = cycle_counter_get();
	led_ao_status_t status = LED_AO_INVALID_ARG;

	if ((NULL != ao) && (NULL != argument) && (pdTRUE == ao->is_open))
	{
		taskENTER_CRITICAL();
		switch (command)
		{
			case LED_AO_IOCTL_GET_STATE:
				*(led_st_t *)argument = ao->led_sc->state;
				status = LED_AO_OK;
				break;
			case LED_AO_IOCTL_GET_PIN_STATE:
				*(GPIO_PinState *)argument = ao->led->pin_state;
				status = LED_AO_OK;
				break;
			case LED_AO_IOCTL_SET_BLINK_PERIOD:
				if (*(TickType_t *)argument >= DEL_LED_MIN)
				{
					ao->blink_period = *(TickType_t *)argument;
					status = LED_AO_OK;
				}
				break;
			default:
				break;
		}
		taskEXIT_CRITICAL();
	}
	else if ((NULL != ao) && (pdFALSE == ao->is_open))
	{
		status = LED_AO_NOT_OPEN;
	}
	update_wcet(&g_ioctl_led_ao_wcet_cycles, start);
	return status;
}

/********************** external functions definition ************************/
/* Task thread */
void task_led(void *parameters)
{
	/*  Declare & Initialize Task Function variables */
	g_task_led_cnt = G_TASK_LED_CNT_INI;
	h_led_t *p_h_led = (h_led_t *)parameters;
	led_ao_msg_t message;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", pcTaskGetName(NULL), xTaskGetTickCount());

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
    {
		/* Update Task Counter */
		g_task_led_cnt++;

		/* Get Events to excite Statechart */
		if (pdPASS == xQueueReceive(p_h_led->ao_queue, &message, TASK_LED_DEL_MAX))
		{
			p_h_led->led_sc->ev_in = message.event;
			LOGGER_INFO("LED AO received id=%u ev=%u",
					(unsigned int)p_h_led->ao_id,
					(unsigned int)message.event);
		}
		else
		{
			p_h_led->led_sc->ev_in = EV_LED_NONE;
			message.requester = NULL;
		}

		/* Run Statechart */
    	task_led_statechart(p_h_led);

		if (NULL != message.requester)
		{
			LOGGER_INFO("LED AO processed id=%u ev=%u state=%u",
					(unsigned int)p_h_led->ao_id,
					(unsigned int)message.event,
					(unsigned int)p_h_led->led_sc->state);
			xTaskNotifyGive(message.requester);
			LOGGER_INFO("LED->SYS confirmation sent");
		}
	}
}

void task_led_statechart(h_led_t *h_led_)
{
	switch (h_led_->led_sc->state)
	{
		case ST_LED_OFF:
		case ST_LED_ON:

			switch (h_led_->led_sc->ev_in)
			{
				case EV_LED_OFF:

					h_led_->led_sc->state = ST_LED_OFF;
					h_led_->led->pin_state = LED_OFF;
					h_led_->led_sc->tick = ZERO;

					HAL_GPIO_WritePin(h_led_->led->gpio_port, h_led_->led->pin, h_led_->led->pin_state);

					break;

				case EV_LED_ON:

					h_led_->led_sc->state = ST_LED_ON;
					h_led_->led->pin_state = LED_ON;
					h_led_->led_sc->tick = ZERO;

					HAL_GPIO_WritePin(h_led_->led->gpio_port, h_led_->led->pin, h_led_->led->pin_state);

					break;

				case EV_LED_BLINK:

					h_led_->led_sc->state = ST_LED_BLINK;
					h_led_->led->pin_state = HAL_GPIO_ReadPin(h_led_->led->gpio_port, h_led_->led->pin);
					h_led_->led_sc->tick = h_led_->blink_period;

					HAL_GPIO_TogglePin(h_led_->led->gpio_port, h_led_->led->pin);

					break;

				case EV_LED_NONE:

					break;
			}

			break;

		case ST_LED_BLINK:

			switch (h_led_->led_sc->ev_in)
			{
				case EV_LED_OFF:

					h_led_->led_sc->state = ST_LED_OFF;
					h_led_->led->pin_state = LED_OFF;
					h_led_->led_sc->tick = ZERO;

					HAL_GPIO_WritePin(h_led_->led->gpio_port, h_led_->led->pin, h_led_->led->pin_state);

					break;

				case EV_LED_ON:

					h_led_->led_sc->state = ST_LED_ON;
					h_led_->led->pin_state = LED_ON;
					h_led_->led_sc->tick = ZERO;

					HAL_GPIO_WritePin(h_led_->led->gpio_port, h_led_->led->pin, h_led_->led->pin_state);

					break;

				case EV_LED_BLINK:
				case EV_LED_NONE:

					h_led_->led_sc->state = ST_LED_BLINK;
					h_led_->led_sc->tick -= DEL_LED_MIN;

					if (ZERO == h_led_->led_sc->tick)
					{
						h_led_->led->pin_state = HAL_GPIO_ReadPin(h_led_->led->gpio_port, h_led_->led->pin);
						h_led_->led_sc->tick = h_led_->blink_period;

						HAL_GPIO_TogglePin(h_led_->led->gpio_port, h_led_->led->pin);
						LOGGER_INFO("LED AO id=%u blink toggle",
								(unsigned int)h_led_->ao_id);
					}

					break;
			}

			break;
	}
}

/********************** end of file ******************************************/
