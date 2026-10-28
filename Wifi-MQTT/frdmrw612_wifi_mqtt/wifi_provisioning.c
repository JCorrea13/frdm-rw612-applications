#include "wifi_provisioning.h"

#include "lwip/apps/mdns.h"
#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/sockets.h"
#include "lwip/tcpip.h"
#include "wpl.h"

#include "fsl_debug_console.h"
#include "mflash_drv.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define WIFI_CREDENTIALS_MAGIC 0x57464954u
#define WIFI_CREDENTIALS_MAX_SSID (WPL_WIFI_SSID_LENGTH + 1U)
#define WIFI_CREDENTIALS_MAX_PASS (WPL_WIFI_PASSWORD_LENGTH + 1U)
#define WIFI_CREDENTIALS_STORAGE_ADDR 0x0801F000UL

#define WIFI_PROVISIONING_BUFFER_SIZE 256

#define WIFI_PROVISIONING_FIELD_SSID "ssid"
#define WIFI_PROVISIONING_FIELD_PASSWORD "password"

typedef struct
{
    uint32_t magic;
    char ssid[WIFI_CREDENTIALS_MAX_SSID];
    char password[WIFI_CREDENTIALS_MAX_PASS];
} wifi_credentials_store_t;

bool wifi_credentials_loaded(char *ssid, size_t ssidLength, char *password, size_t passwordLength)
{
    wifi_credentials_store_t stored;
    uint32_t raw[(sizeof(stored) + sizeof(uint32_t) - 1U) / sizeof(uint32_t)] = {0};

    if ((ssid == NULL) || (password == NULL))
    {
        return false;
    }

    if (mflash_drv_init() != 0)
    {
        return false;
    }

    if (mflash_drv_read(WIFI_CREDENTIALS_STORAGE_ADDR, raw, sizeof(stored)) != 0)
    {
        return false;
    }

    memcpy(&stored, raw, sizeof(stored));
    if ((stored.magic != WIFI_CREDENTIALS_MAGIC) || (stored.ssid[0] == '\0'))
    {
        return false;
    }

    if (strlen(stored.ssid) >= ssidLength)
    {
        return false;
    }

    if (strlen(stored.password) >= passwordLength)
    {
        return false;
    }

    strcpy(ssid, stored.ssid);
    strcpy(password, stored.password);
    return true;
}

bool wifi_credentials_store(const char *ssid, const char *password)
{
    wifi_credentials_store_t stored;
    uint32_t raw[(sizeof(stored) + sizeof(uint32_t) - 1U) / sizeof(uint32_t)] = {0};
    size_t ssidLen;
    size_t passwordLen;

    if ((ssid == NULL) || (password == NULL))
    {
        return false;
    }

    ssidLen = strlen(ssid);
    passwordLen = strlen(password);

    if ((ssidLen == 0U) || (ssidLen >= WIFI_CREDENTIALS_MAX_SSID) ||
        (passwordLen == 0U) || (passwordLen >= WIFI_CREDENTIALS_MAX_PASS))
    {
        return false;
    }

    if (mflash_drv_init() != 0)
    {
        return false;
    }

    memset(&stored, 0, sizeof(stored));
    stored.magic = WIFI_CREDENTIALS_MAGIC;
    strcpy(stored.ssid, ssid);
    strcpy(stored.password, password);

    memcpy(raw, &stored, sizeof(stored));

    if (mflash_drv_sector_erase(WIFI_CREDENTIALS_STORAGE_ADDR) != 0)
    {
        return false;
    }

    if (mflash_drv_page_program(WIFI_CREDENTIALS_STORAGE_ADDR, raw) != 0)
    {
        return false;
    }

    return true;
}

