if(__UFW_STM32HAL_l0)
  return()
endif()
set(__UFW_STM32HAL_l0 1)

set(__UFW_STM32HAL_l0_HIGH_SOURCES
  stm32l0xx_hal.c
  stm32l0xx_hal_adc.c
  stm32l0xx_hal_adc_ex.c
  stm32l0xx_hal_comp.c
  stm32l0xx_hal_comp_ex.c
  stm32l0xx_hal_cortex.c
  stm32l0xx_hal_crc.c
  stm32l0xx_hal_crc_ex.c
  stm32l0xx_hal_cryp.c
  stm32l0xx_hal_cryp_ex.c
  stm32l0xx_hal_dac.c
  stm32l0xx_hal_dac_ex.c
  stm32l0xx_hal_dma.c
  stm32l0xx_hal_firewall.c
  stm32l0xx_hal_flash.c
  stm32l0xx_hal_flash_ex.c
  stm32l0xx_hal_flash_ramfunc.c
  stm32l0xx_hal_gpio.c
  stm32l0xx_hal_i2c.c
  stm32l0xx_hal_i2c_ex.c
  stm32l0xx_hal_i2s.c
  stm32l0xx_hal_irda.c
  stm32l0xx_hal_iwdg.c
  stm32l0xx_hal_lcd.c
  stm32l0xx_hal_lptim.c
  stm32l0xx_hal_pcd.c
  stm32l0xx_hal_pcd_ex.c
  stm32l0xx_hal_pwr.c
  stm32l0xx_hal_pwr_ex.c
  stm32l0xx_hal_rcc.c
  stm32l0xx_hal_rcc_ex.c
  stm32l0xx_hal_rng.c
  stm32l0xx_hal_rtc.c
  stm32l0xx_hal_rtc_ex.c
  stm32l0xx_hal_smartcard.c
  stm32l0xx_hal_smartcard_ex.c
  stm32l0xx_hal_smbus.c
  stm32l0xx_hal_spi.c
  stm32l0xx_hal_tim.c
  stm32l0xx_hal_tim_ex.c
  stm32l0xx_hal_tsc.c
  stm32l0xx_hal_uart.c
  stm32l0xx_hal_uart_ex.c
  stm32l0xx_hal_usart.c
  stm32l0xx_hal_wwdg.c)

set(__UFW_STM32HAL_l0_LOW_SOURCES
  stm32l0xx_ll_adc.c
  stm32l0xx_ll_comp.c
  stm32l0xx_ll_crc.c
  stm32l0xx_ll_crs.c
  stm32l0xx_ll_dac.c
  stm32l0xx_ll_dma.c
  stm32l0xx_ll_exti.c
  stm32l0xx_ll_gpio.c
  stm32l0xx_ll_i2c.c
  stm32l0xx_ll_lptim.c
  stm32l0xx_ll_lpuart.c
  stm32l0xx_ll_pwr.c
  stm32l0xx_ll_rcc.c
  stm32l0xx_ll_rng.c
  stm32l0xx_ll_rtc.c
  stm32l0xx_ll_spi.c
  stm32l0xx_ll_tim.c
  stm32l0xx_ll_usart.c
  stm32l0xx_ll_usb.c
  stm32l0xx_ll_utils.c)