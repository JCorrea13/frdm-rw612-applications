#ifndef WIFI_PROVISIONING_H
#define WIFI_PROVISIONING_H

#include <stdbool.h>
#include <stddef.h>

#define WIFI_PROVISIONING_SSID "RW612_Provisioning_Equipo3"
#define WIFI_PROVISIONING_PASSWORD "12345678"
#define WIFI_PROVISIONING_PORT 15000

bool wifi_credentials_loaded(char *ssid, size_t ssidLength, char *password, size_t passwordLength);
bool wifi_credentials_store(const char *ssid, const char *password);
bool wifi_credentials_clear(void);
void wifi_enter_provisioning_mode(void);

#endif
