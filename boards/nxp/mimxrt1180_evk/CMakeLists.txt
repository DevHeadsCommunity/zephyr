#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()
zephyr_library_sources(board.c)

if(CONFIG_NXP_IMXRT_BOOT_HEADER)
  if(NOT ((DEFINED CONFIG_BOARD_MIMXRT1180_EVK_MIMXRT1189_CM33)
	  OR (DEFINED CONFIG_BOARD_MIMXRT1180_EVK_MIMXRT1189_CM7)))
    message(WARNING "It appears you are using the board definition for "
     "the MIMXRT1180-EVK, but targeting a custom board. You may need to "
     "update your flash configuration or device configuration data blocks")
  endif()
  set(RT1180_BOARD_DIR
    "${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk-ng/boards/evkmimxrt1180")
  if(CONFIG_BOOT_FLEXSPI_NOR)
    # Include flash configuration block for RT1180 EVK from NXP's HAL.
    # This configuration block may need modification if another flash chip is
    # used on your custom board.
    zephyr_compile_definitions(XIP_EXTERNAL_FLASH=1)
    zephyr_compile_definitions(XIP_BOOT_HEADER_ENABLE=1)
    zephyr_library_sources(${RT1180_BOARD_DIR}/xip/evkmimxrt1180_flexspi_nor_config.c)
    zephyr_library_include_directories(${RT1180_BOARD_DIR}/xip)
    zephyr_library_include_directories(${RT1180_BOARD_DIR})
  endif()
  if(CONFIG_EXTERNAL_MEM_CONFIG_DATA AND CONFIG_NXP_IMX_EXTERNAL_HYPERRAM)
     zephyr_compile_definitions(USE_HYPERRAM)
     zephyr_library_sources(${RT1180_BOARD_DIR}/xip/evkmimxrt1180_flexspi_nor_config.c)
  endif()
endif()
