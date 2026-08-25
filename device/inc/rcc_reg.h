/**
 * @file rcc_reg.h
 * @brief RCC (Reset and Clock Control) peripheral register definitions for
 *        the STM32F446xx family.
 *
 * Register layout and field values derived from RM0390 (STM32F446xx
 * reference manual), section 6: Reset and clock control (RCC).
 */
#ifndef RCC_REG_H
#define RCC_REG_H

#include <stdint.h>

/**
 * @addtogroup device_peripherals
 * @{
 */

#define RCC_BASE (0x40023800UL) ///< RCC peripheral base address

/**
 * @brief RCC peripheral register map (RM0390 section 6.3).
 */
typedef struct RccRegisters_t
{
  volatile uint32_t CR;           ///< 0x00: clock control register
  volatile uint32_t PLLCFGR;      ///< 0x04: PLL configuration register
  volatile uint32_t CFGR;         ///< 0x08: clock configuration register
  volatile uint32_t CIR;          ///< 0x0C: clock interrupt register
  volatile uint32_t AHB1RSTR;     ///< 0x10: AHB1 peripheral reset register
  volatile uint32_t AHB2RSTR;     ///< 0x14: AHB2 peripheral reset register
  volatile uint32_t AHB3RSTR;     ///< 0x18: AHB3 peripheral reset register
  volatile uint32_t RESERVED0;    ///< 0x1C: reserved
  volatile uint32_t APB1RSTR;     ///< 0x20: APB1 peripheral reset register
  volatile uint32_t APB2RSTR;     ///< 0x24: APB2 peripheral reset register
  volatile uint32_t RESERVED1[2]; ///< 0x28-0x2C: reserved
  volatile uint32_t AHB1ENR;      ///< 0x30: AHB1 peripheral clock enable register
  volatile uint32_t AHB2ENR;      ///< 0x34: AHB2 peripheral clock enable register
  volatile uint32_t AHB3ENR;      ///< 0x38: AHB3 peripheral clock enable register
  volatile uint32_t RESERVED2;    ///< 0x3C: reserved
  volatile uint32_t APB1ENR;      ///< 0x40: APB1 peripheral clock enable register
  volatile uint32_t APB2ENR;      ///< 0x44: APB2 peripheral clock enable register
  volatile uint32_t RESERVED3[2]; ///< 0x48-0x4C: reserved
  volatile uint32_t AHB1LPENR;    ///< 0x50: AHB1 peripheral clock enable in LP mode reg
  volatile uint32_t AHB2LPENR;    ///< 0x54: AHB2 peripheral clock enable in LP mode reg
  volatile uint32_t AHB3LPENR;    ///< 0x58: AHB3 peripheral clock enable in LP mode reg
  volatile uint32_t RESERVED4;    ///< 0x5C: reserved
  volatile uint32_t APB1LPENR;    ///< 0x60: APB1 peripheral clock enable in LP mode reg
  volatile uint32_t APB2LPENR;    ///< 0x64: APB2 peripheral clock enabled in LP mode reg
  volatile uint32_t RESERVED5[2]; ///< 0x68-0x6C: reserved
  volatile uint32_t BDCR;         ///< 0x70: Backup domain control register
  volatile uint32_t CSR;          ///< 0x74: clock control & status register
  volatile uint32_t RESERVED6[2]; ///< 0x78-0x7C: reserved
  volatile uint32_t SSCGR;        ///< 0x80: spread spectrum clock generation register
  volatile uint32_t PLLI2SCFGR;   ///< 0x84: PLLI2S configuration register
  volatile uint32_t PLLSAICFGR;   ///< 0x88: PLLSAI configuration register
  volatile uint32_t DCKCFGR;      ///< 0x8C: dedicated clock configuration register
  volatile uint32_t CKGATENR;     ///< 0x90: clocks gated enable register
  volatile uint32_t DCKCFGR2;     ///< 0x94: dedicated clocks configuration register 2
} RccRegisters_t;

#define RCC ((RccRegisters_t *)RCC_BASE) ///< Pointer to the RCC register block

