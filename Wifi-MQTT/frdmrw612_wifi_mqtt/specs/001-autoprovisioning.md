It is required to implement a auto-provisioning mechanism for the wifi_mqtt.c file.

# Device Boot procedure
When the device starts it should verify if connection parameters exists in non-volatile memory (predefined location):
- If there are connection parameters, it should try to connect automatically.
- If there are no connection parameters saved, it should start the [Provisioning Mode](#provisioningmode).
- If there are connection parameters but the connection fails, it should change to the [Provisioning Mode](#provisioningmode).

The connection parameters to be saved in non-volatile memory should contain:
- SSID
- Password

# ProvisioningMode
The provisioning mode should be executed following these rules:
1. It should start announcing the provisioning service using mDNS, allowing for services using DNS-SD in the local network to discover it.
2. It should start the Access Point mode to allow a client to connect.
3. It should start a TCP Server in port 15000, waiting to receive the parameters.
4. Once it receives a package with SSID and Password, it should store them in non-volatile memory.
5. Once SSID and Password is stored the device should be software restarted so in it connects with the new configuration.