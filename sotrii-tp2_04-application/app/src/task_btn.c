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
#include "task_btn_attribute.h"

/********************** macros and definitions *******************************/
#define G_TASK_BTN_CNT_INI	0ul

#define DEL_BTN_MIN			(pdMS_TO_TICKS(50ul))

#define TASK_BTN_DEL_ZERO	(pdMS_TO_TICKS(0ul))
#define TASK_BTN_DEL_MAX	DEL_BTN_MIN

/********************** internal data declaration ****************************/
btn_t btn[BTN_QTY] = {{BTN_A, BTN_A_PORT, BTN_A_PIN, BTN_A_HOVER},
					  {BTN_B, BTN_B_PORT, BTN_B_PIN, BTN_B_HOVER}};

btn_sc_t btn_sc[BTN_QTY] = {{ST_BTN_UP, EV_BTN_UP, ZERO, EV_BTN_UP, ZERO},
							{ST_BTN_UP, EV_BTN_UP, ZERO, EV_BTN_UP, ZERO}};

/********************** internal functions declaration ***********************/
void task_btn_statechart(h_btn_t *h_btn_);
void task_btn(void *parameters);

/********************** internal data definition *****************************/

/********************** external data declaration ****************************/
uint32_t g_task_btn_cnt;

h_btn_t h_btn[BTN_QTY] = {
	{.btn=&btn[BTN_A], .btn_sc=&btn_sc[BTN_A], .ao_id=BTN_A},
	{.btn=&btn[BTN_B], .btn_sc=&btn_sc[BTN_B], .ao_id=BTN_B}};

volatile uint32_t g_open_btn_ao_wcet_cycles, g_release_btn_ao_wcet_cycles;
volatile uint32_t g_send_btn_ao_wcet_cycles, g_ioctl_btn_ao_wcet_cycles;

static void update_wcet(volatile uint32_t *wcet, uint32_t start)
{
	uint32_t elapsed = cycle_counter_get() - start;
	if (elapsed > *wcet) *wcet = elapsed;
}

btn_ao_status_t open_btn_ao(h_btn_t *ao)
{
	uint32_t start=cycle_counter_get(); btn_ao_status_t status=BTN_AO_INVALID_ARG;
	if ((NULL != ao) && (ao->ao_id < BTN_QTY)) {
		status=BTN_AO_ERROR;
		if (pdTRUE == ao->is_open) status=BTN_AO_OK;
		else if (NULL != ao->ao_queue) {ao->is_open=pdTRUE; status=BTN_AO_OK;}
	}
	update_wcet(&g_open_btn_ao_wcet_cycles,start); return status;
}

btn_ao_status_t release_btn_ao(h_btn_t *ao)
{
	uint32_t start=cycle_counter_get(); btn_ao_status_t status=BTN_AO_INVALID_ARG;
	if (NULL != ao) {
		status=BTN_AO_NOT_OPEN;
		if (pdTRUE == ao->is_open) {
			if (NULL != ao->ao_task) vTaskDelete(ao->ao_task);
			ao->ao_task=NULL; ao->ao_queue=NULL; ao->is_open=pdFALSE; status=BTN_AO_OK;
		}
	}
	update_wcet(&g_release_btn_ao_wcet_cycles,start); return status;
}

btn_ao_status_t send_btn_ao(h_btn_t *ao, btn_ev_t event,
		TickType_t time, TickType_t timeout)
{
	uint32_t start=cycle_counter_get(); btn_ao_status_t status=BTN_AO_INVALID_ARG;
	btn_ao_msg_t msg={.button_id=ao ? ao->ao_id : BTN_QTY, .event=event,
		.time=time, .requester=xTaskGetCurrentTaskHandle()};
	if ((NULL != ao) && (event <= EV_BTN_DOWN)) {
		status=BTN_AO_NOT_OPEN;
		if (pdTRUE == ao->is_open) {
			status=BTN_AO_TIMEOUT;
			if (pdPASS == xQueueSend(ao->ao_queue,&msg,timeout))
				status=(0ul < ulTaskNotifyTake(pdTRUE,timeout)) ? BTN_AO_OK:BTN_AO_TIMEOUT;
		}
	}
	update_wcet(&g_send_btn_ao_wcet_cycles,start); return status;
}