/* --- RCC_CR bit definitions --- */
#define RCC_CR_HSION_Pos (0U)                   ///< Bit position within RCC_CR
#define RCC_CR_HSION (1U << RCC_CR_HSION_Pos)   ///< WRITE
#define RCC_CR_HSIRDY_Pos (1U)                  ///< Bit position within RCC_CR
#define RCC_CR_HSIRDY (1U << RCC_CR_HSIRDY_Pos) ///< READ only
#define RCC_CR_HSEON_Pos (16U)                  ///< Bit position within RCC_CR
#define RCC_CR_HSEON (1U << RCC_CR_HSEON_Pos)   ///< WRITE
#define RCC_CR_HSERDY_Pos (17U)                 ///< Bit position within RCC_CR
#define RCC_CR_HSERDY (1U << RCC_CR_HSERDY_Pos) ///< READ only
#define RCC_CR_HSEBYP_Pos (18U)                 ///< Bit position within RCC_CR
#define RCC_CR_HSEBYP (1U << RCC_CR_HSEBYP_Pos) ///< WRITE, set before HSEON
#define RCC_CR_CSSON_Pos (19U)                  ///< Bit position within RCC_CR
#define RCC_CR_CSSON (1U << RCC_CR_CSSON_Pos)   ///< WRITE, clock security system on HSE
#define RCC_CR_PLLON_Pos (24U)                  ///< Bit position within RCC_CR
#define RCC_CR_PLLON (1U << RCC_CR_PLLON_Pos)   ///< WRITE, after PLLCFGR is set
#define RCC_CR_PLLRDY_Pos (25U)                 ///< Bit position within RCC_CR
#define RCC_CR_PLLRDY (1U << RCC_CR_PLLRDY_Pos) ///< READ only

/* --- RCC_CIR bit definitions --- */
#define RCC_CIR_CSSF_Pos (7U)                 ///< Bit position within RCC_CIR
#define RCC_CIR_CSSF (1U << RCC_CIR_CSSF_Pos) ///< READ only - clock security system failure flag
#define RCC_CIR_CSSC_Pos (23U)                ///< Bit position within RCC_CIR
#define RCC_CIR_CSSC (1U << RCC_CIR_CSSC_Pos) ///< WRITE 1 to clear CSSF

/* --- RCC_PLLCFGR bit definitions --- */
#define RCC_PLLCFGR_PLLM_Pos (0U)                            ///< Bit position within RCC_PLLCFGR
#define RCC_PLLCFGR_PLLM_Msk (0x3FU << RCC_PLLCFGR_PLLM_Pos) ///< bits 5:0

#define RCC_PLLCFGR_PLLN_Pos (6U)                             ///< Bit position within RCC_PLLCFGR
#define RCC_PLLCFGR_PLLN_Msk (0x1FFU << RCC_PLLCFGR_PLLN_Pos) ///< bits 14:6

#define RCC_PLLCFGR_PLLP_Pos (16U)                          ///< Bit position within RCC_PLLCFGR
#define RCC_PLLCFGR_PLLP_Msk (0x3U << RCC_PLLCFGR_PLLP_Pos) ///< bits 17:16

#define RCC_PLLCFGR_PLLSRC_Pos (22U)                      ///< Bit position within RCC_PLLCFGR
#define RCC_PLLCFGR_PLLSRC (1U << RCC_PLLCFGR_PLLSRC_Pos) ///< 0=HSI, 1=HSE

/* --- RCC_CFGR bit definitions --- */
#define RCC_CFGR_SW_Pos (0U)                      ///< Bit position within RCC_CFGR
#define RCC_CFGR_SW_Msk (0x3U << RCC_CFGR_SW_Pos) ///< bits 1:0, system clock switch

#define RCC_CFGR_SWS_Pos (2U)                       ///< Bit position within RCC_CFGR
#define RCC_CFGR_SWS_Msk (0x3U << RCC_CFGR_SWS_Pos) ///< bits 3:2, system clock switch status

#define RCC_CFGR_HPRE_Pos (4U)                        ///< Bit position within RCC_CFGR
#define RCC_CFGR_HPRE_Msk (0xFU << RCC_CFGR_HPRE_Pos) ///< bits 7:4, AHB prescaler

#define RCC_CFGR_PPRE1_Pos (10U)                        ///< Bit position within RCC_CFGR
#define RCC_CFGR_PPRE1_Msk (0x7U << RCC_CFGR_PPRE1_Pos) ///< bits 12:10, APB1 prescaler

