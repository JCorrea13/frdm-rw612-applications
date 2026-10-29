It is required to implement a mechanism to delete the wifi credentials stored in flash memory when a the following routine is executed.

In the case of the following conditions the wifi SSID and Password should be removed from the flash memory:

- When the application starts and the SW2 button is pressed

Considerations:
1. The button should not affect any other functionality for the SW2 button.
2. The application is already using SW2 to send MQTT messages.
3. If the "delete wifi credentials" routine is executed, the SSID and Password should be removed before ConnectToSavedNetwork method is called in wifi_mqtt.c