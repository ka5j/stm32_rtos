/**
 * @file nvic_reg.h
 * @brief NVIC (Nested Vectored Interrupt Controller) register definitions
 *        for the ARM Cortex-M4 (Armv7-M architecture).
 *
 * Register layout derived from PM0214 (Cortex-M4 programming manual),
 * section on the NVIC. This is architectural rather than vendor-specific;
 * the base address and register offsets are fixed by Armv7-M's System
 * Control Space memory map, not by the STM32F446. The ISER/ICER/ISPR/ICPR
 * and IABR blocks are architecturally spaced 32 words apart regardless
 * of how many interrupts a given chip implements, so the RESERVED
 * members below are not padding for alignment; they are the actual gap
 * between register banks.
 *
 * The STM32F446 implements interrupt lines 0-96 (WWDG..FMPI2C1_Error, in
 * RM0390 vector-table order; see IRQn_e below), so only ISER0-3 /
 * ICER0-3 / ISPR0-3 / ICPR0-3 / IABR0-3 and IP[0..96] are meaningful on
 * this chip; the remainder of each 8-word bank reads as unimplemented.
 *
 * Only the top 4 bits of each IP[] priority byte are implemented on the
 * Cortex-M4 (PM0214: __NVIC_PRIO_BITS = 4), giving 16 priority levels
 * with values 0x00, 0x10, 0x20, ... 0xF0. Writing a priority value with
 * low-nibble bits set does not fault, but those bits are not stored in
 * hardware; a driver setting priorities should shift into the top
 * nibble rather than assume all 8 bits are significant.
 */
#ifndef NVIC_REG_H
#define NVIC_REG_H

#include <stdint.h>

/**
 * @addtogroup core_peripherals
 * @{
 */

#define NVIC_BASE (0xE000E100UL) ///< NVIC peripheral base address (Armv7-M SCS)

/**
 * @brief NVIC register map (PM0214, Armv7-M SCS NVIC region).
 *
 * RESERVEDx members preserve the fixed architectural offsets between
 * register banks (see the top-of-file note above) - they are real gaps
 * in the memory map, not struct-packing artifacts.
 */
typedef struct NvicRegisters_t
{
  volatile uint32_t ISER[8];        ///< 0x000-0x01C: Interrupt Set-Enable Registers
  volatile uint32_t RESERVED0[24];  ///< 0x020-0x07C: reserved
  volatile uint32_t ICER[8];        ///< 0x080-0x09C: Interrupt Clear-Enable Registers
  volatile uint32_t RESERVED1[24];  ///< 0x0A0-0x0FC: reserved
  volatile uint32_t ISPR[8];        ///< 0x100-0x11C: Interrupt Set-Pending Registers
  volatile uint32_t RESERVED2[24];  ///< 0x120-0x17C: reserved
  volatile uint32_t ICPR[8];        ///< 0x180-0x19C: Interrupt Clear-Pending Registers
  volatile uint32_t RESERVED3[24];  ///< 0x1A0-0x1FC: reserved
  volatile uint32_t IABR[8];        ///< 0x200-0x21C: Interrupt Active Bit Registers (READ only)
  volatile uint32_t RESERVED4[56];  ///< 0x220-0x2FC: reserved
  volatile uint8_t IP[240];         ///< 0x300-0x3EF: Interrupt Priority Registers, byte-addressable
  volatile uint32_t RESERVED5[644]; ///< 0x3F0-0xDFC: reserved
  volatile uint32_t STIR;           ///< 0xE00: Software Trigger Interrupt Register (WRITE only)
} NvicRegisters_t;

#define NVIC ((NvicRegisters_t *)NVIC_BASE) ///< Pointer to the NVIC register block

/**
 * @brief STM32F446 interrupt line numbers (RM0390 vector-table order).
 *
 * Matches the position of each *_IRQHandler entry in
 * startup/startup_stm32f446re.s. Used to index NVIC->ISER/ICER/ISPR/
 * ICPR/IABR (word = IRQn / 32, bit = IRQn % 32) and NVIC->IP (byte
 * index = IRQn directly). Gaps (61-62, 79-80, 82-83, 85-86, 88-90) are
 * reserved positions in the vector table and intentionally absent here.
 */
