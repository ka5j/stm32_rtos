/**
 * @file smoke.c
 * @brief On-target smoke test: runs the drivers and the BSP on the
 *        NUCLEO-F446RE and checks what the silicon did.
 *
 * Built by `make smoke` in place of app/src/main.c, and flashed by
 * `make smoke-flash`. It is a test, so it calls drivers directly. Each check
 * prints `PASS <name>` or `FAIL <name> <detail>` on the ST-LINK virtual COM
 * port (USART2, 115200 8N1), then a summary. LD2 then blinks at 1 Hz if every
 * check passed and at 5 Hz if any failed or the console timed out.
 *
 * Readable text is itself a result: the baud rate is generated from PCLK1,
 * so garbage on the port means the clock tree is not what bsp.h says.
 *
 * The watchdog checks need resets, so a full run is three boots, told apart
 * by the reset cause in RCC_CSR:
 * 1. First boot: every check, then the IWDG is started and starved.
 * 2. After the IWDG reset: `PASS iwdg_reset_cause`, every check, then the
 *    WWDG is started and refreshed above its window.
 * 3. After the WWDG reset: `PASS wwdg_reset_cause`, every check, then LD2
 *    blinks.
 *
 * A boot provokes the next reset only if all its checks passed, so a slow
 * blink after the third report means the whole chain passed.
 *
 * `make smoke-flash SMOKE_FAULT=1` ends with a deliberate divide by zero
 * instead of blinking. Fault_Handler then records it; halt and read
 * `p/x faultRecord` in GDB.
 */
#include "bsp.h"
#include "exti.h"
#include "fault.h"
#include "gpio.h"
#include "iwdg.h"
#include "nvic.h"
#include "rcc.h"
#include "systick.h"
#include "uart.h"
#include "wwdg.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef SMOKE_FAULT
#define SMOKE_FAULT 0
#endif

/** SR polls per UART wait: one byte at 115200 is about 1,000 polls at 180 MHz. */
#define SMOKE_UART_TIMEOUT (100000U)
/** SysTick rate, as the kernel will use it. */
#define SMOKE_TICK_HZ (1000U)
/** HSI frequency, the reset clock (RM0390 section 6.2.2). */
#define SMOKE_HSI_HZ (16000000U)
/** IWDG timeout: LSI / 32, reload 100, so 69-190 ms over the LSI's range. */
#define SMOKE_IWDG_PRESCALER (IWDG_PR_DIV32)
#define SMOKE_IWDG_RELOAD (100U)
/** WWDG: PCLK1 / 4096 / 8 is 728 us a count, so 0x7F to the 0x40 early wakeup is 45.9 ms. */
#define SMOKE_WWDG_PRESCALER (WWDG_CFR_WDGTB_DIV8)
#define SMOKE_WWDG_WINDOW (0x5FU)
#define SMOKE_WWDG_COUNTER (0x7FU)
/** The NVIC check's interrupt: one nothing else uses. */
#define SMOKE_SPARE_IRQ (TIM7_IRQn)

static volatile uint32_t ticks;
static volatile uint32_t spare_irq_count;
static volatile uint32_t exti5_count;
static volatile uint32_t exti13_count;
static volatile uint32_t wwdg_ewi_count;
static uint32_t checks_run;
static uint32_t checks_failed;
/** Set if any console write timed out; the report may then be incomplete. */
static bool console_failed;

/** DSB then ISB: a pended interrupt is taken before the next instruction (PM0214 4.3.10). */
static inline void syncBarrier(void)
{
    __asm__ volatile("dsb 0xF\n\tisb 0xF" ::: "memory");
}

void SysTick_Handler(void)
{
    ticks++;
}

void TIM7_IRQHandler(void)
{
    spare_irq_count++;
}

/** Count and clear one line of a shared EXTI vector. */
static void extiCountLine(uint32_t line, volatile uint32_t *count)
{
    bool pending = false;

    if ((extiIsPending(EXTI, line, &pending) == DRIVER_STATUS_OK) && pending
        && (extiClearPending(EXTI, line) == DRIVER_STATUS_OK))
    {
        (*count)++;
    }
}

