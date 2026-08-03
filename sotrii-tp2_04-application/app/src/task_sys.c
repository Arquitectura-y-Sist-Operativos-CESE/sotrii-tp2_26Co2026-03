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
#include "task_led.h"

/********************** macros and definitions *******************************/
#define G_TASK_SYS_CNT_INI	0ul

#define DEL_SYS_MIN			(pdMS_TO_TICKS(50ul))
#define DEL_SYS_BLINK		(pdMS_TO_TICKS(500ul))

#define TASK_SYS_DEL_ZERO	(pdMS_TO_TICKS(0ul))
#define TASK_SYS_DEL_MAX	DEL_SYS_MIN
#define BTN_LONG_TIME		(pdMS_TO_TICKS(2000ul))
#define OP_TIME_A			(pdMS_TO_TICKS(5000ul))
#define OP_TIME_B			(pdMS_TO_TICKS(10000ul))

/********************** internal data declaration ****************************/
sys_sc_t sys_sc = {ST_SYS_IDLE, EV_SYS_OFF, ZERO, EV_SYS_NONE, ZERO};

/********************** internal functions declaration ***********************/
void task_sys_statechart(h_sys_t *h_sys_);

/********************** internal data definition *****************************/

/********************** external data declaration ****************************/
uint32_t g_task_sys_cnt;

h_sys_t h_sys = {.sys_sc=&sys_sc,.ao_id=SYS_AO_0,
	.operation_time={OP_TIME_A,OP_TIME_B},.active_button=BTN_A};

volatile uint32_t g_open_sys_ao_wcet_cycles,g_release_sys_ao_wcet_cycles;
volatile uint32_t g_send_sys_ao_wcet_cycles,g_ioctl_sys_ao_wcet_cycles;
static void update_wcet(volatile uint32_t *w,uint32_t s)
{uint32_t e=cycle_counter_get()-s;if(e>*w)*w=e;}

sys_ao_status_t open_sys_ao(h_sys_t *ao)
{
	uint32_t s=cycle_counter_get();sys_ao_status_t r=SYS_AO_INVALID_ARG;
	if((NULL!=ao)&&(ao->ao_id<SYS_AO_QTY)){r=SYS_AO_ERROR;if(pdTRUE==ao->is_open)r=SYS_AO_OK;
	else if(NULL!=ao->ao_queue){ao->is_open=pdTRUE;r=SYS_AO_OK;}}
	update_wcet(&g_open_sys_ao_wcet_cycles,s);return r;
}
sys_ao_status_t release_sys_ao(h_sys_t *ao)
{
	uint32_t s=cycle_counter_get();sys_ao_status_t r=SYS_AO_INVALID_ARG;
	if(NULL!=ao){r=SYS_AO_NOT_OPEN;if(pdTRUE==ao->is_open){if(NULL!=ao->ao_task)vTaskDelete(ao->ao_task);
	ao->ao_task=NULL;ao->ao_queue=NULL;ao->is_open=pdFALSE;r=SYS_AO_OK;}}
	update_wcet(&g_release_sys_ao_wcet_cycles,s);return r;
}
sys_ao_status_t send_sys_ao(h_sys_t *ao,btn_id_t id,btn_ev_t ev,TickType_t time,TickType_t timeout)
{
	uint32_t s=cycle_counter_get();sys_ao_status_t r=SYS_AO_INVALID_ARG;
	btn_ao_msg_t m={.button_id=id,.event=ev,.time=time,.requester=xTaskGetCurrentTaskHandle()};
	if((NULL!=ao)&&(id<BTN_QTY)){r=SYS_AO_NOT_OPEN;if(pdTRUE==ao->is_open){r=SYS_AO_TIMEOUT;
	if(pdPASS==xQueueSend(ao->ao_queue,&m,timeout))r=(0ul<ulTaskNotifyTake(pdTRUE,timeout))?SYS_AO_OK:SYS_AO_TIMEOUT;}}
	update_wcet(&g_send_sys_ao_wcet_cycles,s);return r;
}
sys_ao_status_t ioctl_sys_ao(h_sys_t *ao,sys_ao_ioctl_cmd_t cmd,void *arg)
{
	uint32_t s=cycle_counter_get();sys_ao_status_t r=SYS_AO_INVALID_ARG;
	if((NULL!=ao)&&(NULL!=arg)&&(pdTRUE==ao->is_open)){taskENTER_CRITICAL();
	if(SYS_AO_IOCTL_GET_STATE==cmd){*(sys_st_t*)arg=ao->sys_sc->state;r=SYS_AO_OK;}
	else if(SYS_AO_IOCTL_GET_TIME_A==cmd){*(TickType_t*)arg=ao->operation_time[BTN_A];r=SYS_AO_OK;}
	else if(SYS_AO_IOCTL_GET_TIME_B==cmd){*(TickType_t*)arg=ao->operation_time[BTN_B];r=SYS_AO_OK;}
	taskEXIT_CRITICAL();}else if((NULL!=ao)&&(pdFALSE==ao->is_open))r=SYS_AO_NOT_OPEN;
	update_wcet(&g_ioctl_sys_ao_wcet_cycles,s);return r;
}