static bool wifi_parse_key_value(const char *payload, char *ssid, size_t ssidSize, char *password, size_t passwordSize)
{
    const char *ssidStart;
    const char *passwordStart;
    const char *ssidMatch;
    const char *passwordMatch;
    const char *jsonSsid;
    const char *jsonPassword;
    char *endPtr;

    if ((payload == NULL) || (ssid == NULL) || (password == NULL))
    {
        return false;
    }

    ssidMatch = strstr(payload, WIFI_PROVISIONING_FIELD_SSID);
    passwordMatch = strstr(payload, WIFI_PROVISIONING_FIELD_PASSWORD);

    if ((ssidMatch != NULL) && (passwordMatch != NULL))
    {
        ssidStart = ssidMatch + strlen(WIFI_PROVISIONING_FIELD_SSID);
        while ((*ssidStart == '=') || (*ssidStart == ':') || (*ssidStart == ' ') || (*ssidStart == '"'))
        {
            ssidStart++;
        }

        passwordStart = passwordMatch + strlen(WIFI_PROVISIONING_FIELD_PASSWORD);
        while ((*passwordStart == '=') || (*passwordStart == ':') || (*passwordStart == ' ') || (*passwordStart == '"'))
        {
            passwordStart++;
        }

        endPtr = strchr(ssidStart, '&');
        if (endPtr == NULL)
        {
            endPtr = strchr(ssidStart, '\r');
        }
        if (endPtr == NULL)
        {
            endPtr = strchr(ssidStart, '\n');
        }
        if (endPtr == NULL)
        {
            endPtr = strchr(ssidStart, '"');
        }

        if (endPtr != NULL)
        {
            size_t length = (size_t)(endPtr - ssidStart);
            if ((length + 1U) > ssidSize)
            {
                return false;
            }
            memcpy(ssid, ssidStart, length);
            ssid[length] = '\0';
        }
        else
        {
            strncpy(ssid, ssidStart, ssidSize - 1U);
            ssid[ssidSize - 1U] = '\0';
        }

        endPtr = strchr(passwordStart, '&');
        if (endPtr == NULL)
        {
            endPtr = strchr(passwordStart, '\r');
        }
        if (endPtr == NULL)
        {
            endPtr = strchr(passwordStart, '\n');
        }
        if (endPtr == NULL)
        {
            endPtr = strchr(passwordStart, '"');
        }

        if (endPtr != NULL)
        {
            size_t length = (size_t)(endPtr - passwordStart);
            if ((length + 1U) > passwordSize)
            {
                return false;
            }
            memcpy(password, passwordStart, length);
            password[length] = '\0';
        }
        else
        {
            strncpy(password, passwordStart, passwordSize - 1U);
            password[passwordSize - 1U] = '\0';
        }

        return ((ssid[0] != '\0') && (password[0] != '\0'));
    }

    jsonSsid = strstr(payload, "\"ssid\"");
    jsonPassword = strstr(payload, "\"password\"");
    if ((jsonSsid != NULL) && (jsonPassword != NULL))
    {
        const char *ssidValue = strchr(jsonSsid, ':');
        const char *passwordValue = strchr(jsonPassword, ':');
        const char *ssidEnd;
        const char *passwordEnd;

        if ((ssidValue == NULL) || (passwordValue == NULL))
        {
            return false;
        }

        ssidValue++;
        while ((*ssidValue == ' ') || (*ssidValue == '"'))
        {
            ssidValue++;
        }
        ssidEnd = strchr(ssidValue, '"');
        if (ssidEnd == NULL)
        {
            ssidEnd = strchr(ssidValue, ',');
        }
        if (ssidEnd == NULL)
        {
            ssidEnd = strchr(ssidValue, '}');
        }
        if (ssidEnd == NULL)
        {
            return false;
        }
        if ((size_t)(ssidEnd - ssidValue) >= ssidSize)
        {
            return false;
        }
        memcpy(ssid, ssidValue, (size_t)(ssidEnd - ssidValue));
        ssid[ssidEnd - ssidValue] = '\0';

        passwordValue++;
        while ((*passwordValue == ' ') || (*passwordValue == '"'))
        {
            passwordValue++;
        }
        passwordEnd = strchr(passwordValue, '"');
        if (passwordEnd == NULL)
        {
            passwordEnd = strchr(passwordValue, ',');
        }
        if (passwordEnd == NULL)
        {
            passwordEnd = strchr(passwordValue, '}');
        }
        if (passwordEnd == NULL)
        {
            return false;
        }
        if ((size_t)(passwordEnd - passwordValue) >= passwordSize)
        {
            return false;
        }
        memcpy(password, passwordValue, (size_t)(passwordEnd - passwordValue));
        password[passwordEnd - passwordValue] = '\0';

        return ((ssid[0] != '\0') && (password[0] != '\0'));
    }

    return false;
}

