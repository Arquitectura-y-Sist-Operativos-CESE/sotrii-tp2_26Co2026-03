#include "ao_led.h"
#include "dwt.h"      /* Para medir ciclos de reloj / WCET */
#include "logger.h"

/* Tarea Gatekeeper (Privada) */
static void prv_gatekeeper_task(void *pvParameters)
{
    ao_led_t *p_ao = (ao_led_t *)pvParameters;
    ao_led_cmd_t cmd;

    for (;;)
    {
        /* Espera bloqueada hasta recibir un evento de la cola */
        if (xQueueReceive(p_ao->ao_queue, &cmd, portMAX_DELAY) == pdPASS)
        {
            switch (cmd)
            {
                case AO_LED_CMD_ON:
                    HAL_GPIO_WritePin(p_ao->gpio_port, p_ao->gpio_pin, GPIO_PIN_SET);
                    break;

                case AO_LED_CMD_OFF:
                    HAL_GPIO_WritePin(p_ao->gpio_port, p_ao->gpio_pin, GPIO_PIN_RESET);
                    break;

                case AO_LED_CMD_TOGGLE:
                    HAL_GPIO_TogglePin(p_ao->gpio_port, p_ao->gpio_pin);
                    break;

                default:
                    break;
            }
        }
    }
}

BaseType_t open_led_ao(ao_led_t *p_ao, uint8_t id, GPIO_TypeDef *port, uint16_t pin, UBaseType_t queue_len, UBaseType_t priority)
{
    if (p_ao == NULL) return pdFAIL;

    p_ao->ao_id = id;
    p_ao->gpio_port = port;
    p_ao->gpio_pin = pin;

    p_ao->ao_queue = xQueueCreate(queue_len, sizeof(ao_led_cmd_t));
    if (p_ao->ao_queue == NULL) return pdFAIL;

    BaseType_t status = xTaskCreate(prv_gatekeeper_task, "AO_LED_Task", configMINIMAL_STACK_SIZE, (void *)p_ao, priority, &p_ao->task_handle);

    return status;
}

BaseType_t ioctl_led_ao(ao_led_t *p_ao, ao_led_cmd_t cmd)
{
    /* Medición de WCET usando DWT */
    uint32_t start_cycles = cycle_counter_get();

    BaseType_t result = xQueueSend(p_ao->ao_queue, &cmd, 0);

    uint32_t elapsed_cycles = cycle_counter_get() - start_cycles;
    LOGGER_INFO("ioctl_led_ao WCET: %lu ciclos de reloj", elapsed_cycles);

    return result;
}