typedef enum IRQn_e
{
  WWDG_IRQn = 0,
  PVD_IRQn = 1,
  TAMP_STAMP_IRQn = 2,
  RTC_WKUP_IRQn = 3,
  FLASH_IRQn = 4,
  RCC_IRQn = 5,
  EXTI0_IRQn = 6,
  EXTI1_IRQn = 7,
  EXTI2_IRQn = 8,
  EXTI3_IRQn = 9,
  EXTI4_IRQn = 10,
  DMA1_Stream0_IRQn = 11,
  DMA1_Stream1_IRQn = 12,
  DMA1_Stream2_IRQn = 13,
  DMA1_Stream3_IRQn = 14,
  DMA1_Stream4_IRQn = 15,
  DMA1_Stream5_IRQn = 16,
  DMA1_Stream6_IRQn = 17,
  ADC_IRQn = 18,
  CAN1_TX_IRQn = 19,
  CAN1_RX0_IRQn = 20,
  CAN1_RX1_IRQn = 21,
  CAN1_SCE_IRQn = 22,
  EXTI9_5_IRQn = 23,
  TIM1_BRK_TIM9_IRQn = 24,
  TIM1_UP_TIM10_IRQn = 25,
  TIM1_TRG_COM_TIM11_IRQn = 26,
  TIM1_CC_IRQn = 27,
  TIM2_IRQn = 28,
  TIM3_IRQn = 29,
  TIM4_IRQn = 30,
  I2C1_EV_IRQn = 31,
  I2C1_ER_IRQn = 32,
  I2C2_EV_IRQn = 33,
  I2C2_ER_IRQn = 34,
  SPI1_IRQn = 35,
  SPI2_IRQn = 36,
  USART1_IRQn = 37,
  USART2_IRQn = 38,
  USART3_IRQn = 39,
  EXTI15_10_IRQn = 40,
  RTC_Alarm_IRQn = 41,
  OTG_FS_WKUP_IRQn = 42,
  TIM8_BRK_TIM12_IRQn = 43,
  TIM8_UP_TIM13_IRQn = 44,
  TIM8_TRG_COM_TIM14_IRQn = 45,
  TIM8_CC_IRQn = 46,
  DMA1_Stream7_IRQn = 47,
  FMC_IRQn = 48,
  SDIO_IRQn = 49,
  TIM5_IRQn = 50,
  SPI3_IRQn = 51,
  UART4_IRQn = 52,
  UART5_IRQn = 53,
  TIM6_DAC_IRQn = 54,
  TIM7_IRQn = 55,
  DMA2_Stream0_IRQn = 56,
  DMA2_Stream1_IRQn = 57,
  DMA2_Stream2_IRQn = 58,
  DMA2_Stream3_IRQn = 59,
  DMA2_Stream4_IRQn = 60,
  CAN2_TX_IRQn = 63,
  CAN2_RX0_IRQn = 64,
  CAN2_RX1_IRQn = 65,
  CAN2_SCE_IRQn = 66,
  OTG_FS_IRQn = 67,
  DMA2_Stream5_IRQn = 68,
  DMA2_Stream6_IRQn = 69,
  DMA2_Stream7_IRQn = 70,
  USART6_IRQn = 71,
  I2C3_EV_IRQn = 72,
  I2C3_ER_IRQn = 73,
  OTG_HS_EP1_OUT_IRQn = 74,
  OTG_HS_EP1_IN_IRQn = 75,
  OTG_HS_WKUP_IRQn = 76,
  OTG_HS_IRQn = 77,
  DCMI_IRQn = 78,
  FPU_IRQn = 81,
  SPI4_IRQn = 84,
  SAI1_IRQn = 87,
  SAI2_IRQn = 91,
  QUADSPI_IRQn = 92,
  CEC_IRQn = 93,
  SPDIF_RX_IRQn = 94,
  FMPI2C1_Event_IRQn = 95,
  FMPI2C1_Error_IRQn = 96,
} IRQn_e;

#define NVIC_PRIO_BITS (4U) ///< Implemented priority bits per IP[] entry (top nibble only)
#define NVIC_PRIO_Pos (8U - NVIC_PRIO_BITS) ///< Shift a 0-15 priority into IP[]'s top nibble

/** @} */

#endif /* NVIC_REG_H */
