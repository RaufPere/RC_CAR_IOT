#include "wiced_bt_stack.h"
#include "cybsp.h"
#include "cyhal.h"
#include "cy_retarget_io.h"
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include "cycfg_gatt_db.h"
#include "cycfg_bt_settings.h"
#include "cycfg_gap.h"
#include "wiced_bt_dev.h"
#include "app_bt_utils.h"
#include "cts_client.h"
#include <stdlib.h>
#include "wiced_bt_uuid.h"
#include "wiced_bt_types.h"
#include "motor.h"

static uint16_t                    bt_connection_id = 0;
static cts_discovery_data_t        cts_discovery_data;
static bool                        notify_val = false;
static bool                        button_press_for_adv = true;

static void ble_app_init(void);
static void handle_received_data_from_controller(wiced_bt_gatt_data_t notif_data);
static void button_interrupt_handler(void *handler_arg, cyhal_gpio_event_t event);
static wiced_bt_gatt_status_t ble_app_write_notification_cccd(bool notify);

/* GATT Event Callback Functions */
static wiced_bt_gatt_status_t ble_app_connect_handler(wiced_bt_gatt_connection_status_t *p_conn_status);
static wiced_bt_gatt_status_t ble_app_gatt_event_callback(wiced_bt_gatt_evt_t  event,
                                                          wiced_bt_gatt_event_data_t *p_event_data);
static wiced_bt_gatt_status_t  ble_app_service_discovery_handler(wiced_bt_gatt_discovery_complete_t *discovery_complete);
static wiced_bt_gatt_status_t  ble_app_discovery_result_handler(wiced_bt_gatt_discovery_result_t *discovery_result);

/* Configure GPIO interrupt. */
cyhal_gpio_callback_data_t button_cb_data =
{
.callback = button_interrupt_handler,
.callback_arg = NULL
};

wiced_result_t app_bt_management_callback(wiced_bt_management_evt_t event,
                                          wiced_bt_management_evt_data_t *p_event_data)
{
    wiced_result_t wiced_result = WICED_BT_SUCCESS;
    wiced_bt_device_address_t bda = { 0 };
    wiced_bt_ble_advert_mode_t *p_adv_mode = NULL;

    switch (event)
    {
        case BTM_ENABLED_EVT:
            /* Bluetooth Controller and Host Stack Enabled */

            if (WICED_BT_SUCCESS == p_event_data->enabled.status)
            {
                wiced_bt_dev_read_local_addr(bda);
                printf("Local Bluetooth Address: ");
                print_bd_address(bda);

                /* Perform application-specific initialization */
                ble_app_init();
            }
            else
            {
                printf( "Bluetooth Disabled \n" );
            }

            break;

        case BTM_BLE_ADVERT_STATE_CHANGED_EVT:

            /* Advertisement State Changed */
            p_adv_mode = &p_event_data->ble_advert_state_changed;
            printf("Advertisement State Change: %s\n",
                   get_bt_advert_mode_name(*p_adv_mode));

            if (BTM_BLE_ADVERT_OFF == *p_adv_mode)
            {
                /* Advertisement Stopped */
                printf("Advertisement stopped\n");
            }
            else
            {
                /* Advertisement Started */
                printf("Advertisement started\n");
            }
            break;

        case BTM_BLE_CONNECTION_PARAM_UPDATE:
            printf("Connection parameter update status:%d, Connection Interval: %d,"
                   " Connection Latency: %d, Connection Timeout: %d\n",
                   p_event_data->ble_connection_param_update.status,
                   p_event_data->ble_connection_param_update.conn_interval,
                   p_event_data->ble_connection_param_update.conn_latency,
                   p_event_data->ble_connection_param_update.supervision_timeout);
            break;
    }

    return wiced_result;
}