/** Refresh the WWDG from its early wakeup at 0x40, inside the window. */
void WWDG_IRQHandler(void)
{
    wwdgClearEarlyWakeup(WWDG);
    if (wwdgRefresh(WWDG, SMOKE_WWDG_COUNTER) == DRIVER_STATUS_OK)
    {
        wwdg_ewi_count++;
    }
}

void EXTI9_5_IRQHandler(void)
{
    extiCountLine(5U, &exti5_count);
}

void EXTI15_10_IRQHandler(void)
{
    extiCountLine(13U, &exti13_count);
}

/* --- Console ------------------------------------------------------------ */

static void print(const char *text)
{
    size_t length = 0U;

    while (text[length] != '\0')
    {
        length++;
    }

    if (uartTransmit(USART2, (const uint8_t *)text, length, SMOKE_UART_TIMEOUT) != DRIVER_STATUS_OK)
    {
        console_failed = true;
    }
}

static void printHex(uint32_t value)
{
    static const char digits[] = "0123456789abcdef";
    char text[11] = "0x";

    for (uint32_t i = 0U; i < 8U; i++)
    {
        text[2U + i] = digits[(value >> (28U - (4U * i))) & 0xFU];
    }
    text[10] = '\0';
    print(text);
}

static void printDec(uint32_t value)
{
    char text[11];
    uint32_t i = 10U;
    uint32_t rest = value;

    text[10] = '\0';
    do
    {
        i--;
        text[i] = (char)('0' + (rest % 10U));
        rest /= 10U;
    } while (rest != 0U);
    print(&text[i]);
}

/**
 * Record one check. @p detail is printed after FAIL as `name=value`, to say
 * what was read instead.
 */
static void check(const char *name, bool passed, const char *detail_name, uint32_t detail)
{
    checks_run++;
    if (passed)
    {
        print("PASS ");
        print(name);
    }
    else
    {
        checks_failed++;
        print("FAIL ");
        print(name);
        print(" ");
        print(detail_name);
        print("=");
        printHex(detail);
    }
    print("\r\n");
}

/**
 * PCLK1 as RCC is actually configured, so the console works even when
 * bspClockInit() failed part-way. bspClockInit() only ever sets HPRE to /1.
 */
static uint32_t actualPclk1Hz(void)
{
    uint32_t sws = (RCC->CFGR & RCC_CFGR_SWS_Msk) >> RCC_CFGR_SWS_Pos;
    uint32_t ppre1 = (RCC->CFGR & RCC_CFGR_PPRE1_Msk) >> RCC_CFGR_PPRE1_Pos;
    uint32_t sysclk = SMOKE_HSI_HZ;
    uint32_t shift = 0U;

    if (sws == RCC_CFGR_SYSCLK_PLL)
    {
        sysclk = BSP_SYSCLK_HZ;
    }
    else if (sws == RCC_CFGR_SYSCLK_HSE)
    {
        sysclk = BSP_HSE_HZ;
    }
    else
    {
        /* HSI */
    }

    /* PPRE 0xx = /1, 100 = /2, 101 = /4, 110 = /8, 111 = /16. */
    if (ppre1 >= RCC_CFGR_PPRE_DIV2)
    {
        shift = (ppre1 - RCC_CFGR_PPRE_DIV2) + 1U;
    }

    return sysclk >> shift;
}

/* --- Bring-up ----------------------------------------------------------- */

/** LD2 on PA5, and USART2 on PA2 (TX) and PA3 (RX), AF7 (UM1724). */
static DriverStatus_e boardIoInit(void)
{
    DriverStatus_e status = rccGpioClockEnable(RCC, GPIOA);

    if (status == DRIVER_STATUS_OK)
    {
        status = gpioInit(GPIOA, GPIO_PIN_5, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_OSPEED_LOW,
                          GPIO_PUPD_NONE);
    }
    if (status == DRIVER_STATUS_OK)
    {
        status = gpioSetAlternateFunction(GPIOA, GPIO_PIN_2, 7U);
    }
    if (status == DRIVER_STATUS_OK)
    {
        status = gpioSetAlternateFunction(GPIOA, GPIO_PIN_3, 7U);
    }
    if (status == DRIVER_STATUS_OK)
    {
        status = gpioInit(GPIOA, GPIO_PIN_2, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_MEDIUM,
                          GPIO_PUPD_NONE);
    }
    if (status == DRIVER_STATUS_OK)
    {
        status = gpioInit(GPIOA, GPIO_PIN_3, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_OSPEED_MEDIUM,
                          GPIO_PUPD_NONE);
    }
    if (status == DRIVER_STATUS_OK)
    {
        rccUsart2ClockEnable(RCC);
        status = uartInit(USART2, actualPclk1Hz(), 115200U, USART_CR2_STOP_1, 0U, 0U,
                          USART_CR1_TE | USART_CR1_RE);
    }

    return status;
}

