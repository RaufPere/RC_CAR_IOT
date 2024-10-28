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
#include "cts_server.h"
#include <stdlib.h>

/*******************************************************************************
*        Variable Definitions
*******************************************************************************/
static uint16_t bt_connection_id = 0;
cyhal_rtc_t my_rtc;

/*******************************************************************************
*        Function Prototypes
*******************************************************************************/
static void           ble_app_init                (void);
static void           ctss_send_notification      (void);
static void           ctss_scan_result_cback      (wiced_bt_ble_scan_results_t *p_scan_result,
                                                   uint8_t *p_adv_data );


/* GATT Event Callback Functions */
static wiced_bt_gatt_status_t ble_app_write_handler(uint16_t conn_id,
                                                    wiced_bt_gatt_opcode_t opcode,
                                                    wiced_bt_gatt_write_req_t *p_data, 
                                                    uint16_t *p_error_handle);
static wiced_bt_gatt_status_t ble_app_read_handler(uint16_t conn_id,
                                                   wiced_bt_gatt_opcode_t opcode,
                                                   wiced_bt_gatt_read_t *p_read_data,
                                                   uint16_t len_requested, 
                                                   uint16_t *p_error_handle);
static wiced_bt_gatt_status_t ble_app_connect_handler(wiced_bt_gatt_connection_status_t *p_conn_status);
static wiced_bt_gatt_status_t ble_app_server_handler(wiced_bt_gatt_attribute_request_t *p_data, 
                                                     uint16_t *p_error_handle);
static wiced_bt_gatt_status_t ble_app_gatt_event_callback(wiced_bt_gatt_evt_t event,
                                                          wiced_bt_gatt_event_data_t *p_event_data);
static wiced_bt_gatt_status_t app_bt_gatt_req_read_by_type_handler(uint16_t conn_id,
                                                                   wiced_bt_gatt_opcode_t opcode,
                                                                   wiced_bt_gatt_read_by_type_t *p_read_req,
                                                                   uint16_t len_requested, 
                                                                   uint16_t *p_error_handle);
static void* app_alloc_buffer(int len);
static void app_free_buffer(uint8_t *p_event_data);
gatt_db_lookup_table_t* app_get_attribute(uint16_t handle);

wiced_result_t app_bt_management_callback(wiced_bt_management_evt_t event,
                                          wiced_bt_management_evt_data_t *p_event_data)
{
    wiced_result_t result = WICED_BT_SUCCESS;
    wiced_bt_device_address_t bda = { 0 };

    switch (event)
    {
        case BTM_ENABLED_EVT:
            /* Bluetooth Controller and Host Stack Enabled */

            if (WICED_BT_SUCCESS == p_event_data->enabled.status)
            {
                /* Bluetooth is enabled */
                wiced_bt_dev_read_local_addr(bda);
                printf("Local Bluetooth Address: ");
                print_bd_address(bda);

                /* Perform application-specific initialization */
                ble_app_init();
            }

            break;

        case BTM_BLE_SCAN_STATE_CHANGED_EVT:

            if(p_event_data->ble_scan_state_changed == BTM_BLE_SCAN_TYPE_HIGH_DUTY)
            {
                printf("Scan State Change: BTM_BLE_SCAN_TYPE_HIGH_DUTY\n");
            }
            else if(p_event_data->ble_scan_state_changed == BTM_BLE_SCAN_TYPE_LOW_DUTY)
            {
                printf("Scan State Change: BTM_BLE_SCAN_TYPE_LOW_DUTY\n");
            }
            else if(p_event_data->ble_scan_state_changed == BTM_BLE_SCAN_TYPE_NONE)
            {
                printf("Scan stopped\n");
            }
            else
            {
                printf("Invalid scan state\n");
            }
            break;

        default:
            printf("Unhandled Bluetooth Management Event: 0x%x %s\n", event,
                                                   get_btm_event_name(event));
            break;
    }
    return result;
}

