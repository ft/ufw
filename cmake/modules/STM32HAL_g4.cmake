# Copyright (c) 2020-2025 ufw workers, All rights reserved.
#
# Terms for redistribution and use can be found in LICENCE.

if(__UFW_STM32HAL_g4)
  return()
endif()
set(__UFW_STM32HAL_g4 1)

set(__UFW_STM32HAL_g4_HIGH_SOURCES
  stm32g4xx_hal.c
  stm32g4xx_hal_adc.c
  stm32g4xx_hal_adc_ex.c
  stm32g4xx_hal_comp.c
  stm32g4xx_hal_cordic.c
  stm32g4xx_hal_cortex.c
  stm32g4xx_hal_crc.c
  stm32g4xx_hal_crc_ex.c
  stm32g4xx_hal_cryp.c
  stm32g4xx_hal_cryp_ex.c
  stm32g4xx_hal_dac.c
  stm32g4xx_hal_dac_ex.c
  stm32g4xx_hal_dma.c
  stm32g4xx_hal_dma_ex.c
  stm32g4xx_hal_exti.c
  stm32g4xx_hal_fdcan.c
  stm32g4xx_hal_flash.c
  stm32g4xx_hal_flash_ex.c
  stm32g4xx_hal_flash_ramfunc.c
  stm32g4xx_hal_fmac.c
  stm32g4xx_hal_gpio.c
  stm32g4xx_hal_hrtim.c
  stm32g4xx_hal_i2c.c
  stm32g4xx_hal_i2c_ex.c
  stm32g4xx_hal_i2s.c
  stm32g4xx_hal_irda.c
  stm32g4xx_hal_iwdg.c
  stm32g4xx_hal_lptim.c
  stm32g4xx_hal_nand.c
  stm32g4xx_hal_nor.c
  stm32g4xx_hal_opamp.c
  stm32g4xx_hal_opamp_ex.c
  stm32g4xx_hal_pcd.c
  stm32g4xx_hal_pcd_ex.c
  stm32g4xx_hal_pwr.c
  stm32g4xx_hal_pwr_ex.c
  stm32g4xx_hal_qspi.c
  stm32g4xx_hal_rcc.c
  stm32g4xx_hal_rcc_ex.c
  stm32g4xx_hal_rng.c
  stm32g4xx_hal_rtc.c
  stm32g4xx_hal_rtc_ex.c
  stm32g4xx_hal_sai.c
  stm32g4xx_hal_sai_ex.c
  stm32g4xx_hal_smartcard.c
  stm32g4xx_hal_smartcard_ex.c
  stm32g4xx_hal_smbus.c
  stm32g4xx_hal_spi.c
  stm32g4xx_hal_spi_ex.c
  stm32g4xx_hal_sram.c
  stm32g4xx_hal_tim.c
  stm32g4xx_hal_tim_ex.c
  stm32g4xx_hal_uart.c
  stm32g4xx_hal_uart_ex.c
  stm32g4xx_hal_usart.c
  stm32g4xx_hal_usart_ex.c
  stm32g4xx_hal_wwdg.c)

set(__UFW_STM32HAL_g4_LOW_SOURCES
  stm32g4xx_ll_adc.c
  stm32g4xx_ll_comp.c
  stm32g4xx_ll_cordic.c
  stm32g4xx_ll_crc.c
  stm32g4xx_ll_crs.c
  stm32g4xx_ll_dac.c
  stm32g4xx_ll_dma.c
  stm32g4xx_ll_exti.c
  stm32g4xx_ll_fmac.c
  stm32g4xx_ll_fmc.c
  stm32g4xx_ll_gpio.c
  stm32g4xx_ll_hrtim.c
  stm32g4xx_ll_i2c.c
  stm32g4xx_ll_lptim.c
  stm32g4xx_ll_lpuart.c
  stm32g4xx_ll_opamp.c
  stm32g4xx_ll_pwr.c
  stm32g4xx_ll_rcc.c
  stm32g4xx_ll_rng.c
  stm32g4xx_ll_rtc.c
  stm32g4xx_ll_spi.c
  stm32g4xx_ll_tim.c
  stm32g4xx_ll_ucpd.c
  stm32g4xx_ll_usart.c
  stm32g4xx_ll_usb.c
  stm32g4xx_ll_utils.c)