/* --- Checks ------------------------------------------------------------- */

/** The clock tree in bsp.h, read back from RCC, PWR and FLASH. */
static void checkClocks(DriverStatus_e clock_status)
{
    uint32_t cfgr = RCC->CFGR;
    uint32_t pllcfgr = RCC->PLLCFGR;
    uint32_t expected_pll =
        (BSP_PLL_M << RCC_PLLCFGR_PLLM_Pos) | (BSP_PLL_N << RCC_PLLCFGR_PLLN_Pos)
        | (RCC_PLLCFGR_PLLP_DIV2 << RCC_PLLCFGR_PLLP_Pos) | RCC_PLLCFGR_PLLSRC_HSE;
    uint32_t pll_fields =
        RCC_PLLCFGR_PLLM_Msk | RCC_PLLCFGR_PLLN_Msk | RCC_PLLCFGR_PLLP_Msk | RCC_PLLCFGR_PLLSRC;
    uint32_t expected_prescalers = (RCC_CFGR_HPRE_DIV1 << RCC_CFGR_HPRE_Pos)
                                   | (RCC_CFGR_PPRE_DIV4 << RCC_CFGR_PPRE1_Pos)
                                   | (RCC_CFGR_PPRE_DIV2 << RCC_CFGR_PPRE2_Pos);
    uint32_t prescaler_fields = RCC_CFGR_HPRE_Msk | RCC_CFGR_PPRE1_Msk | RCC_CFGR_PPRE2_Msk;
    uint32_t pwr_ready = PWR_CSR_VOSRDY | PWR_CSR_ODRDY | PWR_CSR_ODSWRDY;
    uint32_t expected_acr = (BSP_FLASH_LATENCY << FLASH_ACR_LATENCY_Pos) | FLASH_ACR_PRFTEN
                            | FLASH_ACR_ICEN | FLASH_ACR_DCEN;
    uint32_t acr_fields =
        FLASH_ACR_LATENCY_Msk | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;

    check("bsp_clock_init", clock_status == DRIVER_STATUS_OK, "status", (uint32_t)clock_status);
    check("sysclk_is_pll", ((cfgr & RCC_CFGR_SWS_Msk) >> RCC_CFGR_SWS_Pos) == RCC_CFGR_SYSCLK_PLL,
          "CFGR", cfgr);
    check("hse_bypass_ready",
          (RCC->CR & (RCC_CR_HSEBYP | RCC_CR_HSERDY)) == (RCC_CR_HSEBYP | RCC_CR_HSERDY), "CR",
          RCC->CR);
    check("pll_m_n_p_src", (pllcfgr & pll_fields) == expected_pll, "PLLCFGR", pllcfgr);
    check("bus_prescalers", (cfgr & prescaler_fields) == expected_prescalers, "CFGR", cfgr);
    check("pwr_scale1", ((PWR->CR & PWR_CR_VOS_Msk) >> PWR_CR_VOS_Pos) == PWR_CR_VOS_SCALE1, "CR",
          PWR->CR);
    check("pwr_vos_od_ready", (PWR->CSR & pwr_ready) == pwr_ready, "CSR", PWR->CSR);
    check("flash_latency_caches", (FLASH->ACR & acr_fields) == expected_acr, "ACR", FLASH->ACR);
    /* 45 MHz / 115200 = 390.6, rounded to 391. */
    check("uart_brr", USART2->BRR == 391U, "BRR", USART2->BRR);
}

