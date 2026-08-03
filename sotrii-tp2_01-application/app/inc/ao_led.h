#ifndef AO_LED_H
#define AO_LED_H

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* -------------------------------------------------------------------------
 * Enumeraciones (Prefijo e)
 * ------------------------------------------------------------------------- */
typedef enum eAO_LED_ID
{
    eAO_LED_ID_1 = 0,
    eAO_LED_ID_2,
    eAO_LED_ID_3,
    eAO_LED_ID_MAX
} eAO_ID_t;

typedef enum eAO_LED_CMD
{
    eAO_LED_CMD_NONE = 0,
    eAO_LED_CMD_ON,
    eAO_LED_CMD_OFF,
    eAO_LED_CMD_TOGGLE,
    eAO_LED_CMD_SET_PERIOD
} eAO_LED_CMD_t;

/* -------------------------------------------------------------------------
 * Estructuras de Datos (Prefijo x)
 * ------------------------------------------------------------------------- */
typedef struct xAO_LED_MSG
{
    eAO_LED_CMD_t eCmd;            /* e: Enum */
    uint32_t ulParam;              /* ul: Unsigned Long / uint32_t */
    TaskHandle_t xRequester;       /* x: FreeRTOS Handle */
} xAO_LED_MSG_t;

typedef struct xAO_LED
{
    eAO_ID_t eAOId;                /* e: Enum */
    GPIO_TypeDef *pxPort;          /* px: Pointer to Struct */
    uint16_t usPin;                /* us: Unsigned Short */
    QueueHandle_t xAOQueue;        /* x: FreeRTOS Queue Handle */
    TaskHandle_t xTaskHandle;      /* x: FreeRTOS Task Handle */
    uint32_t ulBlinkPeriodMs;      /* ul: Unsigned Long */
    BaseType_t xIsOpen;            /* x: BaseType_t / Boolean */
} xAO_LED_t;

/* -------------------------------------------------------------------------
 * Prototipos de la API Pública (Prefijo x para retorno BaseType_t)
 * ------------------------------------------------------------------------- */
BaseType_t xAOLEDOpen( xAO_LED_t * const pxAO,
                       const eAO_ID_t eId,
                       GPIO_TypeDef * const pxPort,
                       const uint16_t usPin,
                       const UBaseType_t uxQueueLength,
                       const UBaseType_t uxTaskPriority );

BaseType_t xAOLEDRelease( xAO_LED_t * const pxAO );

BaseType_t xAOLEDSend( xAO_LED_t * const pxAO,
                       const xAO_LED_MSG_t * const pxMsg,
                       const TickType_t xTicksToWait );

BaseType_t xAOLEDIoctl( xAO_LED_t * const pxAO,
                        const eAO_LED_CMD_t eCmd,
                        const uint32_t ulParam );

#endif /* AO_LED_H */