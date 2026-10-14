/**
 * @file rcc_reg.h
 * @brief RCC registers: oscillators, PLL, bus clocks, peripheral clock gating
 *        and reset flags (RM0390 section 6.3).
 *
 * Modelled: what HSI/HSE-to-PLL bring-up and GPIO/USART2/SYSCFG/PWR clock
 * gating need. Not modelled: PLLQ (the 48 MHz USB/SDIO output) and the CFGR
 * MCO, RTCPRE and I2SSRC fields.
 */
#ifndef RCC_REG_H
#define RCC_REG_H

#include <stdint.h>

/**
 * @addtogroup rcc_registers
 * @{
 */

#define RCC_BASE (0x40023800UL) ///< RCC base address (AHB1)

/** @brief RCC register block. */
typedef struct RccRegisters_t
{
    volatile uint32_t CR;           ///< 0x00: clock control
    volatile uint32_t PLLCFGR;      ///< 0x04: PLL configuration
    volatile uint32_t CFGR;         ///< 0x08: clock configuration
    volatile uint32_t CIR;          ///< 0x0C: clock interrupt
    volatile uint32_t AHB1RSTR;     ///< 0x10: AHB1 peripheral reset
    volatile uint32_t AHB2RSTR;     ///< 0x14: AHB2 peripheral reset
    volatile uint32_t AHB3RSTR;     ///< 0x18: AHB3 peripheral reset
    volatile uint32_t RESERVED0;    ///< 0x1C: reserved
    volatile uint32_t APB1RSTR;     ///< 0x20: APB1 peripheral reset
    volatile uint32_t APB2RSTR;     ///< 0x24: APB2 peripheral reset
    volatile uint32_t RESERVED1[2]; ///< 0x28-0x2C: reserved
    volatile uint32_t AHB1ENR;      ///< 0x30: AHB1 clock enable
    volatile uint32_t AHB2ENR;      ///< 0x34: AHB2 clock enable
    volatile uint32_t AHB3ENR;      ///< 0x38: AHB3 clock enable
    volatile uint32_t RESERVED2;    ///< 0x3C: reserved
    volatile uint32_t APB1ENR;      ///< 0x40: APB1 clock enable
    volatile uint32_t APB2ENR;      ///< 0x44: APB2 clock enable
    volatile uint32_t RESERVED3[2]; ///< 0x48-0x4C: reserved
    volatile uint32_t AHB1LPENR;    ///< 0x50: AHB1 clock enable in low-power mode
    volatile uint32_t AHB2LPENR;    ///< 0x54: AHB2 clock enable in low-power mode
    volatile uint32_t AHB3LPENR;    ///< 0x58: AHB3 clock enable in low-power mode
    volatile uint32_t RESERVED4;    ///< 0x5C: reserved
    volatile uint32_t APB1LPENR;    ///< 0x60: APB1 clock enable in low-power mode
    volatile uint32_t APB2LPENR;    ///< 0x64: APB2 clock enable in low-power mode
    volatile uint32_t RESERVED5[2]; ///< 0x68-0x6C: reserved
    volatile uint32_t BDCR;         ///< 0x70: backup domain control
    volatile uint32_t CSR;          ///< 0x74: clock control and status
    volatile uint32_t RESERVED6[2]; ///< 0x78-0x7C: reserved
    volatile uint32_t SSCGR;        ///< 0x80: spread-spectrum clock generation
    volatile uint32_t PLLI2SCFGR;   ///< 0x84: PLLI2S configuration
    volatile uint32_t PLLSAICFGR;   ///< 0x88: PLLSAI configuration
    volatile uint32_t DCKCFGR;      ///< 0x8C: dedicated clock configuration
    volatile uint32_t CKGATENR;     ///< 0x90: clock gating enable
    volatile uint32_t DCKCFGR2;     ///< 0x94: dedicated clock configuration 2
} RccRegisters_t;

#define RCC ((RccRegisters_t *)RCC_BASE) ///< The RCC instance

/** @name CR - clock control (0x00)
 *  @{ */