/** LD2 driven high and low, read back through IDR, and toggled. */
static void checkGpio(void)
{
    gpioWritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    check("gpio_write_set", gpioReadPin(GPIOA, GPIO_PIN_5) == GPIO_PIN_SET, "IDR", GPIOA->IDR);

    gpioTogglePin(GPIOA, GPIO_PIN_5);
    check("gpio_toggle_reset", gpioReadPin(GPIOA, GPIO_PIN_5) == GPIO_PIN_RESET, "IDR", GPIOA->IDR);
}

/** A pended spare interrupt is taken once enabled, and not while disabled. */
static void checkNvic(void)
{
    uint32_t priority = 0U;
    DriverStatus_e status = nvicSetPriority(NVIC, SMOKE_SPARE_IRQ, 5U);

    if (status == DRIVER_STATUS_OK)
    {
        status = nvicGetPriority(NVIC, SMOKE_SPARE_IRQ, &priority);
    }
    check("nvic_priority_readback", (status == DRIVER_STATUS_OK) && (priority == 5U), "priority",
          priority);

    if (status == DRIVER_STATUS_OK)
    {
        status = nvicEnableIrq(NVIC, SMOKE_SPARE_IRQ);
    }
    if (status == DRIVER_STATUS_OK)
    {
        status = nvicSetPending(NVIC, SMOKE_SPARE_IRQ);
        syncBarrier();
    }
    /* Taken at once: nothing masks it and its priority beats thread mode. */
    check("nvic_pending_irq_taken", (status == DRIVER_STATUS_OK) && (spare_irq_count == 1U),
          "count", spare_irq_count);

    if (status == DRIVER_STATUS_OK)
    {
        status = nvicDisableIrq(NVIC, SMOKE_SPARE_IRQ);
    }
    if (status == DRIVER_STATUS_OK)
    {
        status = nvicSetPending(NVIC, SMOKE_SPARE_IRQ);
        syncBarrier();
    }
    check("nvic_disabled_irq_held", (status == DRIVER_STATUS_OK) && (spare_irq_count == 1U),
          "count", spare_irq_count);

    if (status == DRIVER_STATUS_OK)
    {
        status = nvicClearPending(NVIC, SMOKE_SPARE_IRQ);
    }
    check("nvic_clear_pending",
          (status == DRIVER_STATUS_OK)
              && ((NVIC->ISPR[(uint32_t)SMOKE_SPARE_IRQ >> 5U]
                   & (1U << ((uint32_t)SMOKE_SPARE_IRQ & 0x1FU)))
                  == 0U),
          "count", spare_irq_count);
}

/**
 * SysTick at 1 kHz from HCLK, timed against 1,152 bytes on the UART, which
 * take 100 ms at 115200 baud (10 bits each) from PCLK1. Getting 100 ticks
 * shows SysTick's reload and the HCLK/PCLK1 ratio agree with bsp.h.
 */
static void checkSystick(void)
{
    static const uint8_t zeros[64] = {0U};
    DriverStatus_e status = nvicSetSystemHandlerPriority(SCB, SysTick_IRQn, NVIC_PRIORITY_MAX);
    uint32_t start;
    uint32_t elapsed;

    if (status == DRIVER_STATUS_OK)
    {
        status = systickInit(SYSTICK, BSP_HCLK_HZ, SMOKE_TICK_HZ);
    }
    check("systick_init", status == DRIVER_STATUS_OK, "status", (uint32_t)status);

    /* Drain the console first, so the timed bytes start on an idle line. */
    if (status == DRIVER_STATUS_OK)
    {
        status = uartFlush(USART2, SMOKE_UART_TIMEOUT);
    }
    start = ticks;
    for (uint32_t i = 0U; (i < 18U) && (status == DRIVER_STATUS_OK); i++)
    {
        status = uartTransmit(USART2, zeros, sizeof zeros, SMOKE_UART_TIMEOUT);
    }
    elapsed = ticks - start;

    check("systick_100ms_vs_uart",
          (status == DRIVER_STATUS_OK) && (elapsed >= 99U) && (elapsed <= 101U), "ticks", elapsed);
}