static void ble_app_init(void)
{
    cy_rslt_t              cy_result = CY_RSLT_SUCCESS;
    wiced_bt_gatt_status_t status    = WICED_BT_GATT_ERROR;

    /* GPIO init failed. Stop program execution */
    if (CY_RSLT_SUCCESS !=  cy_result)
    {
        printf("Button GPIO init failed! \n");
        CY_ASSERT(0);
    }

    /* Initialize RTC */
    cy_result = cyhal_rtc_init(&my_rtc);
    if(CY_RSLT_SUCCESS != cy_result)
    {
        printf("[Error] : RTC Initialization failed!! ");
        CY_ASSERT(0);
    }

   /* Disable pairing for this application */
    wiced_bt_set_pairable_mode(WICED_FALSE, 0);

    /* Register with BT stack to receive GATT callback */
    status = wiced_bt_gatt_register(ble_app_gatt_event_callback );
    printf("GATT event Handler registration status: %s \n",
            get_bt_gatt_status_name(status));

    /* Initialize GATT Database */
    status = wiced_bt_gatt_db_init(gatt_database, gatt_database_len, NULL);
    printf("GATT database initialization status: %s \n",
            get_bt_gatt_status_name(status));

    printf("Press User button to start scanning.....\n");
    BaseType_t xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;
	vTaskNotifyGiveFromISR(button_task_handle, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

void ctss_scan_result_cback(wiced_bt_ble_scan_results_t *p_scan_result,
                            uint8_t *p_adv_data )
{
    wiced_result_t         result = WICED_BT_SUCCESS;
    uint8_t                length = 0u;
    uint8_t                *adv_name;
    uint8_t                client_device_name[15] = {'C','T','S',' ','C','l',
                                                     'i','e','n','t','\0'};

    if (p_scan_result)
    {
        adv_name = wiced_bt_ble_check_advertising_data(p_adv_data,
                                BTM_BLE_ADVERT_TYPE_NAME_COMPLETE,
                                                         &length);
        if(NULL == adv_name)
        {
            return;
        }
        /* Check if the peer device's name is "BLE CTS Client" */
        if(0 == memcmp(adv_name, client_device_name, strlen((const char *)client_device_name)))
        {
            printf("\nFound the peer device! BD Addr: ");
            print_bd_address(p_scan_result->remote_bd_addr);

            /* Device found. Stop scan. */
            if((result = wiced_bt_ble_scan(BTM_BLE_SCAN_TYPE_NONE, WICED_TRUE,
                                           ctss_scan_result_cback))!= WICED_BT_SUCCESS)
            {
                printf("\r\nscan off status %d\n", result);
            }
            else
            {
                printf("Scan completed\n\n");
            }

            printf("Initiating connection\n");
            /* Initiate the connection */
            if(wiced_bt_gatt_le_connect(p_scan_result->remote_bd_addr,
                                        p_scan_result->ble_addr_type,
                                        BLE_CONN_MODE_HIGH_DUTY,
                                        WICED_TRUE)!= WICED_TRUE)
            {
                printf("\rwiced_bt_gatt_connect failed\n");
            }
        }
        else
        {
            printf("BD Addr: ");
            print_bd_address(p_scan_result->remote_bd_addr);
            return;    //Skip - This is not the device we are looking for.
        }
    }
}

static wiced_bt_gatt_status_t ble_app_gatt_event_callback (wiced_bt_gatt_evt_t event,
                                                           wiced_bt_gatt_event_data_t *p_event_data)
{
    wiced_bt_gatt_status_t gatt_status = WICED_BT_GATT_SUCCESS;

    uint16_t error_handle = 0;
    wiced_bt_gatt_attribute_request_t *p_attr_req = &p_event_data->attribute_request;

    /* Call the appropriate callback function based on the GATT event type,
       and pass the relevant event
     * parameters to the callback function */

    switch ( event )
    {
        case GATT_CONNECTION_STATUS_EVT:
            gatt_status = ble_app_connect_handler(&p_event_data->connection_status);
            break;

        case GATT_ATTRIBUTE_REQUEST_EVT:
            gatt_status = ble_app_server_handler(&p_event_data->attribute_request, 
                                                 &error_handle);
            if(gatt_status != WICED_BT_GATT_SUCCESS)
            {  
              wiced_bt_gatt_server_send_error_rsp(p_attr_req->conn_id, 
                                                  p_attr_req->opcode, 
                                                  error_handle, 
                                                  gatt_status);
            }

            break;

        default:
            gatt_status = WICED_BT_GATT_ERROR;
            break;
    }

    return gatt_status;
}

static wiced_bt_gatt_status_t ble_app_write_handler(uint16_t conn_id,
                                                    wiced_bt_gatt_opcode_t opcode,
                                                    wiced_bt_gatt_write_req_t *p_data, 
                                                    uint16_t *p_error_handle)
{
    wiced_bt_gatt_status_t gatt_status = WICED_BT_GATT_INVALID_HANDLE;
    int i = 0;
    wiced_bool_t isHandleInTable = WICED_FALSE;
    wiced_bool_t validLen = WICED_FALSE;

    *p_error_handle = p_data->handle;

    /* Check for a matching handle entry */
    for(i = 0; i < app_gatt_db_ext_attr_tbl_size; i++)
    {
        if(app_gatt_db_ext_attr_tbl[i].handle == p_data->handle)
        {
            /* Detected a matching handle in external lookup table */
            isHandleInTable = WICED_TRUE;

            /* Check if the buffer has space to store the data */
            validLen = (app_gatt_db_ext_attr_tbl[i].max_len >= p_data->val_len);

            if(validLen)
            {
                /* Value fits within the supplied buffer; copy over the value */
                app_gatt_db_ext_attr_tbl[i].cur_len = p_data->val_len;
                memcpy(app_gatt_db_ext_attr_tbl[i].p_data, p_data->p_val,
                       p_data->val_len);
                gatt_status = WICED_BT_GATT_SUCCESS;
            }
            else
            {
                /* Value to write does not meet size constraints */
                gatt_status = WICED_BT_GATT_INVALID_ATTR_LEN;
            }
            break;
        }
    }

    if (!isHandleInTable)
    {
        /* The write operation was not performed for the indicated handle */
        printf("Write Request to Invalid Handle: 0x%x\n", p_data->handle);
        gatt_status = WICED_BT_GATT_INVALID_HANDLE;
    }

    return gatt_status;
}

static wiced_bt_gatt_status_t ble_app_read_handler( uint16_t conn_id,
                                                    wiced_bt_gatt_opcode_t opcode,
                                                    wiced_bt_gatt_read_t *p_read_data,
                                                    uint16_t len_requested, 
                                                    uint16_t *p_error_handle)
{
    wiced_bt_gatt_status_t gatt_status = WICED_BT_SUCCESS;
    gatt_db_lookup_table_t *puAttribute;
    int attr_len_to_copy;

    *p_error_handle = p_read_data->handle;

    /* Get the right address for the handle in GATT DB */
    if (NULL == (puAttribute = app_get_attribute(p_read_data->handle)))
    {
        return WICED_BT_GATT_INVALID_HANDLE;
    }

    attr_len_to_copy = puAttribute->cur_len;

    printf("GATT Read handler: handle:0x%X, len:%d\n",
            p_read_data->handle, attr_len_to_copy);

    /* If the incoming offset is greater than the current length in the GATT DB
    then the data cannot be read back*/
    if (p_read_data->offset >= puAttribute->cur_len)
    {
        return (WICED_BT_GATT_INVALID_OFFSET);
    }

    int to_send = MIN(len_requested, attr_len_to_copy - p_read_data->offset);

    uint8_t *from = ((uint8_t *)puAttribute->p_data) + p_read_data->offset;

    gatt_status = wiced_bt_gatt_server_send_read_handle_rsp(conn_id, opcode, to_send, from, NULL);
    return gatt_status;
}

static wiced_bt_gatt_status_t app_bt_gatt_req_read_by_type_handler(uint16_t conn_id,
                                                                   wiced_bt_gatt_opcode_t opcode,
                                                                   wiced_bt_gatt_read_by_type_t *p_read_req,
                                                                   uint16_t len_requested, 
                                                                   uint16_t *p_error_handle)
{
    gatt_db_lookup_table_t *puAttribute;
    wiced_bt_gatt_status_t gatt_status;
    uint16_t last_handle = 0;
    uint16_t attr_handle = p_read_req->s_handle;
    uint8_t *p_rsp = app_alloc_buffer(len_requested);
    uint8_t pair_len = 0;
    int used = 0;

    if (p_rsp == NULL)
    {
        printf("No memory, len_requested: %d!!\r\n",len_requested);
        return WICED_BT_GATT_INSUF_RESOURCE;
    }

    /* Read by type returns all attributes of the specified type, between the
       start and end handles */
    while (WICED_TRUE)
    {
        *p_error_handle = attr_handle;
        last_handle = attr_handle;
        attr_handle = wiced_bt_gatt_find_handle_by_type(attr_handle,
                                                        p_read_req->e_handle,
                                                        &p_read_req->uuid);
        if (attr_handle == 0)
            break;

        if ((puAttribute = app_get_attribute(attr_handle)) == NULL)
        {
            printf("found type but no attribute for %d \r\n",last_handle);
            app_free_buffer(p_rsp);
            return WICED_BT_GATT_INVALID_HANDLE;
        }

        {
            int filled = wiced_bt_gatt_put_read_by_type_rsp_in_stream(p_rsp + used,
                                                                      len_requested - used,
                                                                      &pair_len,
                                                                      attr_handle,
                                                                      puAttribute->cur_len,
                                                                      puAttribute->p_data);
            if (filled == 0)
            {
                break;
            }
            used += filled;
        }

        /* Increment starting handle for next search to one past current */
        attr_handle++;
    }

    if(used == 0)
    {
        printf("attr not found  start_handle: 0x%04x, end_handle: 0x%04x,"
                                                     " Type: 0x%04x\r\n",
                                                    p_read_req->s_handle,
                                                    p_read_req->e_handle,
                                             p_read_req->uuid.uu.uuid16);

        app_free_buffer(p_rsp);
        return WICED_BT_GATT_INVALID_HANDLE;
    }

    /* Send the response */
    gatt_status = wiced_bt_gatt_server_send_read_by_type_rsp(conn_id,
                                                              opcode,
                                                            pair_len,
                                                                used,
                                                               p_rsp,
                                            (void *)app_free_buffer);
    if(WICED_BT_SUCCESS != gatt_status)
    {
        app_free_buffer(p_rsp);
    }

    return gatt_status;
}

static wiced_bt_gatt_status_t ble_app_connect_handler (wiced_bt_gatt_connection_status_t *p_conn_status)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;
    wiced_result_t result;

    if ( NULL != p_conn_status )
    {
        if ( p_conn_status->connected )
        {
            /* Device has connected */
            printf("Connected : BD Addr: " );
            print_bd_address(p_conn_status->bd_addr);
            printf("Connection ID '%d'\n", p_conn_status->conn_id);

            /* Store the connection ID */
            bt_connection_id = p_conn_status->conn_id;

            //connected();

        }
        else
        {
            /* Device has disconnected */
            printf("\nDisconnected : BD Addr: " );
            print_bd_address(p_conn_status->bd_addr);
            printf("Connection ID '%d', Reason '%s'\n", p_conn_status->conn_id,
                    get_bt_gatt_disconn_reason_name(p_conn_status->reason));

            /* Set the connection id to zero to indicate disconnected state */
            bt_connection_id = 0;

            /*restart the scan*/
            result = wiced_bt_ble_scan(BTM_BLE_SCAN_TYPE_HIGH_DUTY,
                                       WICED_TRUE,
                                       ctss_scan_result_cback);
            if(WICED_BT_PENDING != result)
            {
                printf("Cannot restart scanning. Error: %d \n", result);
            }
            else
            {
                printf("\r\nScanning.....\n");
            }

        }

        status = WICED_BT_GATT_ERROR ;
    }

    return status;
}

static wiced_bt_gatt_status_t ble_app_server_handler (wiced_bt_gatt_attribute_request_t *p_data, uint16_t *p_error_handle)
{
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;
    wiced_bt_gatt_write_req_t *p_write_request = &p_data->data.write_req;

    switch ( p_data->opcode )
    {
        case GATT_REQ_READ:
        case GATT_REQ_READ_BLOB:
            /* Attribute read request */
            status = ble_app_read_handler(p_data->conn_id, p_data->opcode,
                                          &p_data->data.read_req,
                                          p_data->len_requested, p_error_handle);
            break;
        case GATT_REQ_READ_BY_TYPE:
            status = app_bt_gatt_req_read_by_type_handler(p_data->conn_id,
                                                          p_data->opcode,
                                                          &p_data->data.read_by_type,
                                                          p_data->len_requested, p_error_handle);
            break;
            /* Attribute write request */
        case GATT_REQ_WRITE:
        case GATT_CMD_WRITE:
            status = ble_app_write_handler(p_data->conn_id,
                                           p_data->opcode,
                                           &(p_data->data.write_req), p_error_handle);
            if((p_data->opcode == GATT_REQ_WRITE) && (status == WICED_BT_GATT_SUCCESS))
            {
                wiced_bt_gatt_server_send_write_rsp(p_data->conn_id,
                                                    p_data->opcode,
                                                    p_write_request->handle);
                if(app_cts_current_time_client_char_config[0])
                {
                    ctss_send_notification();
                }
            }
            break;

        default:
            status = WICED_BT_GATT_ERROR;
    }
    return status;
}

// ***DATA SEND FUNCTION***
static void ctss_send_notification(void)
{
    cy_rslt_t  cy_result;
    struct tm date_time;
    char buffer[STRING_BUFFER_SIZE];
    wiced_bt_gatt_status_t status = WICED_BT_GATT_SUCCESS;

    cy_result = cyhal_rtc_read(&my_rtc, &date_time);

    date_time.tm_year += TM_YEAR_BASE;

    // ***DATA TO BE SENT***
    app_cts_current_time[0] = 'a';

    status = wiced_bt_gatt_server_send_notification(bt_connection_id,
                                                    HDLC_CTS_CURRENT_TIME_VALUE,
                                                    1,
                                                    app_cts_current_time,NULL);

    if (WICED_BT_GATT_SUCCESS != status)
    {
        printf("Send notification failed\n");
    }
    else
    {
    	printf("Succesfully sent data: %d\n\r",app_cts_current_time[0]);
    }
}

void button_task(void *pvParameters)
{
    wiced_result_t result;
    for(;;)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        result = wiced_bt_ble_scan(BTM_BLE_SCAN_TYPE_HIGH_DUTY, WICED_TRUE,
                                   ctss_scan_result_cback);
        if ((WICED_BT_PENDING == result) || (WICED_BT_BUSY == result))
        {
            printf("\r\nScanning.....\n");
        }
        else
        {
            printf("\rError: Starting scan failed. Error code: %d\n", result);
            return;
        }
    }
}

gatt_db_lookup_table_t *app_get_attribute(uint16_t handle)
{
    /* Search for the given handle in the GATT DB and return the pointer to the
    correct attribute */
    uint8_t array_index = 0;

    for (array_index = 0; array_index < app_gatt_db_ext_attr_tbl_size; array_index++)
    {
        if (app_gatt_db_ext_attr_tbl[array_index].handle == handle)
        {
            return (&app_gatt_db_ext_attr_tbl[array_index]);
        }
    }
    return NULL;
}

static void app_free_buffer(uint8_t *p_buf)
{
    vPortFree(p_buf);
}

static void* app_alloc_buffer(int len)
{
    return pvPortMalloc(len);
}
/* [] END OF FILE */
