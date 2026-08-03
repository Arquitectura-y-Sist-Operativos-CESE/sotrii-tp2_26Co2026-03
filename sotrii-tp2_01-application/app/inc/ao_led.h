#ifndef AO_LED_H_
#define AO_LED_H_

#include "main.h"
#include "cmsis_os.h"

/* Enumeración de comandos */
typedef enum {
    AO_LED_CMD_OFF = 0,
    AO_LED_CMD_ON,
    AO_LED_CMD_TOGGLE
} ao_led_cmd_t;

/* Estrutura del Active Object según consigna */
typedef struct {
    uint8_t        ao_id;
    GPIO_TypeDef*  gpio_port;
    uint16_t       gpio_pin;
    QueueHandle_t  ao_queue;
    TaskHandle_t   task_handle;
} ao_led_t;

/* Funciones de Interfaz exigidas en Paso 06 */
BaseType_t open_led_ao(ao_led_t *p_ao, uint8_t id, GPIO_TypeDef *port, uint16_t pin, UBaseType_t queue_len, UBaseType_t priority);
BaseType_t release_led_ao(ao_led_t *p_ao);
BaseType_t send_led_ao(ao_led_t *p_ao, ao_led_cmd_t cmd);
BaseType_t ioctl_led_ao(ao_led_t *p_ao, ao_led_cmd_t cmd);

#endif /* AO_LED_H_ */