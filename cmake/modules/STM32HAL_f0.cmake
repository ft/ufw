# Copyright (c) 2020-2025 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

if(__UFW_STM32HAL_f0)
  return()
endif()
set(__UFW_STM32HAL_f0 1)

set(__UFW_STM32HAL_f0_HIGH_SOURCES
  stm32f0xx_hal.c
  stm32f0xx_hal_adc.c
  stm32f0xx_hal_adc_ex.c
  stm32f0xx_hal_can.c
  stm32f0xx_hal_cec.c
  stm32f0xx_hal_comp.c
  stm32f0xx_hal_cortex.c
  stm32f0xx_hal_crc.c
  stm32f0xx_hal_crc_ex.c
  stm32f0xx_hal_dac.c
  stm32f0xx_hal_dac_ex.c
  stm32f0xx_hal_dma.c
  stm32f0xx_hal_exti.c
  stm32f0xx_hal_flash.c
  stm32f0xx_hal_flash_ex.c
  stm32f0xx_hal_gpio.c
  stm32f0xx_hal_i2c.c
  stm32f0xx_hal_i2c_ex.c
  stm32f0xx_hal_i2s.c
  stm32f0xx_hal_irda.c
  stm32f0xx_hal_iwdg.c
  stm32f0xx_hal_pcd.c
  stm32f0xx_hal_pcd_ex.c
  stm32f0xx_hal_pwr.c
  stm32f0xx_hal_pwr_ex.c
  stm32f0xx_hal_rcc.c
  stm32f0xx_hal_rcc_ex.c
  stm32f0xx_hal_rtc.c
  stm32f0xx_hal_rtc_ex.c
  stm32f0xx_hal_smartcard.c
  stm32f0xx_hal_smartcard_ex.c
  stm32f0xx_hal_smbus.c
  stm32f0xx_hal_spi.c
  stm32f0xx_hal_spi_ex.c
  stm32f0xx_hal_tim.c
  stm32f0xx_hal_tim_ex.c
  stm32f0xx_hal_tsc.c
  stm32f0xx_hal_uart.c
  stm32f0xx_hal_uart_ex.c
  stm32f0xx_hal_usart.c
  stm32f0xx_hal_usart_ex.c
  stm32f0xx_hal_wwdg.c)

set(__UFW_STM32HAL_f0_LOW_SOURCES
  stm32f0xx_ll_adc.c
  stm32f0xx_ll_comp.c
  stm32f0xx_ll_crc.c
  stm32f0xx_ll_crs.c
  stm32f0xx_ll_dac.c
  stm32f0xx_ll_dma.c
  stm32f0xx_ll_exti.c
  stm32f0xx_ll_gpio.c
  stm32f0xx_ll_i2c.c
  stm32f0xx_ll_pwr.c
  stm32f0xx_ll_rcc.c
  stm32f0xx_ll_rtc.c
  stm32f0xx_ll_spi.c
  stm32f0xx_ll_tim.c
  stm32f0xx_ll_usart.c
  stm32f0xx_ll_usb.c
  stm32f0xx_ll_utils.c)

set(__UFW_STM32HAL_f0_LEGACY_SOURCES Legacy/stm32f0xx_hal_can.c)