#define RCC_CR_HSION_Pos (0U) ///< Bit 0
/** \regaccess{rw} HSI on. */
#define RCC_CR_HSION ((uint32_t)1U << RCC_CR_HSION_Pos)
#define RCC_CR_HSIRDY_Pos (1U) ///< Bit 1
/** \regaccess{ro} HSI ready. */
#define RCC_CR_HSIRDY ((uint32_t)1U << RCC_CR_HSIRDY_Pos)
#define RCC_CR_HSEON_Pos (16U) ///< Bit 16
/** \regaccess{rw} HSE on. */
#define RCC_CR_HSEON ((uint32_t)1U << RCC_CR_HSEON_Pos)
#define RCC_CR_HSERDY_Pos (17U) ///< Bit 17
/** \regaccess{ro} HSE ready. */
#define RCC_CR_HSERDY ((uint32_t)1U << RCC_CR_HSERDY_Pos)
#define RCC_CR_HSEBYP_Pos (18U) ///< Bit 18
/** \regaccess{rw} HSE bypass: external clock on OSC_IN. Writable only with HSEON clear. */
#define RCC_CR_HSEBYP ((uint32_t)1U << RCC_CR_HSEBYP_Pos)
#define RCC_CR_CSSON_Pos (19U) ///< Bit 19
/** \regaccess{rw} Clock security system on. */
#define RCC_CR_CSSON ((uint32_t)1U << RCC_CR_CSSON_Pos)
#define RCC_CR_PLLON_Pos (24U) ///< Bit 24
/** \regaccess{rw} Main PLL on. Configure PLLCFGR first. */
#define RCC_CR_PLLON ((uint32_t)1U << RCC_CR_PLLON_Pos)
#define RCC_CR_PLLRDY_Pos (25U) ///< Bit 25
/** \regaccess{ro} Main PLL locked. */
#define RCC_CR_PLLRDY ((uint32_t)1U << RCC_CR_PLLRDY_Pos)
/** @} */

/** @name PLLCFGR - PLL configuration (0x04)
 *  Every field is \regaccess{rw} and writable only while the PLL is off.
 *  @{ */
#define RCC_PLLCFGR_PLLM_Pos (0U) ///< Bits 5:0
/** Input divider M (2-63): VCO input = source / M, 1-2 MHz. */
#define RCC_PLLCFGR_PLLM_Msk ((uint32_t)0x3FU << RCC_PLLCFGR_PLLM_Pos)
#define RCC_PLLCFGR_PLLN_Pos (6U) ///< Bits 14:6
/** Multiplier N (50-432): VCO output = VCO input x N, 100-432 MHz. */
#define RCC_PLLCFGR_PLLN_Msk ((uint32_t)0x1FFU << RCC_PLLCFGR_PLLN_Pos)
#define RCC_PLLCFGR_PLLP_Pos (16U) ///< Bits 17:16
/** Output divider P for SYSCLK; values below. */
#define RCC_PLLCFGR_PLLP_Msk ((uint32_t)0x3U << RCC_PLLCFGR_PLLP_Pos)
#define RCC_PLLCFGR_PLLSRC_Pos (22U) ///< Bit 22
/** PLL input: 0 = HSI, 1 = HSE. */
#define RCC_PLLCFGR_PLLSRC ((uint32_t)1U << RCC_PLLCFGR_PLLSRC_Pos)
/** @} */

/** @name PLLCFGR.PLLSRC values
 *  Already shifted into position.
 *  @{ */
#define RCC_PLLCFGR_PLLSRC_HSI (0x0U)               ///< HSI feeds the PLL
#define RCC_PLLCFGR_PLLSRC_HSE (RCC_PLLCFGR_PLLSRC) ///< HSE feeds the PLL
/** @} */

/** @name PLLCFGR.PLLP values
 *  @{ */
#define RCC_PLLCFGR_PLLP_DIV2 (0x0U) ///< VCO / 2
#define RCC_PLLCFGR_PLLP_DIV4 (0x1U) ///< VCO / 4
#define RCC_PLLCFGR_PLLP_DIV6 (0x2U) ///< VCO / 6
#define RCC_PLLCFGR_PLLP_DIV8 (0x3U) ///< VCO / 8
/** @} */

/** @name CFGR - clock configuration (0x08)
 *  @{ */