#define RCC_CFGR_PPRE2_Pos (13U)                        ///< Bit position within RCC_CFGR
#define RCC_CFGR_PPRE2_Msk (0x7U << RCC_CFGR_PPRE2_Pos) ///< bits 15:13, APB2 prescaler

/* --- RCC_AHB1RSTR bit definitions --- */
#define RCC_AHB1RSTR_GPIOARST_Pos (0U) ///< Bit position within RCC_AHB1RSTR
#define RCC_AHB1RSTR_GPIOARST (1U << RCC_AHB1RSTR_GPIOARST_Pos) ///< Bit within RCC_AHB1RSTR
#define RCC_AHB1RSTR_GPIOBRST_Pos (1U) ///< Bit position within RCC_AHB1RSTR
#define RCC_AHB1RSTR_GPIOBRST (1U << RCC_AHB1RSTR_GPIOBRST_Pos) ///< Bit within RCC_AHB1RSTR
#define RCC_AHB1RSTR_GPIOCRST_Pos (2U) ///< Bit position within RCC_AHB1RSTR
#define RCC_AHB1RSTR_GPIOCRST (1U << RCC_AHB1RSTR_GPIOCRST_Pos) ///< Bit within RCC_AHB1RSTR
#define RCC_AHB1RSTR_GPIODRST_Pos (3U) ///< Bit position within RCC_AHB1RSTR
#define RCC_AHB1RSTR_GPIODRST (1U << RCC_AHB1RSTR_GPIODRST_Pos) ///< Bit within RCC_AHB1RSTR
#define RCC_AHB1RSTR_GPIOERST_Pos (4U) ///< Bit position within RCC_AHB1RSTR
#define RCC_AHB1RSTR_GPIOERST (1U << RCC_AHB1RSTR_GPIOERST_Pos) ///< Bit within RCC_AHB1RSTR
#define RCC_AHB1RSTR_GPIOFRST_Pos (5U) ///< Bit position within RCC_AHB1RSTR
#define RCC_AHB1RSTR_GPIOFRST (1U << RCC_AHB1RSTR_GPIOFRST_Pos) ///< Bit within RCC_AHB1RSTR
#define RCC_AHB1RSTR_GPIOGRST_Pos (6U) ///< Bit position within RCC_AHB1RSTR
#define RCC_AHB1RSTR_GPIOGRST (1U << RCC_AHB1RSTR_GPIOGRST_Pos) ///< Bit within RCC_AHB1RSTR
#define RCC_AHB1RSTR_GPIOHRST_Pos (7U) ///< Bit position within RCC_AHB1RSTR
#define RCC_AHB1RSTR_GPIOHRST (1U << RCC_AHB1RSTR_GPIOHRST_Pos) ///< Bit within RCC_AHB1RSTR

/* --- RCC_AHB1ENR bit definitions --- */
#define RCC_AHB1ENR_GPIOAEN_Pos (0U)                        ///< Bit position within RCC_AHB1ENR
#define RCC_AHB1ENR_GPIOAEN (1U << RCC_AHB1ENR_GPIOAEN_Pos) ///< Bit within RCC_AHB1ENR
#define RCC_AHB1ENR_GPIOBEN_Pos (1U)                        ///< Bit position within RCC_AHB1ENR
#define RCC_AHB1ENR_GPIOBEN (1U << RCC_AHB1ENR_GPIOBEN_Pos) ///< Bit within RCC_AHB1ENR
#define RCC_AHB1ENR_GPIOCEN_Pos (2U)                        ///< Bit position within RCC_AHB1ENR
#define RCC_AHB1ENR_GPIOCEN (1U << RCC_AHB1ENR_GPIOCEN_Pos) ///< Bit within RCC_AHB1ENR
#define RCC_AHB1ENR_GPIODEN_Pos (3U)                        ///< Bit position within RCC_AHB1ENR
#define RCC_AHB1ENR_GPIODEN (1U << RCC_AHB1ENR_GPIODEN_Pos) ///< Bit within RCC_AHB1ENR
#define RCC_AHB1ENR_GPIOEEN_Pos (4U)                        ///< Bit position within RCC_AHB1ENR
#define RCC_AHB1ENR_GPIOEEN (1U << RCC_AHB1ENR_GPIOEEN_Pos) ///< Bit within RCC_AHB1ENR
#define RCC_AHB1ENR_GPIOFEN_Pos (5U)                        ///< Bit position within RCC_AHB1ENR
#define RCC_AHB1ENR_GPIOFEN (1U << RCC_AHB1ENR_GPIOFEN_Pos) ///< Bit within RCC_AHB1ENR
#define RCC_AHB1ENR_GPIOGEN_Pos (6U)                        ///< Bit position within RCC_AHB1ENR
#define RCC_AHB1ENR_GPIOGEN (1U << RCC_AHB1ENR_GPIOGEN_Pos) ///< Bit within RCC_AHB1ENR
#define RCC_AHB1ENR_GPIOHEN_Pos (7U)                        ///< Bit position within RCC_AHB1ENR
#define RCC_AHB1ENR_GPIOHEN (1U << RCC_AHB1ENR_GPIOHEN_Pos) ///< Bit within RCC_AHB1ENR

