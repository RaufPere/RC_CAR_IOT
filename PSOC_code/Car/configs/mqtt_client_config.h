/******************************************************************************
* File Name:   mqtt_client_config.h
*
* Description: This file contains all the configuration macros used by the
*              MQTT client in this example.
*
* Related Document: See README.md
*
*
*******************************************************************************
* Copyright 2020-2024, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/

#ifndef MQTT_CLIENT_CONFIG_H_
#define MQTT_CLIENT_CONFIG_H_

#include "cy_mqtt_api.h"

/*******************************************************************************
* Macros
********************************************************************************/

/***************** MQTT CLIENT CONNECTION CONFIGURATION MACROS *****************/
/* MQTT Broker/Server address and port used for the MQTT connection. */
#define MQTT_BROKER_ADDRESS               "1f921750cd3c40f8a5b596c38018d775.s1.eu.hivemq.cloud"
#define MQTT_PORT                         8883

/* Set this macro to 1 if a secure (TLS) connection to the MQTT Broker is
 * required to be established, else 0.
 */
#define MQTT_SECURE_CONNECTION            ( 1 )

/* Configure the user credentials to be sent as part of MQTT CONNECT packet */
#define MQTT_USERNAME                     "Psoc6_Uhasselt"
#define MQTT_PASSWORD                     "Psoc6_Uhasselt"

/********************* MQTT MESSAGE CONFIGURATION MACROS **********************/
/* The MQTT topics to be used by the publisher and subscriber. */
#define MQTT_PUB_TOPIC                    "MPU6050"
#define MQTT_SUB_TOPIC_MPU                "MPU6050"
#define MQTT_SUB_TOPIC_RESET_GYRO         "GYROrst"
#define MQTT_SUB_TOPIC_MOTOR        	  "MOTOR"
#define MQTT_SUB_TOPIC_LP   	          "LP"

/* Set the QoS that is associated with the MQTT publish, and subscribe messages.
 * Valid choices are 0, 1, and 2. Other values should not be used in this macro.
 */
#define MQTT_MESSAGES_QOS                 ( 1 )

/* Configuration for the 'Last Will and Testament (LWT)'. It is an MQTT message
 * that will be published by the MQTT broker if the MQTT connection is
 * unexpectedly closed. This configuration is sent to the MQTT broker during
 * MQTT connect operation and the MQTT broker will publish the Will message on
 * the Will topic when it recognizes an unexpected disconnection from the client.
 *
 * If you want to use the last will message, set this macro to 1 and configure
 * the topic and will message, else 0.
 */
#define ENABLE_LWT_MESSAGE                ( 0 )
#if ENABLE_LWT_MESSAGE
    #define MQTT_WILL_TOPIC_NAME          MQTT_PUB_TOPIC "/will"
    #define MQTT_WILL_MESSAGE             ("MQTT client unexpectedly disconnected!")
#endif

/* MQTT messages which are published on the MQTT_PUB_TOPIC that controls the
 * device (user LED in this example) state in this code example.
 */


/******************* OTHER MQTT CLIENT CONFIGURATION MACROS *******************/
/* A unique client identifier to be used for every MQTT connection. */
#define MQTT_CLIENT_IDENTIFIER            "Psoc6"

#define MQTT_DEVICE_ON_MESSAGE            "TURN ON"
#define MQTT_DEVICE_OFF_MESSAGE           "TURN OFF"

/* The timeout in milliseconds for MQTT operations in this example. */
#define MQTT_TIMEOUT_MS                   ( 5000 )

/* The keep-alive interval in seconds used for MQTT ping request. */
#define MQTT_KEEP_ALIVE_SECONDS           ( 60 )

/* Every active MQTT connection must have a unique client identifier. If you
 * are using the above 'MQTT_CLIENT_IDENTIFIER' as client ID for multiple MQTT
 * connections simultaneously, set this macro to 1. The device will then
 * generate a unique client identifier by appending a timestamp to the
 * 'MQTT_CLIENT_IDENTIFIER' string. Example: 'psoc6-mqtt-client5927'
 */
#define GENERATE_UNIQUE_CLIENT_ID         ( 1 )

/* The longest client identifier that an MQTT server must accept (as defined
 * by the MQTT 3.1.1 spec) is 23 characters. However some MQTT brokers support
 * longer client IDs. Configure this macro as per the MQTT broker specification.
 */
#define MQTT_CLIENT_IDENTIFIER_MAX_LEN    ( 23 )

/* As per Internet Assigned Numbers Authority (IANA) the port numbers assigned
 * for MQTT protocol are 1883 for non-secure connections and 8883 for secure
 * connections. In some cases there is a need to use other ports for MQTT like
 * port 443 (which is reserved for HTTPS). Application Layer Protocol
 * Negotiation (ALPN) is an extension to TLS that allows many protocols to be
 * used over a secure connection. The ALPN ProtocolNameList specifies the
 * protocols that the client would like to use to communicate over TLS.
 *
 * This macro specifies the ALPN Protocol Name to be used that is supported
 * by the MQTT broker in use.
 * Note: For AWS IoT, currently "x-amzn-mqtt-ca" is the only supported ALPN
 *       ProtocolName and it is only supported on port 443.
 *
 * Uncomment the below line and specify the ALPN Protocol Name to use this
 * feature.
 */