/** Wait a bounded time for an interrupt to bring @p count to @p expected. */
static bool waitForCount(const volatile uint32_t *count, uint32_t expected)
{
    for (uint32_t i = 0U; (i < 10000U) && (*count != expected); i++)
    {
    }

    return *count == expected;
}

/**
 * Line 13 (PC13, the B1 button) raised from software, and line 5 routed to
 * PA5 (LD2), whose input stage stays live as an output. Driving the LED makes
 * real pin edges, with no wiring.
 */
static void checkExti(void)
{
    DriverStatus_e status;

    rccSyscfgClockEnable(RCC);

    status = extiSelectPort(SYSCFG, 13U, GPIOC);
    check("exti_route_pc13",
          (status == DRIVER_STATUS_OK) && (((SYSCFG->EXTICR[3] >> 4U) & 0xFU) == SYSCFG_EXTICR_PC),
          "EXTICR4", SYSCFG->EXTICR[3]);

    if (status == DRIVER_STATUS_OK)
    {
        status = extiConfigure(EXTI, 13U, EXTI_EDGE_FALLING);
    }
    if (status == DRIVER_STATUS_OK)
    {
        status = extiClearPending(EXTI, 13U);
    }
    if (status == DRIVER_STATUS_OK)
    {
        status = extiEnableInterrupt(EXTI, 13U);
    }
    if (status == DRIVER_STATUS_OK)
    {
        status = nvicEnableIrq(NVIC, EXTI15_10_IRQn);
    }
    if (status == DRIVER_STATUS_OK)
    {
        status = extiTriggerSoftware(EXTI, 13U);
        syncBarrier();
    }
    check("exti_software_trigger_taken",
          (status == DRIVER_STATUS_OK) && waitForCount(&exti13_count, 1U), "count", exti13_count);
    /* RM0390 10.3.5: clearing PR also clears the SWIER bit. */
    check("exti_clear_pending_clears_swier", (EXTI->SWIER & (1U << 13U)) == 0U, "SWIER",
          EXTI->SWIER);

    /* Line 5 from PA5, rising edge only, starting low. */
    gpioWritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    status = extiSelectPort(SYSCFG, 5U, GPIOA);
    if (status == DRIVER_STATUS_OK)
    {
        status = extiConfigure(EXTI, 5U, EXTI_EDGE_RISING);
    }
    if (status == DRIVER_STATUS_OK)
    {
        status = extiClearPending(EXTI, 5U);
    }
    if (status == DRIVER_STATUS_OK)
    {
        status = extiEnableInterrupt(EXTI, 5U);
    }
    if (status == DRIVER_STATUS_OK)
    {
        status = nvicEnableIrq(NVIC, EXTI9_5_IRQn);
    }

    gpioWritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    check("exti_rising_edge_taken", (status == DRIVER_STATUS_OK) && waitForCount(&exti5_count, 1U),
          "count", exti5_count);
    gpioWritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    check("exti_falling_edge_ignored", !waitForCount(&exti5_count, 2U), "count", exti5_count);

    if (status == DRIVER_STATUS_OK)
    {
        status = extiConfigure(EXTI, 5U, EXTI_EDGE_FALLING);
    }
    gpioWritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    check("exti_rising_edge_ignored", !waitForCount(&exti5_count, 2U), "count", exti5_count);
    gpioWritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
    check("exti_falling_edge_taken", (status == DRIVER_STATUS_OK) && waitForCount(&exti5_count, 2U),
          "count", exti5_count);

    /* The LED blinks from here on; stop it raising line 5. */
    if ((extiDisableInterrupt(EXTI, 5U) != DRIVER_STATUS_OK)
        || (nvicDisableIrq(NVIC, EXTI9_5_IRQn) != DRIVER_STATUS_OK))
    {
        check("exti_line5_disable", false, "status", 0U);
    }
}

/** Busy-wait @p ms SysTick ticks, refreshing the IWDG every @p refresh_ms (0: never). */
static void waitMs(uint32_t ms, uint32_t refresh_ms)
{
    uint32_t start = ticks;
    uint32_t last_refresh = start;

    while ((ticks - start) < ms)
    {
        if ((refresh_ms != 0U) && ((ticks - last_refresh) >= refresh_ms))
        {
            iwdgRefresh(IWDG);
            last_refresh = ticks;
        }
    }
}

