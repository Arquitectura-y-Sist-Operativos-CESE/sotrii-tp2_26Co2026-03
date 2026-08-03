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

/********************** internal data declaration ****************************/
led_t led[LED_QTY] = {{LED_A, LED_A_PORT, LED_A_PIN, LED_A_OFF},
			     	  {LED_B, LED_B_PORT, LED_B_PIN, LED_B_OFF},
					  {LED_C, LED_C_PORT, LED_C_PIN, LED_C_OFF}};

led_sc_t led_sc[LED_QTY] = {{ST_LED_OFF, EV_LED_NONE, ZERO},
							{ST_LED_OFF, EV_LED_NONE, ZERO},
							{ST_LED_OFF, EV_LED_NONE, ZERO}};

/********************** internal functions declaration ***********************/
void task_led_statechart(h_led_t *h_led_);

/********************** internal data definition *****************************/

/********************** external data declaration ****************************/
uint32_t g_task_led_cnt;

h_led_t h_led[LED_QTY] = {{&led[LED_A], &led_sc[LED_A]},
				    	  {&led[LED_B], &led_sc[LED_B]},
						  {&led[LED_C], &led_sc[LED_C]}};

/********************** external functions definition ************************/
/* Task thread */
void task_led(void *parameters)
{
	/*  Declare & Initialize Task Function variables */
	g_task_led_cnt = G_TASK_LED_CNT_INI;
	h_led_t *p_h_led = (h_led_t *)parameters;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", pcTaskGetName(NULL), xTaskGetTickCount());

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
    {
		/* Update Task Counter */
		g_task_led_cnt++;

		/* Get Events to excite Statechart */
		if (pdFAIL == xQueueReceive(h_led_task_q, (void *)&p_h_led->led_sc->ev_in, (TickType_t)ZERO))
		{
			p_h_led->led_sc->ev_in = EV_LED_NONE;
		}

		/* Run Statechart */
    	task_led_statechart(p_h_led);

    	/* We want this task to execute every 50 milliseconds. */
		vTaskDelay(TASK_LED_DEL_MAX);
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
					h_led->led->pin_state = LED_OFF;
					h_led_->led_sc->tick = ZERO;

					HAL_GPIO_WritePin(h_led->led->gpio_port, h_led->led->pin, h_led->led->pin_state);

					break;

				case EV_LED_ON:

					h_led_->led_sc->state = ST_LED_ON;
					h_led->led->pin_state = LED_ON;
					h_led_->led_sc->tick = ZERO;

					HAL_GPIO_WritePin(h_led->led->gpio_port, h_led->led->pin, h_led->led->pin_state);

					break;

				case EV_LED_BLINK:

					h_led_->led_sc->state = ST_LED_BLINK;
					h_led->led->pin_state = HAL_GPIO_ReadPin(h_led->led->gpio_port, h_led->led->pin);
					h_led_->led_sc->tick = DEL_LED_BLINK;

					HAL_GPIO_TogglePin(h_led->led->gpio_port, h_led->led->pin);

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
					h_led->led->pin_state = LED_OFF;
					h_led_->led_sc->tick = ZERO;

					HAL_GPIO_WritePin(h_led->led->gpio_port, h_led->led->pin, h_led->led->pin_state);

					break;

				case EV_LED_ON:

					h_led_->led_sc->state = ST_LED_ON;
					h_led->led->pin_state = LED_ON;
					h_led_->led_sc->tick = ZERO;

					HAL_GPIO_WritePin(h_led->led->gpio_port, h_led->led->pin, h_led->led->pin_state);

					break;

				case EV_LED_BLINK:
				case EV_LED_NONE:

					h_led_->led_sc->state = ST_LED_BLINK;
					h_led_->led_sc->tick -= DEL_LED_MIN;

					if (ZERO == h_led_->led_sc->tick)
					{
						h_led->led->pin_state = HAL_GPIO_ReadPin(h_led->led->gpio_port, h_led->led->pin);
						h_led_->led_sc->tick = DEL_LED_BLINK;

						HAL_GPIO_TogglePin(h_led->led->gpio_port, h_led->led->pin);
					}

					break;
			}

			break;
	}
}

/********************** end of file ******************************************/