// #define MQTT_ALPN_PROTOCOL_NAME           "x-amzn-mqtt-ca"

/* Server Name Indication (SNI) is extension to the Transport Layer Security
 * (TLS) protocol. As required by some MQTT Brokers, SNI typically includes the
 * hostname in the Client Hello message sent during TLS handshake.
 *
 * Specify the SNI Host Name to use this extension
 * as specified by the MQTT Broker.
 */
#define MQTT_SNI_HOSTNAME 		"1f921750cd3c40f8a5b596c38018d775.s1.eu.hivemq.cloud"

/* A Network buffer is allocated for sending and receiving MQTT packets over
 * the network. Specify the size of this buffer using this macro.
 *
 * Note: The minimum buffer size is defined by 'CY_MQTT_MIN_NETWORK_BUFFER_SIZE'
 * macro in the MQTT library. Please ensure this macro value is larger than
 * 'CY_MQTT_MIN_NETWORK_BUFFER_SIZE'.
 */
#define MQTT_NETWORK_BUFFER_SIZE          ( 2 * CY_MQTT_MIN_NETWORK_BUFFER_SIZE )

/* Maximum MQTT connection re-connection limit. */
#define MAX_MQTT_CONN_RETRIES            (150u)
// 150
/* MQTT re-connection time interval in milliseconds. */
#define MQTT_CONN_RETRY_INTERVAL_MS      (2000)


/**************** MQTT CLIENT CERTIFICATE CONFIGURATION MACROS ****************/

/* Configure the below credentials in case of a secure MQTT connection. */
/* PEM-encoded client certificate */

#define CLIENT_CERTIFICATE      \
"-----BEGIN CERTIFICATE-----\n"\
"MIIDuDCCAqCgAwIBAgIBADANBgkqhkiG9w0BAQsFADCBkDELMAkGA1UEBhMCR0Ix"\
"FzAVBgNVBAgMDlVuaXRlZCBLaW5nZG9tMQ4wDAYDVQQHDAVEZXJieTESMBAGA1UE"\
"CgwJTW9zcXVpdHRvMQswCQYDVQQLDAJDQTEWMBQGA1UEAwwNbW9zcXVpdHRvLm9y"\
"ZzEfMB0GCSqGSIb3DQEJARYQcm9nZXJAYXRjaG9vLm9yZzAeFw0yNDExMTUxMDUy"\
"MjJaFw0yNTAyMTMxMDUyMjJaMIGRMQswCQYDVQQGEwJCRTEQMA4GA1UECAwHTGlt"\
"YnVyZzERMA8GA1UEBwwIWm9uaG92ZW4xETAPBgNVBAoMCFVoYXNzZWx0MRQwEgYD"\
"VQQLDAtFbGVjdHJvbmljczEPMA0GA1UEAwwGRGFycmVuMSMwIQYJKoZIhvcNAQkB"\
"FhRkYXJyZW43MDgwQGdtYWlsLmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCC"\
"AQoCggEBAMQMvhIeV/U2CXPb+NgrbfflRWfdZycMlOBN0rTAzq5ML3EXiAcR+o2l"\
"PxBaJGOaeGGHky+5oGw3mGS4PQFAtnVYyDFifFwONDVLleN4f8GDMy+rSMW4Pqh/"\
"0+8W3BZiKImyc+p6Coc9HrwEMXySd2hJyribRY95By7C1IDmtCeQU80oG84SvZgX"\
"Owj1jl/Q5gtvRnXDoeZmx6ST2DRFol5xhc2o3rtFKGrk2hXvD2ihGMcFvAFszuoI"\
"0J+N1O/js9pJWKMqWNWBtk9AjYJ3bvLhb09L2iKeG/T54zGzrHxuToJnJkDKPW23"\
"41A2KSzwln+8MV3brtKG+jw+BO4si1UCAwEAAaMaMBgwCQYDVR0TBAIwADALBgNV"\
"HQ8EBAMCBeAwDQYJKoZIhvcNAQELBQADggEBAGBLum52k0zqR7ItQKr8NUX7sR6e"\
"526VA2hy9PXJxTAbIFkQwlCU1J8K1BnoBlTczXtxM63G3mn789iheX3EgfIqo/tG"\
"U6A4+9wE69XYETDR8IJCWGKREnuGGqN8JMRSbW8vlzBgHU8EI6hFsiwHPpAg/WE4"\
"SUSdpt7kZlkukW3rGF8qF9+xgJgS9LEbwhxfN1hmxIitW8sI6XAm9+sq8zh2thsv"\
"yIiBCEGkaZuNkSwY/PXPCLZIwkm++pegQ8F/Sh7SI1wrwA37MBqeAg9IpRjympYo"\
"EdOePNqSPDs4kWf6JJuEARj6U8OUkQgJnWgJqWvVWrIDr6eGlwphDSLPr/k="\
"-----END CERTIFICATE-----"