btn_ao_status_t ioctl_btn_ao(h_btn_t *ao, btn_ao_ioctl_cmd_t cmd, void *arg)
{
	uint32_t start=cycle_counter_get(); btn_ao_status_t status=BTN_AO_INVALID_ARG;
	if ((NULL != ao)&&(NULL != arg)&&(pdTRUE == ao->is_open)) {
		taskENTER_CRITICAL();
		if (BTN_AO_IOCTL_GET_STATE == cmd) {*(btn_st_t*)arg=ao->btn_sc->state; status=BTN_AO_OK;}
		else if (BTN_AO_IOCTL_GET_PIN_STATE == cmd) {*(GPIO_PinState*)arg=ao->btn->pin_state; status=BTN_AO_OK;}
		else if (BTN_AO_IOCTL_GET_ELAPSED_TIME == cmd) {*(TickType_t*)arg=ao->btn_sc->tick; status=BTN_AO_OK;}
		taskEXIT_CRITICAL();
	} else if ((NULL != ao)&&(pdFALSE == ao->is_open)) status=BTN_AO_NOT_OPEN;
	update_wcet(&g_ioctl_btn_ao_wcet_cycles,start); return status;
}

/********************** external functions definition ************************/
/* Task thread */
void task_btn(void *parameters)
{
	/*  Declare & Initialize Task Function variables */
	g_task_btn_cnt = G_TASK_BTN_CNT_INI;
	h_btn_t *p_h_btn = (h_btn_t *)parameters;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", pcTaskGetName(NULL), xTaskGetTickCount());

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
    {
		/* Update Task Counter */
		g_task_btn_cnt++;

		/* Get Events to excite Statechart */
		p_h_btn->btn->pin_state = HAL_GPIO_ReadPin(p_h_btn->btn->gpio_port, p_h_btn->btn->pin);
		GPIO_PinState pressed = (BTN_A == p_h_btn->ao_id) ? BTN_A_PRESSED : BTN_B_PRESSED;
		if (pressed == p_h_btn->btn->pin_state)
		{
			p_h_btn->btn_sc->ev_in = EV_BTN_DOWN;
		}
		else
		{
			p_h_btn->btn_sc->ev_in = EV_BTN_UP;
		}

		/* Run Statechart */
    	task_btn_statechart(p_h_btn);

    	/* We want this task to execute every 50 milliseconds. */
		vTaskDelay(TASK_BTN_DEL_MAX);
    }
}

void task_btn_statechart(h_btn_t *h_btn_)
{
	/* Run to Completion Statechart */
	switch (h_btn_->btn_sc->state)
	{
		case ST_BTN_UP:

			if (EV_BTN_DOWN == h_btn_->btn_sc->ev_in)
			{
				h_btn_->btn_sc->state = ST_BTN_DOWN;
				h_btn_->btn_sc->ev_out = EV_BTN_DOWN;
				h_btn_->btn_sc->tick_out = h_btn_->btn_sc->tick;
				h_btn_->btn_sc->tick = ZERO;

				LOGGER_INFO("BTN%u->SYS ev=%u time=%lu", (unsigned)h_btn_->ao_id,
						(unsigned)h_btn_->btn_sc->ev_out, (unsigned long)h_btn_->btn_sc->tick_out);
				(void)send_btn_ao(h_btn_,h_btn_->btn_sc->ev_out,h_btn_->btn_sc->tick_out,portMAX_DELAY);
			}
			else
			{
				h_btn_->btn_sc->tick += DEL_BTN_MIN;
			}

			break;

		case ST_BTN_DOWN:

			if (EV_BTN_UP == h_btn_->btn_sc->ev_in)
			{
				h_btn_->btn_sc->state = ST_BTN_UP;
				h_btn_->btn_sc->ev_out = EV_BTN_UP;
				h_btn_->btn_sc->tick_out = h_btn_->btn_sc->tick;
				h_btn_->btn_sc->tick = ZERO;

				LOGGER_INFO("BTN%u->SYS ev=%u time=%lu", (unsigned)h_btn_->ao_id,
						(unsigned)h_btn_->btn_sc->ev_out, (unsigned long)h_btn_->btn_sc->tick_out);
				(void)send_btn_ao(h_btn_,h_btn_->btn_sc->ev_out,h_btn_->btn_sc->tick_out,portMAX_DELAY);
			}
			else
			{
				h_btn_->btn_sc->tick += DEL_BTN_MIN;
			}

			break;
	}
}

/********************** end of file ******************************************/