static void set_normal_leds(void)
{
	(void)send_led_ao(&h_led[LED_A],EV_LED_ON,portMAX_DELAY);
	(void)send_led_ao(&h_led[LED_B],EV_LED_ON,portMAX_DELAY);
	(void)send_led_ao(&h_led[LED_C],EV_LED_OFF,portMAX_DELAY);
}

static void start_operation(h_sys_t *ao,btn_id_t id)
{
	ao->active_button=id;ao->operation_remaining=ao->operation_time[id];
	ao->sys_sc->state=(BTN_A==id)?ST_SYS_ACTIVE_0:ST_SYS_ACTIVE_1;
	(void)send_led_ao(&h_led[LED_A],(BTN_A==id)?EV_LED_BLINK:EV_LED_ON,portMAX_DELAY);
	(void)send_led_ao(&h_led[LED_B],(BTN_B==id)?EV_LED_BLINK:EV_LED_ON,portMAX_DELAY);
	(void)send_led_ao(&h_led[LED_C],EV_LED_ON,portMAX_DELAY);
	LOGGER_INFO("SYS start BTN%c operation=%lu ms",(char)('A'+id),
			(unsigned long)ao->operation_remaining);
}

/********************** external functions definition ************************/
/* Task thread */
void task_sys(void *parameters)
{
	/*  Declare & Initialize Task Function variables */
	g_task_sys_cnt = G_TASK_SYS_CNT_INI;
	h_sys_t *p_h_sys = (h_sys_t *)parameters;
	btn_ao_msg_t message;

	/* Print out: Task Initialized */
	LOGGER_INFO(" ");
	LOGGER_INFO("  %s is running - Tick [mS] = %lu", pcTaskGetName(NULL), xTaskGetTickCount());
	set_normal_leds();

	/* As per most tasks, this task is implemented in an infinite loop. */
	for (;;)
    {
		/* Update Task Counter */
		g_task_sys_cnt++;

		/* Get Events to excite Statechart */
		if (pdPASS == xQueueReceive(p_h_sys->ao_queue,&message,(TickType_t)ZERO))
		{
			LOGGER_INFO("SYS recv BTN%c ev=%u time=%lu",(char)('A'+message.button_id),
					(unsigned)message.event,(unsigned long)message.time);
			if(EV_BTN_UP==message.event)
			{
				if(message.time>=BTN_LONG_TIME)
				{
					p_h_sys->operation_time[message.button_id]=message.time;
					LOGGER_INFO("SYS set T_%c=%lu ms",(BTN_A==message.button_id)?'A':'B',
							(unsigned long)message.time);
				}
				else start_operation(p_h_sys,message.button_id);
			}
		}
		else
		{
			message.requester=NULL;
		}

		if((ST_SYS_IDLE!=p_h_sys->sys_sc->state)&&(p_h_sys->operation_remaining>ZERO))
		{
			p_h_sys->operation_remaining=(p_h_sys->operation_remaining>TASK_SYS_DEL_MAX)?
					p_h_sys->operation_remaining-TASK_SYS_DEL_MAX:ZERO;
			if(ZERO==p_h_sys->operation_remaining)
			{
				set_normal_leds();p_h_sys->sys_sc->state=ST_SYS_IDLE;
				LOGGER_INFO("SYS operation finished");
			}
		}
		if(NULL!=message.requester)xTaskNotifyGive(message.requester);

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