static void ble_app_init(void)
{


    cy_rslt_t cy_result = CY_RSLT_SUCCESS;
    wiced_bt_gatt_status_t gatt_status = WICED_BT_GATT_SUCCESS;

    printf("\n***********************************************\n");
    printf("**Discover device with \"CTS Client\" name*\n");
    printf("***********************************************\n\n");

    /* Initialize GPIO for button interrupt*/
    cy_result = cyhal_gpio_init(CYBSP_USER_BTN, CYHAL_GPIO_DIR_INPUT,
                                CYHAL_GPIO_DRIVE_PULLUP, CYBSP_BTN_OFF);
    /* GPIO init failed. Stop program execution */
    if (CY_RSLT_SUCCESS !=  cy_result)
    {
        printf("Button GPIO init failed! \n");
        CY_ASSERT(0);
    }

    /* Configure GPIO interrupt. */
    cyhal_gpio_register_callback(CYBSP_USER_BTN,&button_cb_data);
    cyhal_gpio_enable_event(CYBSP_USER_BTN, CYHAL_GPIO_IRQ_FALL,
                            BUTTON_INTERRUPT_PRIORITY, true);

    /* Set Advertisement Data */
    wiced_bt_ble_set_raw_advertisement_data(CY_BT_ADV_PACKET_DATA_SIZE,
                                            cy_bt_adv_packet_data);

    /* Register with BT stack to receive GATT callback */
    gatt_status = wiced_bt_gatt_register(ble_app_gatt_event_callback);
    printf("GATT event Handler registration status: %s \n",
            get_bt_gatt_status_name(gatt_status));

    /* Initialize GATT Database */
    gatt_status = wiced_bt_gatt_db_init(gatt_database, gatt_database_len, NULL);
    printf("GATT database initialization status: %s \n",
            get_bt_gatt_status_name(gatt_status));
    BaseType_t xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;
	vTaskNotifyGiveFromISR(bluetooth_task_handle, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

void button_interrupt_handler(void *handler_arg, cyhal_gpio_event_t event)
{

    BaseType_t xHigherPriorityTaskWoken;
    xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(bluetooth_task_handle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

void bluetooth_task(void *pvParameters)
{
	ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

	/* Configure platform specific settings for the BT device */
	      cybt_platform_config_init(&cybsp_bt_platform_cfg);

	      /* Register call back and configuration with stack */
	      wiced_result_t wiced_result = wiced_bt_stack_init(app_bt_management_callback,
	                                         &wiced_bt_cfg_settings);

	      /* Check if stack initialization was successful */
	      if( WICED_BT_SUCCESS == wiced_result)
	      {
	          printf("Bluetooth Stack Initialization Successful \n");
	      }
	      else
	      {
	          printf("Bluetooth Stack Initialization failed!! \n");
	          CY_ASSERT(0);
	      }
    wiced_result = WICED_BT_ERROR;
    wiced_bt_gatt_status_t gatt_status;
    for(;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if(button_press_for_adv)
        {
            wiced_result = wiced_bt_start_advertisements(BTM_BLE_ADVERT_UNDIRECTED_HIGH,
                                                         0, NULL);
            /* Failed to start advertisement, inform user */
            if (WICED_BT_SUCCESS != wiced_result)
            {
                printf("Failed to start advertisement! Error code: %X \n",
                       wiced_result);
            }
        }
        else
        {
            if((cts_discovery_data.cts_service_found == true) && (bt_connection_id != 0))
            {
                /* Toggle the flag to enable/disable CCCD based on previous state */
                notify_val = !notify_val;
                gatt_status = ble_app_write_notification_cccd(notify_val);
                if(WICED_BT_GATT_SUCCESS != gatt_status)
                {
                    printf("Enable/Disable notification failed! Error code: %X \n"
                           ,gatt_status);
                }
            }
        }
    }
}

static wiced_bt_gatt_status_t
ble_app_gatt_event_callback(wiced_bt_gatt_evt_t event,
                            wiced_bt_gatt_event_data_t *p_event_data)
{
    wiced_bt_gatt_status_t gatt_status = WICED_BT_GATT_SUCCESS;

    /* Call the appropriate callback function based on the GATT event type, and
     * pass the relevant event parameters to the callback function */
    switch ( event )
    {
        case GATT_CONNECTION_STATUS_EVT:
            gatt_status = ble_app_connect_handler(&p_event_data->connection_status);
            break;

        case GATT_DISCOVERY_RESULT_EVT:
            gatt_status = ble_app_discovery_result_handler(&p_event_data->discovery_result);
            break;

        case GATT_DISCOVERY_CPLT_EVT:
            gatt_status = ble_app_service_discovery_handler(&p_event_data->discovery_complete);
            break;

        case GATT_OPERATION_CPLT_EVT:
            switch (p_event_data->operation_complete.op)
            {
                case GATTC_OPTYPE_WRITE_WITH_RSP:
                    /* Check if GATT operation of enable/disable notification is success. */
                    if ((p_event_data->operation_complete.response_data.handle
                        == (cts_discovery_data.cts_cccd_handle))
                        && (WICED_BT_GATT_SUCCESS == p_event_data->operation_complete.status))
                    {
                        if(notify_val)
                        {
                            //printf("Notifications enabled\n");
                        }
                        else
                        {
                            printf("Notifications disabled\n");
                        }
                    }
                    else
                    {
                        printf("CCCD update failed. Error code: %d\n",
                               p_event_data->operation_complete.status);
                    }
                    break;

                case GATTC_OPTYPE_NOTIFICATION:
                    /* Function call to print the time and date notifcation */
                    handle_received_data_from_controller(p_event_data->operation_complete.response_data.att_value);
                    break;
            }
            break;

        case GATT_APP_BUFFER_TRANSMITTED_EVT:
        {
            vPortFree(p_event_data->buffer_xmitted.p_app_data);
            break;
        }

        default:
            gatt_status = WICED_BT_GATT_SUCCESS;
            break;
    }

    return gatt_status;
}

static wiced_bt_gatt_status_t
ble_app_connect_handler(wiced_bt_gatt_connection_status_t *p_conn_status)
{
    wiced_bt_gatt_status_t gatt_status =  WICED_BT_GATT_SUCCESS;
    wiced_bt_gatt_discovery_param_t service_discovery_setup = {0};
    if ( NULL == p_conn_status )
    {
        gatt_status = WICED_BT_GATT_ERROR;
        return gatt_status;
    }

    if ( p_conn_status->connected )
    {
        /* Device has connected */
        printf("Connected : BDA " );
        print_bd_address(p_conn_status->bd_addr);
        printf("Connection ID '%d' \n", p_conn_status->conn_id );

        /* Store the connection ID */
        bt_connection_id = p_conn_status->conn_id;
        /* After connection, successive button presses must enable/disable
        notification from server */
        button_press_for_adv = false;

        /* Send GATT service discovery request */
        service_discovery_setup.s_handle = 0x01;
        service_discovery_setup.e_handle = 0xFFFF;
        service_discovery_setup.uuid.len = LEN_UUID_16;
        service_discovery_setup.uuid.uu.uuid16 = UUID_SERVICE_CURRENT_TIME;

        gatt_status = wiced_bt_gatt_client_send_discover(bt_connection_id,
                                            GATT_DISCOVER_SERVICES_BY_UUID,
                                                    &service_discovery_setup);
        if(WICED_BT_GATT_SUCCESS != gatt_status)
        {
            printf("GATT Discovery request failed. Error code: %d, "
                    "Conn id: %d\n", gatt_status, bt_connection_id);
        }
        else
        {
            printf("Service Discovery Started\n");
        }
    }
    else
    {
        /* Device has disconnected */
        printf("Disconnected : BDA " );
        print_bd_address(p_conn_status->bd_addr);
        printf("Connection ID '%d', Reason '%s'\n", p_conn_status->conn_id,
                get_bt_gatt_disconn_reason_name(p_conn_status->reason) );

        /* Set the connection id to zero to indicate disconnected state */
        bt_connection_id = 0;

        /* Service discovery is performed upon reconnection, so reset the
            * status of service found flag */
        cts_discovery_data.cts_service_found = false;
        /* First button press after disconnection must start advertisement */
        button_press_for_adv = true;
    }
    return gatt_status;
}

static wiced_bt_gatt_status_t
ble_app_discovery_result_handler(wiced_bt_gatt_discovery_result_t *discovery_result)
{
    wiced_bt_gatt_status_t gatt_status =  WICED_BT_GATT_SUCCESS;
    wiced_bt_gatt_discovery_type_t discovery_type;
    discovery_type = discovery_result->discovery_type;
    switch (discovery_type)
    {
        case GATT_DISCOVER_SERVICES_BY_UUID:
            if(UUID_SERVICE_CURRENT_TIME ==
               discovery_result->discovery_data.group_value.service_type.uu.uuid16)
            {
                cts_discovery_data.cts_start_handle = discovery_result->discovery_data.group_value.s_handle;
                cts_discovery_data.cts_end_handle = discovery_result->discovery_data.group_value.e_handle;
                printf("CTS Service Found, Start Handle = %d, End Handle = %d \n",
                        cts_discovery_data.cts_start_handle,
                        cts_discovery_data.cts_end_handle);
            }
            break;

        case GATT_DISCOVER_CHARACTERISTICS:
            if(UUID_CHARACTERISTIC_CURRENT_TIME ==
               discovery_result->discovery_data.characteristic_declaration.char_uuid.uu.uuid16)
            {
                cts_discovery_data.cts_char_handle = discovery_result->discovery_data.characteristic_declaration.handle;
                cts_discovery_data.cts_char_val_handle = discovery_result->discovery_data.characteristic_declaration.val_handle;
                printf("Current Time characteristic handle = %d, "
                       "Current Time characteristic value handle = %d\n",
                        cts_discovery_data.cts_char_handle,
                        cts_discovery_data.cts_char_val_handle);
            }
            break;

        case GATT_DISCOVER_CHARACTERISTIC_DESCRIPTORS:
            if(UUID_DESCRIPTOR_CLIENT_CHARACTERISTIC_CONFIGURATION ==
               discovery_result->discovery_data.char_descr_info.type.uu.uuid16)
            {
                cts_discovery_data.cts_cccd_handle = discovery_result->discovery_data.char_descr_info.handle;
                cts_discovery_data.cts_service_found = true;
                printf("Current Time CCCD found, Handle = %d\n",
                        cts_discovery_data.cts_cccd_handle);
                printf("Press User button to start data communication\n");
                cyhal_gpio_write(LED_PIN, 0);
            }
            break;

        default:
            break;
        }

    return gatt_status;
}

static wiced_bt_gatt_status_t
ble_app_service_discovery_handler(wiced_bt_gatt_discovery_complete_t *discovery_complete)
{
    wiced_bt_gatt_status_t gatt_status =  WICED_BT_GATT_SUCCESS;
    wiced_bt_gatt_discovery_param_t char_discovery_setup = {0};
    wiced_bt_gatt_discovery_type_t discovery_type;
    discovery_type = discovery_complete->discovery_type;
    switch (discovery_type)
    {
        case GATT_DISCOVER_SERVICES_BY_UUID:
        {
            char_discovery_setup.s_handle = cts_discovery_data.cts_start_handle;
            char_discovery_setup.e_handle = cts_discovery_data.cts_end_handle;
            char_discovery_setup.uuid.uu.uuid16 = UUID_CHARACTERISTIC_CURRENT_TIME;
            gatt_status = wiced_bt_gatt_client_send_discover(bt_connection_id,
                                                GATT_DISCOVER_CHARACTERISTICS,
                                                &char_discovery_setup);
            if(WICED_BT_GATT_SUCCESS != gatt_status)
            printf("GATT characteristics discovery failed! Error code = %d\n", gatt_status);
            break;
        }

        case GATT_DISCOVER_CHARACTERISTICS:
        {
            char_discovery_setup.s_handle = cts_discovery_data.cts_start_handle;
            char_discovery_setup.e_handle = cts_discovery_data.cts_end_handle;
            char_discovery_setup.uuid.uu.uuid16 = UUID_CHARACTERISTIC_CURRENT_TIME;
            gatt_status = wiced_bt_gatt_client_send_discover(bt_connection_id,
                                               GATT_DISCOVER_CHARACTERISTIC_DESCRIPTORS,
                                               &char_discovery_setup);

            if(WICED_BT_GATT_SUCCESS != gatt_status)
            printf("GATT CCCD discovery failed! Error code = %d\n", gatt_status);
            break;
        }

        default:
            break;
    }
    return gatt_status;
}

// ***REQUEST CONTROLLER FOR DATA***
static wiced_bt_gatt_status_t ble_app_write_notification_cccd(bool notify)
{
    wiced_bt_gatt_write_hdr_t  write_hdr = {0};
    wiced_bt_gatt_status_t     gatt_status = WICED_BT_GATT_SUCCESS;
    uint8_t * notif_val = NULL;

    /* Allocate memory for data to be written on server DB and pass it to stack*/
    notif_val = pvPortMalloc(sizeof(uint16_t)); //CCCD is two bytes
    if (notif_val)
    {
        notif_val[0] = notify;
        notif_val[1] = 0;
        write_hdr.auth_req = GATT_AUTH_REQ_NONE;
        write_hdr.handle = cts_discovery_data.cts_cccd_handle;
        write_hdr.len      = LEN_UUID_16;
        write_hdr.offset = 0;

        for (;;)
        {
			gatt_status = wiced_bt_gatt_client_send_write(bt_connection_id,
														  GATT_REQ_WRITE,
														  &write_hdr,
														  notif_val,
														  NULL);
			vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
    return gatt_status;
}

static void handle_received_data_from_controller(wiced_bt_gatt_data_t notif_data)
{
	joystickData_t dataToSend;
	dataToSend.x = notif_data.p_data[0];
	dataToSend.y = notif_data.p_data[1];

	xQueueSend(joystickDataQueueHandle, &dataToSend, 0);
	printf("X: %d - Y: %d\n\r", notif_data.p_data[0], notif_data.p_data[1]);
}
