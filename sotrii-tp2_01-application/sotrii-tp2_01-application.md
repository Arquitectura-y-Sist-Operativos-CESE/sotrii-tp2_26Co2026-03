## 4. Descripción Detallada de las Tareas (`.c`)

### 4.1. Tarea de Botones (`task_btn.c`)
- **Frecuencia de Ejecución:** Bucle infinito ejecutado periódicamente cada 50 ms (`vTaskDelay(pdMS_TO_TICKS(50))`).
- **Funcionamiento:**
  1. Lee el estado físico del GPIO mediante `HAL_GPIO_ReadPin()`.
  2. Determina el evento de entrada (`EV_BTN_DOWN` si está presionado, `EV_BTN_UP` si no).
  3. Ejecuta la función `task_btn_statechart()`.
- **Lógica de Statechart (`task_btn_statechart`):**
  - Si está en `ST_BTN_UP` y recibe `EV_BTN_DOWN`: Transiciona a `ST_BTN_DOWN` y envía `EV_BTN_DOWN` a la cola `h_sys_task_q` utilizando `xQueueSend()`.
  - Si está en `ST_BTN_DOWN` y recibe `EV_BTN_UP`: Transiciona a `ST_BTN_UP` y envía `EV_BTN_UP` a la cola `h_sys_task_q`.

### 4.2. Tarea del Sistema (`task_sys.c`)
- **Frecuencia de Ejecución:** Periodo de 50 ms (`vTaskDelay`).
- **Funcionamiento:**
  1. Consulta la cola `h_sys_task_q` mediante `xQueueReceive()` con tiempo de espera cero (`TickType_t ZERO`).
  2. Si no hay elementos en la cola, asume el evento `EV_SYS_NONE`.
  3. Ejecuta `task_sys_statechart()`.
- **Lógica de Statechart (`task_sys_statechart`):**
  - **`ST_SYS_IDLE`:** Al recibir `EV_SYS_ON`, conmuta al estado `ST_SYS_ACTIVE_0` y envía el evento `EV_SYS_ON` a la cola `h_led_task_q`.
  - **`ST_SYS_ACTIVE_0`:** Al recibir nuevamente `EV_SYS_ON`, conmuta al estado `ST_SYS_ACTIVE_1` y envía el evento `EV_SYS_BLINK` a la cola `h_led_task_q`.
  - **`ST_SYS_ACTIVE_1`:** Al recibir por tercera vez `EV_SYS_ON`, regresa al estado `ST_SYS_IDLE` y envía el evento `EV_SYS_OFF` a la cola `h_led_task_q`.

### 4.3. Tarea de LEDs (`task_led.c`)
- **Frecuencia de Ejecución:** Periodo de 50 ms (`vTaskDelay`).
- **Funcionamiento:**
  1. Consulta la cola `h_led_task_q` mediante `xQueueReceive()`. Si no hay nuevo evento, asigna `EV_LED_NONE`.
  2. Ejecuta `task_led_statechart()`.
- **Lógica de Statechart (`task_led_statechart`):**
  - **Manejo de `EV_LED_OFF`:** Configura el estado a `ST_LED_OFF`, fuerza el pin a `LED_OFF` con `HAL_GPIO_WritePin()` y reinicia el contador de ticks.
  - **Manejo de `EV_LED_ON`:** Configura el estado a `ST_LED_ON`, fuerza el pin a `LED_ON` con `HAL_GPIO_WritePin()` y reinicia el contador de ticks.
  - **Manejo de `EV_LED_BLINK`:**
    - Ingresa o se mantiene en el estado `ST_LED_BLINK`.
    - Realiza una temporización no bloqueante basada en el periodo de la tarea (50 ms).
    - Cada vez que transcurren 500 ms (`DEL_LED_BLINK`), conmuta el estado del pin mediante `HAL_GPIO_TogglePin()` y reinicia la cuenta.

---

## 5. Resumen de Transiciones y Ciclo de Vida del Sistema

| Acción del Usuario | Estado del Sistema (`task_sys`) | Evento Enviado al LED (`h_led_task_q`) | Estado / Acción del LED (`task_led`) |
| :--- | :--- | :--- | :--- |
| **Estado Inicial** | `ST_SYS_IDLE` | `EV_SYS_OFF` | `ST_LED_OFF` (LED Apagado) |
| **1ª Presión del Botón** | Transiciona a `ST_SYS_ACTIVE_0` | `EV_SYS_ON` | Transiciona a `ST_LED_ON` (Encendido Fijo) |
| **2ª Presión del Botón** | Transiciona a `ST_SYS_ACTIVE_1` | `EV_SYS_BLINK` | Transiciona a `ST_LED_BLINK` (Parpadeo a 500 ms) |
| **3ª Presión del Botón** | Regresa a `ST_SYS_IDLE` | `EV_SYS_OFF` | Regresa a `ST_LED_OFF` (LED Apagado) |