#define RCC_CFGR_SW_Pos (0U) ///< Bits 1:0
/** \regaccess{rw} SYSCLK source requested; values below. */
#define RCC_CFGR_SW_Msk ((uint32_t)0x3U << RCC_CFGR_SW_Pos)
#define RCC_CFGR_SWS_Pos (2U) ///< Bits 3:2
/** \regaccess{ro} SYSCLK source in use, reported once a switch completes. */
#define RCC_CFGR_SWS_Msk ((uint32_t)0x3U << RCC_CFGR_SWS_Pos)
#define RCC_CFGR_HPRE_Pos (4U) ///< Bits 7:4
/** \regaccess{rw} AHB prescaler; values below. */
#define RCC_CFGR_HPRE_Msk ((uint32_t)0xFU << RCC_CFGR_HPRE_Pos)
#define RCC_CFGR_PPRE1_Pos (10U) ///< Bits 12:10
/** \regaccess{rw} APB1 prescaler; APB1 must stay at or below 45 MHz. */
#define RCC_CFGR_PPRE1_Msk ((uint32_t)0x7U << RCC_CFGR_PPRE1_Pos)
#define RCC_CFGR_PPRE2_Pos (13U) ///< Bits 15:13
/** \regaccess{rw} APB2 prescaler; APB2 must stay at or below 90 MHz. */
#define RCC_CFGR_PPRE2_Msk ((uint32_t)0x7U << RCC_CFGR_PPRE2_Pos)
/** @} */

/** @name CFGR.SW / CFGR.SWS values
 *  One encoding for both fields; shift by the field's `_Pos`.
 *  @{ */
#define RCC_CFGR_SYSCLK_HSI (0x0U) ///< HSI
#define RCC_CFGR_SYSCLK_HSE (0x1U) ///< HSE
#define RCC_CFGR_SYSCLK_PLL (0x2U) ///< Main PLL (P output)
/** @} */

/** @name CFGR.HPRE values
 *  There is no /32.
 *  @{ */
#define RCC_CFGR_HPRE_DIV1 (0x0U)   ///< SYSCLK / 1
#define RCC_CFGR_HPRE_DIV2 (0x8U)   ///< SYSCLK / 2
#define RCC_CFGR_HPRE_DIV4 (0x9U)   ///< SYSCLK / 4
#define RCC_CFGR_HPRE_DIV8 (0xAU)   ///< SYSCLK / 8
#define RCC_CFGR_HPRE_DIV16 (0xBU)  ///< SYSCLK / 16
#define RCC_CFGR_HPRE_DIV64 (0xCU)  ///< SYSCLK / 64
#define RCC_CFGR_HPRE_DIV128 (0xDU) ///< SYSCLK / 128
#define RCC_CFGR_HPRE_DIV256 (0xEU) ///< SYSCLK / 256
#define RCC_CFGR_HPRE_DIV512 (0xFU) ///< SYSCLK / 512
/** @} */

/** @name CFGR.PPRE1 / CFGR.PPRE2 values
 *  One encoding for both fields; shift by the field's `_Pos`.
 *  @{ */
#define RCC_CFGR_PPRE_DIV1 (0x0U)  ///< HCLK / 1
#define RCC_CFGR_PPRE_DIV2 (0x4U)  ///< HCLK / 2
#define RCC_CFGR_PPRE_DIV4 (0x5U)  ///< HCLK / 4
#define RCC_CFGR_PPRE_DIV8 (0x6U)  ///< HCLK / 8
#define RCC_CFGR_PPRE_DIV16 (0x7U) ///< HCLK / 16
/** @} */

/** @name CIR - clock interrupt (0x0C)
 *  @{ */
#define RCC_CIR_CSSF_Pos (7U) ///< Bit 7
/** \regaccess{ro} Clock security system detected an HSE failure. */
#define RCC_CIR_CSSF ((uint32_t)1U << RCC_CIR_CSSF_Pos)
#define RCC_CIR_CSSC_Pos (23U) ///< Bit 23
/** \regaccess{wo} Write 1 to clear CSSF. */
#define RCC_CIR_CSSC ((uint32_t)1U << RCC_CIR_CSSC_Pos)
/** @} */

/** @name AHB1RSTR - AHB1 peripheral reset (0x10)
 *  Every bit is \regaccess{rw}: set to hold the peripheral in reset.
 *  @{ */