/** Where this boot sits in the watchdog chain, from the reset cause. */
typedef enum SmokeBoot_e
{
    SMOKE_BOOT_FIRST,      ///< Anything but a watchdog reset
    SMOKE_BOOT_AFTER_IWDG, ///< The IWDG reset that the first boot provoked
    SMOKE_BOOT_AFTER_WWDG, ///< The WWDG reset that the second boot provoked
} SmokeBoot_e;

static SmokeBoot_e smokeBoot(uint32_t reset_flags)
{
    SmokeBoot_e boot = SMOKE_BOOT_FIRST;

    if ((reset_flags & RCC_CSR_WWDGRSTF) != 0U)
    {
        boot = SMOKE_BOOT_AFTER_WWDG;
    }
    else if ((reset_flags & RCC_CSR_IWDGRSTF) != 0U)
    {
        boot = SMOKE_BOOT_AFTER_IWDG;
    }
    else
    {
        /* Power-on, pin or software reset: start the chain. */
    }

    return boot;
}

/** Start the IWDG and show that refreshing holds it off. */
static void checkIwdg(void)
{
    DriverStatus_e status;

    /* OpenOCD sets the freeze bits itself when it attaches: clear ours
     * first, so the read-back shows this driver's write. */
    DBGMCU->APB1_FZ &= ~DBGMCU_APB1_FZ_DBG_IWDG_STOP;
    iwdgFreezeInDebug(DBGMCU);
    check("iwdg_freeze_in_debug", (DBGMCU->APB1_FZ & DBGMCU_APB1_FZ_DBG_IWDG_STOP) != 0U, "APB1_FZ",
          DBGMCU->APB1_FZ);

    status = iwdgStart(IWDG, SMOKE_IWDG_PRESCALER, SMOKE_IWDG_RELOAD);
    check("iwdg_start", status == DRIVER_STATUS_OK, "status", (uint32_t)status);
    /* PR and RLR read back only if the unlock key was accepted. */
    check("iwdg_pr_rlr_readback",
          (IWDG->PR == SMOKE_IWDG_PRESCALER) && (IWDG->RLR == SMOKE_IWDG_RELOAD), "RLR", IWDG->RLR);

    /* 300 ms is longer than the longest timeout: still running proves the refreshes. */
    waitMs(300U, 20U);
    check("iwdg_refresh_keeps_alive", true, "", 0U);
}

/** Stop refreshing and wait well past the timeout; returning means the IWDG never fired. */
static void starveIwdg(void)
{
    print("iwdg: refresh stopped, a reset is due within 190 ms\r\n");
    if (uartFlush(USART2, SMOKE_UART_TIMEOUT) != DRIVER_STATUS_OK)
    {
        console_failed = true;
    }
    waitMs(1000U, 0U);
    check("iwdg_reset_on_starvation", false, "", 0U);
}

/**
 * Start the WWDG, refreshed from its early-wakeup interrupt, and show that
 * this holds it off for 200 ms: four early wakeups, 45.9 ms apart.
 */
static void checkWwdg(void)
{
    DriverStatus_e status;

    rccWwdgClockEnable(RCC);
    DBGMCU->APB1_FZ &= ~DBGMCU_APB1_FZ_DBG_WWDG_STOP;
    wwdgFreezeInDebug(DBGMCU);
    check("wwdg_freeze_in_debug", (DBGMCU->APB1_FZ & DBGMCU_APB1_FZ_DBG_WWDG_STOP) != 0U, "APB1_FZ",
          DBGMCU->APB1_FZ);

    status = wwdgStart(WWDG, SMOKE_WWDG_PRESCALER, SMOKE_WWDG_WINDOW, SMOKE_WWDG_COUNTER);
    check("wwdg_start", status == DRIVER_STATUS_OK, "status", (uint32_t)status);
    check("wwdg_cfr_readback",
          (WWDG->CFR & (WWDG_CFR_W_Msk | WWDG_CFR_WDGTB_Msk))
              == (SMOKE_WWDG_WINDOW | (SMOKE_WWDG_PRESCALER << WWDG_CFR_WDGTB_Pos)),
          "CFR", WWDG->CFR);

    wwdgEnableEarlyWakeup(WWDG);
    if (status == DRIVER_STATUS_OK)
    {
        status = nvicEnableIrq(NVIC, WWDG_IRQn);
    }

    waitMs(200U, 0U);
    check("wwdg_early_wakeup_refreshes", (status == DRIVER_STATUS_OK) && (wwdg_ewi_count == 4U),
          "wakeups", wwdg_ewi_count);
}