static void wifi_provisioning_start_mdns(void)
{
#if LWIP_MDNS_RESPONDER
    mdns_resp_init();
    mdns_resp_add_netif(netif_default, "rw612-prov");
    mdns_resp_add_service(netif_default, "rw612-prov", "_wifi-prov", DNSSD_PROTO_TCP, WIFI_PROVISIONING_PORT, NULL, NULL);
    mdns_resp_announce(netif_default);
#else
    PRINTF("[!] mDNS responder is disabled in lwIP build.\r\n");
#endif
}

void wifi_enter_provisioning_mode(void)
{
    int serverFd;
    int clientFd;
    struct sockaddr_in serverAddress;
    char payload[WIFI_PROVISIONING_BUFFER_SIZE] = {0};
    char ssid[WPL_WIFI_SSID_LENGTH + 1U] = {0};
    char password[WPL_WIFI_PASSWORD_LENGTH + 1U] = {0};
    int received;

    PRINTF("[i] Starting provisioning mode.\r\n");

    if (WPL_Start_AP(WIFI_PROVISIONING_SSID, WIFI_PROVISIONING_PASSWORD, 1) != WPLRET_SUCCESS)
    {
        PRINTF("[!] Failed to start provisioning access point.\r\n");
        return;
    }

    wifi_provisioning_start_mdns();

    serverFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverFd < 0)
    {
        PRINTF("[!] Failed to create provisioning socket.\r\n");
        WPL_Stop_AP();
        return;
    }

    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(WIFI_PROVISIONING_PORT);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(serverFd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) != 0)
    {
        PRINTF("[!] Failed to bind provisioning socket on port %d.\r\n", WIFI_PROVISIONING_PORT);
        closesocket(serverFd);
        WPL_Stop_AP();
        return;
    }

    if (listen(serverFd, 1) != 0)
    {
        PRINTF("[!] Failed to listen on provisioning socket.\r\n");
        closesocket(serverFd);
        WPL_Stop_AP();
        return;
    }

    PRINTF("[i] Waiting for provisioning credentials on port %d.\r\n", WIFI_PROVISIONING_PORT);

    clientFd = accept(serverFd, NULL, NULL);
    if (clientFd < 0)
    {
        PRINTF("[!] Provisioning client connection failed.\r\n");
        closesocket(serverFd);
        WPL_Stop_AP();
        return;
    }

    memset(payload, 0, sizeof(payload));
    received = recv(clientFd, payload, sizeof(payload) - 1U, 0);
    if (received > 0)
    {
        payload[received] = '\0';
        PRINTF("[i] Received provisioning payload: %s\r\n", payload);
    }
    closesocket(clientFd);
    closesocket(serverFd);

    if (!wifi_parse_key_value(payload, ssid, sizeof(ssid), password, sizeof(password)))
    {
        PRINTF("[!] Invalid provisioning payload received.\r\n");
        WPL_Stop_AP();
        return;
    }

    if (!wifi_credentials_store(ssid, password))
    {
        PRINTF("[!] Failed to store provisioning credentials.\r\n");
        WPL_Stop_AP();
        return;
    }

    PRINTF("[i] Provisioning credentials stored. SSID=%s\r\n", ssid);
    WPL_Stop_AP();

    NVIC_SystemReset();
}