#define RCC_AHB1RSTR_GPIOARST_Pos (0U)                                    ///< Bit 0
#define RCC_AHB1RSTR_GPIOARST ((uint32_t)1U << RCC_AHB1RSTR_GPIOARST_Pos) ///< GPIOA reset
#define RCC_AHB1RSTR_GPIOBRST_Pos (1U)                                    ///< Bit 1
#define RCC_AHB1RSTR_GPIOBRST ((uint32_t)1U << RCC_AHB1RSTR_GPIOBRST_Pos) ///< GPIOB reset
#define RCC_AHB1RSTR_GPIOCRST_Pos (2U)                                    ///< Bit 2
#define RCC_AHB1RSTR_GPIOCRST ((uint32_t)1U << RCC_AHB1RSTR_GPIOCRST_Pos) ///< GPIOC reset
#define RCC_AHB1RSTR_GPIODRST_Pos (3U)                                    ///< Bit 3
#define RCC_AHB1RSTR_GPIODRST ((uint32_t)1U << RCC_AHB1RSTR_GPIODRST_Pos) ///< GPIOD reset
#define RCC_AHB1RSTR_GPIOERST_Pos (4U)                                    ///< Bit 4
#define RCC_AHB1RSTR_GPIOERST ((uint32_t)1U << RCC_AHB1RSTR_GPIOERST_Pos) ///< GPIOE reset
#define RCC_AHB1RSTR_GPIOFRST_Pos (5U)                                    ///< Bit 5
#define RCC_AHB1RSTR_GPIOFRST ((uint32_t)1U << RCC_AHB1RSTR_GPIOFRST_Pos) ///< GPIOF reset
#define RCC_AHB1RSTR_GPIOGRST_Pos (6U)                                    ///< Bit 6
#define RCC_AHB1RSTR_GPIOGRST ((uint32_t)1U << RCC_AHB1RSTR_GPIOGRST_Pos) ///< GPIOG reset
#define RCC_AHB1RSTR_GPIOHRST_Pos (7U)                                    ///< Bit 7
#define RCC_AHB1RSTR_GPIOHRST ((uint32_t)1U << RCC_AHB1RSTR_GPIOHRST_Pos) ///< GPIOH reset
/** @} */

/** @name AHB1ENR - AHB1 clock enable (0x30)
 *  Every bit is \regaccess{rw}. Read the register back after enabling,
 *  before touching the peripheral.
 *  @{ */
#define RCC_AHB1ENR_GPIOAEN_Pos (0U)                                  ///< Bit 0
#define RCC_AHB1ENR_GPIOAEN ((uint32_t)1U << RCC_AHB1ENR_GPIOAEN_Pos) ///< GPIOA clock
#define RCC_AHB1ENR_GPIOBEN_Pos (1U)                                  ///< Bit 1
#define RCC_AHB1ENR_GPIOBEN ((uint32_t)1U << RCC_AHB1ENR_GPIOBEN_Pos) ///< GPIOB clock
#define RCC_AHB1ENR_GPIOCEN_Pos (2U)                                  ///< Bit 2
#define RCC_AHB1ENR_GPIOCEN ((uint32_t)1U << RCC_AHB1ENR_GPIOCEN_Pos) ///< GPIOC clock
#define RCC_AHB1ENR_GPIODEN_Pos (3U)                                  ///< Bit 3
#define RCC_AHB1ENR_GPIODEN ((uint32_t)1U << RCC_AHB1ENR_GPIODEN_Pos) ///< GPIOD clock
#define RCC_AHB1ENR_GPIOEEN_Pos (4U)                                  ///< Bit 4
#define RCC_AHB1ENR_GPIOEEN ((uint32_t)1U << RCC_AHB1ENR_GPIOEEN_Pos) ///< GPIOE clock
#define RCC_AHB1ENR_GPIOFEN_Pos (5U)                                  ///< Bit 5
#define RCC_AHB1ENR_GPIOFEN ((uint32_t)1U << RCC_AHB1ENR_GPIOFEN_Pos) ///< GPIOF clock
#define RCC_AHB1ENR_GPIOGEN_Pos (6U)                                  ///< Bit 6
#define RCC_AHB1ENR_GPIOGEN ((uint32_t)1U << RCC_AHB1ENR_GPIOGEN_Pos) ///< GPIOG clock
#define RCC_AHB1ENR_GPIOHEN_Pos (7U)                                  ///< Bit 7
#define RCC_AHB1ENR_GPIOHEN ((uint32_t)1U << RCC_AHB1ENR_GPIOHEN_Pos) ///< GPIOH clock
/** @} */

/** @name APB1RSTR - APB1 peripheral reset (0x20)
 *  Every bit is \regaccess{rw}.
 *  @{ */
#define RCC_APB1RSTR_USART2RST_Pos (17U) ///< Bit 17
/** USART2 reset. */
#define RCC_APB1RSTR_USART2RST ((uint32_t)1U << RCC_APB1RSTR_USART2RST_Pos)
#define RCC_APB1RSTR_PWRRST_Pos (28U)                                 ///< Bit 28
#define RCC_APB1RSTR_PWRRST ((uint32_t)1U << RCC_APB1RSTR_PWRRST_Pos) ///< PWR reset
/** @} */

