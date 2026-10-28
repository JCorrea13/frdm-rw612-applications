/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2022 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "lwip/tcpip.h"
#include "board.h"
#include "app.h"
#include "wpl.h"
#include "timers.h"
#include "wifi_provisioning.h"

#include "fsl_debug_console.h"
#include "mqtt_freertos.h"

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#ifndef AP_SSID
#define AP_SSID "#FestivalCultural"
#endif

#ifndef AP_PASSWORD
#define AP_PASSWORD ""
#endif

#define WIFI_NETWORK_LABEL "my_wifi"

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

/* Link lost callback */
static void LinkStatusChangeCallback(bool linkState)
{
    if (linkState == false)
    {
        /* -------- LINK LOST -------- */
        /* DO SOMETHING */
        PRINTF("-------- LINK LOST --------\r\n");
    }
    else
    {
        /* -------- LINK REESTABLISHED -------- */
        /* DO SOMETHING */
        PRINTF("-------- LINK REESTABLISHED --------\r\n");
    }
}

/* Connect to the external AP */
static bool ConnectToSavedNetwork(void)
{
    char savedSsid[WPL_WIFI_SSID_LENGTH + 1U];
    char savedPassword[WPL_WIFI_PASSWORD_LENGTH + 1U];
    int32_t result;

    if (!wifi_credentials_loaded(savedSsid, sizeof(savedSsid), savedPassword, sizeof(savedPassword)))
    {
        return false;
    }

    result = WPL_AddNetwork(savedSsid, savedPassword, WIFI_NETWORK_LABEL);
    if (result == WPLRET_SUCCESS)
    {
        PRINTF("Connecting as client to ssid: %s with password %s\r\n", savedSsid, savedPassword);
        result = WPL_Join(WIFI_NETWORK_LABEL);
    }

    if (result != WPLRET_SUCCESS)
    {
        PRINTF("[!] Cannot connect to saved Wi-Fi credentials\r\n[!]ssid: %s\r\n[!]passphrase: %s\r\n",
               savedSsid,
               savedPassword);
        return false;
    }

    PRINTF("[i] Connected to Wi-Fi\r\nssid: %s\r\n[!]passphrase: %s\r\n", savedSsid, savedPassword);
    char ip[16];
    WPL_GetIP(ip, 1);
    return true;
}

/*!
 * @brief The main task function
 */
static void main_task(void *arg)
{
    uint32_t result = 0;

    PRINTF("\r\n************************************************\r\n");
    PRINTF(" MQTT client example\r\n");
    PRINTF("************************************************\r\n");

    /* Initialize Wi-Fi board */
    PRINTF("[i] Initializing Wi-Fi connection... \r\n");

    result = WPL_Init();
    if (result != WPLRET_SUCCESS)
    {
        PRINTF("[!] WPL Init failed: %d\r\n", (uint32_t)result);
        __BKPT(0);
    }

    result = WPL_Start(LinkStatusChangeCallback);
    if (result != WPLRET_SUCCESS)
    {
        PRINTF("[!] WPL Start failed %d\r\n", (uint32_t)result);
        __BKPT(0);
    }

    PRINTF("[i] Successfully initialized Wi-Fi module\r\n");

    if (!ConnectToSavedNetwork())
    {
        PRINTF("[i] No valid saved Wi-Fi credentials or join failed. Entering provisioning mode.\r\n");
        wifi_enter_provisioning_mode();
    }

    /// wait_dns
    mqtt_freertos_run_thread(netif_default);

    vTaskDelete(NULL);
}

/*!
 * @brief Main function.
 */
int main(void)
{
    /* Initialize the hardware */
    BOARD_InitHardware();

    /* Create the main Task */
    if (xTaskCreate(main_task, "main_task", 1024, NULL, configMAX_PRIORITIES - 4, NULL) != pdPASS)
    {
        PRINTF("[!] MAIN Task creation failed!\r\n");
        while (1)
            ;
    }

    /* Run RTOS */
    vTaskStartScheduler();

    /* Should not reach this statement */
    for (;;)
        ;
}
