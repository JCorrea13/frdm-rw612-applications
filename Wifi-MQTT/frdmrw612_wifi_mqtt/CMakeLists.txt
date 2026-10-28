
# Copyright 2026 NXP
#
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.22.0)

cmake_path(APPEND conf_file_path ${CMAKE_CURRENT_LIST_DIR} ${board} prj.conf)
set(CONF_FILE ${conf_file_path} ${CONF_FILE})
include(${SdkRootDirPath}/cmake/extension/mcux.cmake)

project(frdmrw612_wifi_mqtt LANGUAGES C CXX ASM PROJECT_BOARD_PORT_PATH ${board_root}/${board}/wifi_examples/wifi_mqtt)

include(${SdkRootDirPath}/CMakeLists.txt)


include(${board}/board_files.cmake)

mcux_add_include(
    INCLUDES mqtt_common
)

mcux_add_source(
    SOURCES mqtt_common/mqtt_freertos.c
            mqtt_common/mqtt_freertos.h
            __repo__/mcuxsdk/middleware/lwip/src/apps/mdns/mdns.c
            __repo__/mcuxsdk/middleware/lwip/src/apps/mdns/mdns_domain.c
            __repo__/mcuxsdk/middleware/lwip/src/apps/mdns/mdns_out.c
)

mcux_add_source(
    SOURCES FreeRTOSConfig.h
            common/pin_mux.c
            common/pin_mux.h
            frdmrw612/FreeRTOSConfigBoard.h
            lwipopts.h
            lwippools.h
            wifi_config/wifi_config.h
            wifi_mqtt.c
            wifi_provisioning.c
            wifi_provisioning.h
)

mcux_add_include(
    INCLUDES .
             common
             wifi_config
)
mcux_add_macro(
    CC "-DUSE_RTOS=1\
       -DPRINTF_ADVANCED_ENABLE=1"
)

mcux_add_iar_configuration(
	CC "--dlib_config full\
		--no_inline"
	CX "--no_clustering"
	LD "--semihosting"
)

mcux_add_armgcc_linker_script(
    BASE_PATH ${SdkRootDirPath}
    TARGETS debug release flexspi_nor_debug flexspi_nor_release flash_debug flash_release
    LINKER ${board_root}/${board}/wifi_examples/common/linker/${CONFIG_MCUX_TOOLCHAIN_LINKER_DEVICE_PREFIX}_wifi.ld
)

mcux_add_iar_linker_script(
    BASE_PATH ${SdkRootDirPath}
    TARGETS debug release flexspi_nor_debug flexspi_nor_release flash_debug flash_release
    LINKER ${board_root}/${board}/wifi_examples/common/linker/${CONFIG_MCUX_TOOLCHAIN_LINKER_DEVICE_PREFIX}_wifi.icf
)

mcux_add_mdk_linker_script(
    BASE_PATH ${SdkRootDirPath}
    TARGETS debug release flexspi_nor_debug flexspi_nor_release flash_debug flash_release
    LINKER ${board_root}/${board}/wifi_examples/common/linker/${CONFIG_MCUX_TOOLCHAIN_LINKER_DEVICE_PREFIX}_wifi.scf
)


mcux_add_source(
    SOURCES common/app.h
            common/hardware_init.c
)

mcux_add_include(
    INCLUDES common
)
mcux_add_mdk_configuration(
    TARGETS flash_debug flash_release
    LD "--no_remove"
)

mcux_remove_mdk_configuration(
    TARGETS flash_debug flash_release
    LD "--remove"
)

mcux_add_iar_configuration(
    CX "--no_clustering"
    TARGETS flash_release
    CC "--no_inline"
)

mcux_remove_armgcc_linker_script(
    BASE_PATH ${SdkRootDirPath}
    TARGETS debug release flexspi_nor_debug flexspi_nor_release flash_debug flash_release
    LINKER ${board_root}/${board}/wifi_examples/common/linker/RW612_wifi.ld
)

mcux_remove_iar_linker_script(
    BASE_PATH ${SdkRootDirPath}
    TARGETS debug release flexspi_nor_debug flexspi_nor_release flash_debug flash_release
    LINKER ${board_root}/${board}/wifi_examples/common/linker/RW612_wifi.icf
)

mcux_remove_mdk_linker_script(
    BASE_PATH ${SdkRootDirPath}
    TARGETS debug release flexspi_nor_debug flexspi_nor_release flash_debug flash_release
    LINKER ${board_root}/${board}/wifi_examples/common/linker/RW612_wifi.scf
)


mcux_add_armgcc_linker_script(
    LINKER linker_files/RW610_flash.ld
    TARGETS flash_debug flash_release
)

mcux_add_iar_linker_script(
    LINKER linker_files/RW610_flash.icf
    TARGETS flash_debug flash_release
)

mcux_add_mdk_linker_script(
    LINKER linker_files/RW610_flash.scf
    TARGETS flash_debug flash_release
)
mcux_remove_armgcc_linker_script(
    BASE_PATH ${SdkRootDirPath}
    TARGETS flash_debug flash_release
    LINKER ${device_root}/${soc_portfolio}/${soc_series}/${device}/gcc/${CONFIG_MCUX_TOOLCHAIN_LINKER_DEVICE_PREFIX}_flash.ld
)

mcux_remove_iar_linker_script(
    BASE_PATH ${SdkRootDirPath}
    TARGETS flash_debug flash_release
    LINKER ${device_root}/${soc_portfolio}/${soc_series}/${device}/iar/${CONFIG_MCUX_TOOLCHAIN_LINKER_DEVICE_PREFIX}_flash.icf
)

mcux_remove_mdk_linker_script(
    BASE_PATH ${SdkRootDirPath}
    TARGETS flash_debug flash_release
    LINKER ${device_root}/${soc_portfolio}/${soc_series}/${device}/arm/${CONFIG_MCUX_TOOLCHAIN_LINKER_DEVICE_PREFIX}_flash.scf
)
mcux_convert_binary(BINARY ${APPLICATION_BINARY_DIR}/${MCUX_SDK_PROJECT_NAME}.bin)

mcux_add_include(INCLUDES frdmrw612)
