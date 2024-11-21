#include "wiced_bt_dev.h"
#include <FreeRTOS.h>
#include <task.h>
#include "timers.h"
#include <queue.h>
/*******************************************************************************
*        Macro Definitions
*******************************************************************************/
#define STRING_BUFFER_SIZE              (80u)
#define DAYS_PER_WEEK                   (7u)

/* Structure tm stores years since 1900 */
#define TM_YEAR_BASE                    (1900u)

/* Macros for button interrupt and button task */
/* Interrupt priority for the GPIO connected to the user button */
#define BUTTON_INTERRUPT_PRIORITY       (7u)
#define BUTTON_TASK_PRIORITY            (configMAX_PRIORITIES - 1)
#define BUTTON_TASK_STACK_SIZE          (configMINIMAL_STACK_SIZE * 2)

/*******************************************************************************
 * Extern variables
 ******************************************************************************/
extern TaskHandle_t  button_task_handle;

extern QueueHandle_t JoystickDataQueue;

typedef struct{
	u_int8_t x;
	u_int8_t y;
}JoystickData;

/*******************************************************************************
*        Function Prototypes
*******************************************************************************/

/*Button interrupt handler*/
void button_interrupt_handler(void *handler_arg, cyhal_gpio_event_t event);
void button_task(void *pvParameters);

/* Callback function for Bluetooth stack management events */
wiced_bt_dev_status_t app_bt_management_callback (wiced_bt_management_evt_t event,
                                                  wiced_bt_management_evt_data_t *p_event_data);
/* [] END OF FILE */