/* PEM-encoded client private key */

#define CLIENT_PRIVATE_KEY          \
"-----BEGIN PRIVATE KEY-----\n"\
"MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDEDL4SHlf1Nglz"\
"2/jYK2335UVn3WcnDJTgTdK0wM6uTC9xF4gHEfqNpT8QWiRjmnhhh5MvuaBsN5hk"\
"uD0BQLZ1WMgxYnxcDjQ1S5XjeH/BgzMvq0jFuD6of9PvFtwWYiiJsnPqegqHPR68"\
"BDF8kndoScq4m0WPeQcuwtSA5rQnkFPNKBvOEr2YFzsI9Y5f0OYLb0Z1w6HmZsek"\
"k9g0RaJecYXNqN67RShq5NoV7w9ooRjHBbwBbM7qCNCfjdTv47PaSVijKljVgbZP"\
"QI2Cd27y4W9PS9oinhv0+eMxs6x8bk6CZyZAyj1tt+NQNiks8JZ/vDFd267Shvo8"\
"PgTuLItVAgMBAAECggEAKc3Hx8hFnClyGYzpwnp8P/RY+N1KjhaONkgR6m4EhEEF"\
"irloTY2lXgfC60M1aDG7gpxso6wSy5CNQ8jPZVF9tocAZRtphh2XP1gHNZJvt304"\
"8Gy3H0X6wmZDdENtGa7Dkx0Ev5ZXYWAEijXgsX3vzUKS4gtl6UJ26B8uPfAzsQ2j"\
"ZQJr11KwPlFNZP+8fiZHEZtbBGJXP3id2w+C9xSUO1tUehi18dv3Xvc0h2rfSBg5"\
"MBoTWa9zNJpg782uUSAtv19/RwGe7fLgAy5semirNOKez/0nF3G6s5z1y4frmFp6"\
"via/QTI4dEFTLKZrxNOHfkPr5qrwr0ZiuUeiCZfnAQKBgQDmVK8u6hH3K0aqKA58"\
"qWwWVYSz+N6OEVYbdqV4LOdo8DdvwDWUSbxE9VLWdBojMPsTCfxrsbz90MvfUpEw"\
"T/2bVqW6fB02wI2xYkqOg5/bUatVEqArh43/QZZQtxJisIeCzrFM6W+Yae7cbnGJ"\
"6Z+yL5w6cVSIjzavsL0pw5xJ1QKBgQDZ5gYapbCsd4DITnnOIFZV1fw4au+B/QWC"\
"oOUdRbm0QIUIVTiYf/QWaj9ujvJSOlBZGkQ6j86mWZwZrosB8g9aR11oOvKbuCJN"\
"mK0ORHq5a8cagoDsgQ2Zh6PYqzL6RLI6M0aEOP/0BoJ7PGsUdsJ5NIBR4r1B8Gsj"\
"SLugiCp7gQKBgDFtmuqGcLSKJO//CJzX/soMMB4vGhOCxOe/3mKbwE1Uv5DLvrPD"\
"3xMtUkmTQMisijTbv7+ctOIDHL0gCuhDQjlDru0GFX7ac43NehJ7TKLUM+BdzuGI"\
"hYBxosBPiTKEj5ASqxnGPpyGnyvOx67A4/RpIy2nhGyJz0KEuxNrVu7BAoGBAJ2C"\
"h3sPtqyVHgStpaL5AS+/JTlrI4LlcexBfh0w39KKqR+i6uxh4gGp//xdXJFQEfC2"\
"6gcjGRBqykpOP7HCVpK976l+ow0ph7Jr4PzlR4ltfVmOaL+NclU/FZNz/b+nlOY5"\
"VooR4gN2CTAoNoduaZnP98o5ivF1Zxz0YJzkkUEBAoGBANrHHGNoKBSadeV03y7T"\
"A/FbAlybLBBZXm0t+DeWuV1fkm9SjdGqeD1D4OYjVeLFKMwKhQ7WYxyxAF88XDck"\
"gL0yABAhNdGcvfSwTFT5sb7b2c3Bizm132CsPOL+2zm8Jfq7ovvES5lpy1QT15ZJ"\
"pHBBsFX6Gi/iBXzErpR2kaOH"\
"-----END PRIVATE KEY-----"



/* PEM-encoded Root CA certificate */

#define ROOT_CA_CERTIFICATE     \
"-----BEGIN CERTIFICATE-----\n" \
"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\n" \
"-----END CERTIFICATE-----"

/******************************************************************************
* Global Variables
*******************************************************************************/
extern cy_mqtt_broker_info_t broker_info;
extern cy_awsport_ssl_credentials_t  *security_info;
extern cy_mqtt_connect_info_t connection_info;


#endif /* MQTT_CLIENT_CONFIG_H_ */
