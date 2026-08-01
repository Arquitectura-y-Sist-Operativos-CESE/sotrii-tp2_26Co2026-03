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

#ifndef TASK_LED_ATTRIBUTE_H_
#define TASK_LED_ATTRIBUTE_H_

/********************** CPP guard ********************************************/
#ifdef __cplusplus
extern "C" {
#endif

/********************** inclusions *******************************************/
#include "task_sys_attribute.h"

/********************** macros ***********************************************/
/* ID of Leds */
typedef enum led_id {LED_A,
					 LED_B,
					 LED_C,
					 LED_QTY} led_id_t;

/* Events of Statechart */
typedef enum led_ev {EV_LED_OFF = EV_SYS_OFF,
					 EV_LED_ON = EV_SYS_ON,
					 EV_LED_BLINK = EV_SYS_BLINK,
					 EV_LED_NONE} led_ev_t;

/* States of Statechart */
typedef enum led_st {ST_LED_OFF,
					 ST_LED_ON,
					 ST_LED_BLINK} led_st_t;

/********************** typedef **********************************************/
/* Structure of Leds */
typedef struct
{
	led_id_t		id;
	GPIO_TypeDef *	gpio_port;
	uint16_t		pin;
	GPIO_PinState 	pin_state;
} led_t;

/* Structure of Statechart */
typedef struct
{
	led_st_t		state;
	led_ev_t		ev_in;
	TickType_t		tick;
} led_sc_t;

/* Structure of Task */
typedef struct
{
	led_t *		led;
	led_sc_t *	led_sc;
} h_led_t;

/********************** external data declaration ****************************/

/********************** external functions declaration ***********************/

/********************** End of CPP guard *************************************/
#ifdef __cplusplus
}
#endif

#endif /* TASK_LED_ATTRIBUTE_H_ */

/********************** end of file ******************************************/