/** @name APB1ENR - APB1 clock enable (0x40)
 *  Every bit is \regaccess{rw}.
 *  @{ */
#define RCC_APB1ENR_USART2EN_Pos (17U) ///< Bit 17
/** USART2 clock. */
#define RCC_APB1ENR_USART2EN ((uint32_t)1U << RCC_APB1ENR_USART2EN_Pos)
#define RCC_APB1ENR_PWREN_Pos (28U) ///< Bit 28
/** PWR clock; required before any PWR register access. */
#define RCC_APB1ENR_PWREN ((uint32_t)1U << RCC_APB1ENR_PWREN_Pos)
/** @} */

/** @name APB2RSTR - APB2 peripheral reset (0x24)
 *  @{ */
#define RCC_APB2RSTR_SYSCFGRST_Pos (14U) ///< Bit 14
/** \regaccess{rw} SYSCFG reset. */
#define RCC_APB2RSTR_SYSCFGRST ((uint32_t)1U << RCC_APB2RSTR_SYSCFGRST_Pos)
/** @} */

/** @name APB2ENR - APB2 clock enable (0x44)
 *  @{ */
#define RCC_APB2ENR_SYSCFGEN_Pos (14U) ///< Bit 14
/** \regaccess{rw} SYSCFG clock; required before any SYSCFG register access. */
#define RCC_APB2ENR_SYSCFGEN ((uint32_t)1U << RCC_APB2ENR_SYSCFGEN_Pos)
/** @} */

/** @name CSR - clock control and status (0x74)
 *  The reset-cause flags stay set until RMVF clears them.
 *  @{ */
#define RCC_CSR_LSION_Pos (0U) ///< Bit 0
/** \regaccess{rw} LSI on. */
#define RCC_CSR_LSION ((uint32_t)1U << RCC_CSR_LSION_Pos)
#define RCC_CSR_LSIRDY_Pos (1U) ///< Bit 1
/** \regaccess{ro} LSI ready. */
#define RCC_CSR_LSIRDY ((uint32_t)1U << RCC_CSR_LSIRDY_Pos)
#define RCC_CSR_RMVF_Pos (24U) ///< Bit 24
/** \regaccess{rw} Write 1 to clear every reset flag below. */
#define RCC_CSR_RMVF ((uint32_t)1U << RCC_CSR_RMVF_Pos)
#define RCC_CSR_BORRSTF_Pos (25U) ///< Bit 25
/** \regaccess{ro} Brown-out reset occurred. */
#define RCC_CSR_BORRSTF ((uint32_t)1U << RCC_CSR_BORRSTF_Pos)
#define RCC_CSR_PINRSTF_Pos (26U) ///< Bit 26
/** \regaccess{ro} NRST pin reset occurred. */
#define RCC_CSR_PINRSTF ((uint32_t)1U << RCC_CSR_PINRSTF_Pos)
#define RCC_CSR_PORRSTF_Pos (27U) ///< Bit 27
/** \regaccess{ro} Power-on reset occurred. */
#define RCC_CSR_PORRSTF ((uint32_t)1U << RCC_CSR_PORRSTF_Pos)
#define RCC_CSR_SFTRSTF_Pos (28U) ///< Bit 28
/** \regaccess{ro} Software reset occurred. */
#define RCC_CSR_SFTRSTF ((uint32_t)1U << RCC_CSR_SFTRSTF_Pos)
#define RCC_CSR_IWDGRSTF_Pos (29U) ///< Bit 29
/** \regaccess{ro} Independent watchdog reset occurred. */
#define RCC_CSR_IWDGRSTF ((uint32_t)1U << RCC_CSR_IWDGRSTF_Pos)
#define RCC_CSR_WWDGRSTF_Pos (30U) ///< Bit 30
/** \regaccess{ro} Window watchdog reset occurred. */
#define RCC_CSR_WWDGRSTF ((uint32_t)1U << RCC_CSR_WWDGRSTF_Pos)
#define RCC_CSR_LPWRRSTF_Pos (31U) ///< Bit 31
/** \regaccess{ro} Low-power reset occurred. */
#define RCC_CSR_LPWRRSTF ((uint32_t)1U << RCC_CSR_LPWRRSTF_Pos)
/** @} */

/** @} */

#endif /* RCC_REG_H */