/**
 * Refresh with the counter above the window, which must reset the chip at
 * once. The early-wakeup refreshes keep running, so returning means the
 * window was not enforced.
 */
static void violateWwdgWindow(void)
{
    print("wwdg: refreshing above the window, a reset is due at once\r\n");
    if (uartFlush(USART2, SMOKE_UART_TIMEOUT) != DRIVER_STATUS_OK)
    {
        console_failed = true;
    }
    /* The second refresh comes microseconds after the first, at 0x7F > 0x5F. */
    if ((wwdgRefresh(WWDG, SMOKE_WWDG_COUNTER) == DRIVER_STATUS_OK)
        && (wwdgRefresh(WWDG, SMOKE_WWDG_COUNTER) == DRIVER_STATUS_OK))
    {
        waitMs(100U, 0U);
    }
    check("wwdg_reset_on_early_refresh", false, "", 0U);
}

/* --- main --------------------------------------------------------------- */

int main(void)
{
    DriverStatus_e clock_status;
    uint32_t blink_ms = 500U;
    uint32_t next_toggle;
    /* Read first: they accumulate across resets until cleared. */
    uint32_t reset_flags = rccResetFlags(RCC);
    SmokeBoot_e boot = smokeBoot(reset_flags);

    rccClearResetFlags(RCC);
    faultEnable(SCB);
    clock_status = bspClockInit(RCC, PWR, FLASH);

    if (boardIoInit() != DRIVER_STATUS_OK)
    {
        /* No console and maybe no LED: nothing to report through. */
        for (;;)
        {
        }
    }

    print("\r\nstm32_rtos smoke test, reset flags ");
    printHex(reset_flags);
    print("\r\n");
    checkClocks(clock_status);
    checkGpio();
    checkNvic();
    checkSystick();
    checkExti();

    switch (boot)
    {
        case SMOKE_BOOT_FIRST:
            checkIwdg();
            break;
        case SMOKE_BOOT_AFTER_IWDG:
            check("iwdg_reset_cause", true, "flags", reset_flags);
            checkWwdg();
            break;
        default:
            check("wwdg_reset_cause", true, "flags", reset_flags);
            break;
    }

    print(checks_failed == 0U ? "SMOKE PASS " : "SMOKE FAIL ");
    printDec(checks_run - checks_failed);
    print("/");
    printDec(checks_run);
    print("\r\n");

    /* The next reset only after a clean boot, so a failure stays on the console. */
    if (checks_failed == 0U)
    {
        if (boot == SMOKE_BOOT_FIRST)
        {
            starveIwdg();
        }
        else if (boot == SMOKE_BOOT_AFTER_IWDG)
        {
            violateWwdgWindow();
        }
        else
        {
            /* End of the chain. */
        }
    }

#if SMOKE_FAULT
    {
        volatile uint32_t divisor = 0U;

        print("fault: dividing by zero; read p/x faultRecord in GDB\r\n");
        if (uartFlush(USART2, SMOKE_UART_TIMEOUT) != DRIVER_STATUS_OK)
        {
            console_failed = true;
        }
        spare_irq_count = 100U / divisor;
    }
#endif

    if ((checks_failed != 0U) || console_failed)
    {
        blink_ms = 100U;
    }
    next_toggle = ticks + blink_ms;
    for (;;)
    {
        /* The IWDG still runs if the first boot failed; a refresh is harmless otherwise.
         * A running WWDG is refreshed by its early-wakeup handler. */
        iwdgRefresh(IWDG);
        if ((int32_t)(ticks - next_toggle) >= 0)
        {
            gpioTogglePin(GPIOA, GPIO_PIN_5);
            next_toggle += blink_ms;
        }
    }
}
