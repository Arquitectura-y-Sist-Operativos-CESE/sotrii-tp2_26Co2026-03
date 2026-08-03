#include "ao_led.h"
#include <stdio.h>

/* Prototipo de la tarea estática privada (prv = Private) */
static void prvAOLEDGatekeeperTask( void *pvParameters );

/*-----------------------------------------------------------*/

BaseType_t xAOLEDOpen( xAO_LED_t * const pxAO,
                       const eAO_ID_t eId,
                       GPIO_TypeDef * const pxPort,
                       const uint16_t usPin,
                       const UBaseType_t uxQueueLength,
                       const UBaseType_t uxTaskPriority )
{
    /* Declaración de variables al inicio del bloque (Estándar C89) */
    BaseType_t xReturn = pdFAIL;
    BaseType_t xStatus;
    GPIO_InitTypeDef xGpioInitStruct;
    char pcTaskName[ 16 ];

    if( ( pxAO != NULL ) && ( pxPort != NULL ) )
    {
        pxAO->eAOId = eId;
        pxAO->pxPort = pxPort;
        pxAO->usPin = usPin;
        pxAO->ulBlinkPeriodMs = 500U;

        /* 1. Crear Cola de Mensajes FreeRTOS */
        pxAO->xAOQueue = xQueueCreate( uxQueueLength, sizeof( xAO_LED_MSG_t ) );

        if( pxAO->xAOQueue != NULL )
        {
            /* 2. Inicializar Hardware GPIO (STM32 HAL) */
            xGpioInitStruct.Pin = ( uint32_t ) usPin;
            xGpioInitStruct.Mode = GPIO_MODE_OUTPUT_PP;
            xGpioInitStruct.Pull = GPIO_NOPULL;
            xGpioInitStruct.Speed = GPIO_SPEED_FREQ_LOW;

            HAL_GPIO_Init( pxPort, &xGpioInitStruct );
            HAL_GPIO_WritePin( pxPort, usPin, GPIO_PIN_RESET );

            /* 3. Crear Tarea Gatekeeper */
            ( void ) snprintf( pcTaskName, sizeof( pcTaskName ), "AO_LED_%d", ( int ) eId );

            xStatus = xTaskCreate( prvAOLEDGatekeeperTask,
                                   pcTaskName,
                                   configMINIMAL_STACK_SIZE,
                                   ( void * ) pxAO,
                                   uxTaskPriority,
                                   &( pxAO->xTaskHandle ) );

            if( xStatus == pdPASS )
            {
                pxAO->xIsOpen = pdTRUE;
                xReturn = pdPASS;
            }
            else
            {
                vQueueDelete( pxAO->xAOQueue );
                pxAO->xAOQueue = NULL;
            }
        }
    }

    return xReturn;
}

/*-----------------------------------------------------------*/

BaseType_t xAOLEDRelease( xAO_LED_t * const pxAO )
{
    BaseType_t xReturn = pdFAIL;

    if( ( pxAO != NULL ) && ( pxAO->xIsOpen == pdTRUE ) )
    {
        HAL_GPIO_WritePin( pxAO->pxPort, pxAO->usPin, GPIO_PIN_RESET );

        if( pxAO->xTaskHandle != NULL )
        {
            vTaskDelete( pxAO->xTaskHandle );
            pxAO->xTaskHandle = NULL;
        }

        if( pxAO->xAOQueue != NULL )
        {
            vQueueDelete( pxAO->xAOQueue );
            pxAO->xAOQueue = NULL;
        }

        pxAO->xIsOpen = pdFALSE;
        xReturn = pdPASS;
    }

    return xReturn;
}

/*-----------------------------------------------------------*/

BaseType_t xAOLEDSend( xAO_LED_t * const pxAO,
                       const xAO_LED_MSG_t * const pxMsg,
                       const TickType_t xTicksToWait )
{
    BaseType_t xReturn = pdFAIL;

    if( ( pxAO != NULL ) && ( pxAO->xIsOpen == pdTRUE ) && ( pxMsg != NULL ) )
    {
        xReturn = xQueueSend( pxAO->xAOQueue, ( const void * ) pxMsg, xTicksToWait );
    }

    return xReturn;
}

/*-----------------------------------------------------------*/

BaseType_t xAOLEDIoctl( xAO_LED_t * const pxAO,
                        const eAO_LED_CMD_t eCmd,
                        const uint32_t ulParam )
{
    BaseType_t xReturn = pdFAIL;
    xAO_LED_MSG_t xMsg;

    if( ( pxAO != NULL ) && ( pxAO->xIsOpen == pdTRUE ) )
    {
        xMsg.eCmd = eCmd;
        xMsg.ulParam = ulParam;
        xMsg.xRequester = xTaskGetCurrentTaskHandle();

        xReturn = xAOLEDSend( pxAO, &xMsg, pdMS_TO_TICKS( 10U ) );
    }

    return xReturn;
}

/*-----------------------------------------------------------*/

static void prvAOLEDGatekeeperTask( void *pvParameters )
{
    xAO_LED_t * const pxAO = ( xAO_LED_t * ) pvParameters;
    xAO_LED_MSG_t xMsg;

    for( ;; )
    {
        if( xQueueReceive( pxAO->xAOQueue, &xMsg, portMAX_DELAY ) == pdTRUE )
        {
            switch( xMsg.eCmd )
            {
                case eAO_LED_CMD_ON:
                    HAL_GPIO_WritePin( pxAO->pxPort, pxAO->usPin, GPIO_PIN_SET );
                    break;

                case eAO_LED_CMD_OFF:
                    HAL_GPIO_WritePin( pxAO->pxPort, pxAO->usPin, GPIO_PIN_RESET );
                    break;

                case eAO_LED_CMD_TOGGLE:
                    HAL_GPIO_TogglePin( pxAO->pxPort, pxAO->usPin );
                    break;

                case eAO_LED_CMD_SET_PERIOD:
                    if( xMsg.ulParam > 0U )
                    {
                        pxAO->ulBlinkPeriodMs = xMsg.ulParam;
                    }
                    break;

                default:
                    break;
            }
        }
    }
}