/* --- RCC_APB1RSTR bit definitions --- */
#define RCC_APB1RSTR_USART2RST_Pos (17U) ///< Bit position within RCC_APB1RSTR
#define RCC_APB1RSTR_USART2RST (1U << RCC_APB1RSTR_USART2RST_Pos) ///< Bit within RCC_APB1RSTR

/* --- RCC_APB1ENR bit definitions --- */
#define RCC_APB1ENR_USART2EN_Pos (17U)                        ///< Bit position within RCC_APB1ENR
#define RCC_APB1ENR_USART2EN (1U << RCC_APB1ENR_USART2EN_Pos) ///< Bit within RCC_APB1ENR

/* --- RCC_CSR bit definitions --- */
#define RCC_CSR_LSION_Pos (0U)                    ///< Bit position within RCC_CSR
#define RCC_CSR_LSION (1U << RCC_CSR_LSION_Pos)   ///< WRITE
#define RCC_CSR_LSIRDY_Pos (1U)                   ///< Bit position within RCC_CSR
#define RCC_CSR_LSIRDY (1U << RCC_CSR_LSIRDY_Pos) ///< READ only

#define RCC_CSR_RMVF_Pos (24U)                        ///< Bit position within RCC_CSR
#define RCC_CSR_RMVF (1U << RCC_CSR_RMVF_Pos)         ///< WRITE 1 to clear all reset flags below
#define RCC_CSR_BORRSTF_Pos (25U)                     ///< Bit position within RCC_CSR
#define RCC_CSR_BORRSTF (1U << RCC_CSR_BORRSTF_Pos)   ///< READ only - brown-out reset flag
#define RCC_CSR_PINRSTF_Pos (26U)                     ///< Bit position within RCC_CSR
#define RCC_CSR_PINRSTF (1U << RCC_CSR_PINRSTF_Pos)   ///< READ only - NRST pin reset flag
#define RCC_CSR_PORRSTF_Pos (27U)                     ///< Bit position within RCC_CSR
#define RCC_CSR_PORRSTF (1U << RCC_CSR_PORRSTF_Pos)   ///< READ only - power-on reset flag
#define RCC_CSR_SFTRSTF_Pos (28U)                     ///< Bit position within RCC_CSR
#define RCC_CSR_SFTRSTF (1U << RCC_CSR_SFTRSTF_Pos)   ///< READ only - software reset flag
#define RCC_CSR_IWDGRSTF_Pos (29U)                    ///< Bit position within RCC_CSR
#define RCC_CSR_IWDGRSTF (1U << RCC_CSR_IWDGRSTF_Pos) ///< READ only - IWDG reset flag
#define RCC_CSR_WWDGRSTF_Pos (30U)                    ///< Bit position within RCC_CSR
#define RCC_CSR_WWDGRSTF (1U << RCC_CSR_WWDGRSTF_Pos) ///< READ only - window watchdog reset flag
#define RCC_CSR_LPWRRSTF_Pos (31U)                    ///< Bit position within RCC_CSR
#define RCC_CSR_LPWRRSTF (1U << RCC_CSR_LPWRRSTF_Pos) ///< READ only - low-power reset flag

/** @} */

#endif /* RCC_REG_H */