/**
 * @file test_scb_reg.c
 * @brief Host-side sanity checks for core/inc/scb_reg.h (including its
 *        FpuRegisters_t). Pure data validation - no hardware, no driver
 *        logic. Compiled into tests/unit/test_runner.c's run_tests binary.
 */
#include "scb_reg.h"
#include "unity.h"

#include <stddef.h>

/**
 * ScbRegisters_t must be exactly 0x8C bytes (CPUID..CPACR, PM0214) with
 * CPACR landing at its fixed offset past the CPU-ID register block.
 */
void test_scb_register_block_size_and_offset(void)
{
    TEST_ASSERT_EQUAL_UINT(0x8C, sizeof(ScbRegisters_t));
    TEST_ASSERT_EQUAL_HEX32(0x88, offsetof(ScbRegisters_t, CPACR));
}

/** FpuRegisters_t must be exactly 3 words (FPCCR, FPCAR, FPDSCR). */
void test_fpu_register_block_size(void)
{
    TEST_ASSERT_EQUAL_UINT(0xC, sizeof(FpuRegisters_t));
}

/** SCB_CFSR's MMFSR/BFSR/UFSR byte fields must not overlap. */
void test_scb_cfsr_fault_status_byte_fields_do_not_overlap(void)
{
    uint32_t mmfsr_bits = SCB_CFSR_IACCVIOL | SCB_CFSR_DACCVIOL | SCB_CFSR_MUNSTKERR
                          | SCB_CFSR_MSTKERR | SCB_CFSR_MLSPERR | SCB_CFSR_MMARVALID;
    uint32_t bfsr_bits = SCB_CFSR_IBUSERR | SCB_CFSR_PRECISERR | SCB_CFSR_IMPRECISERR
                         | SCB_CFSR_UNSTKERR | SCB_CFSR_STKERR | SCB_CFSR_LSPERR
                         | SCB_CFSR_BFARVALID;

    TEST_ASSERT_EQUAL_HEX32(0x00U, mmfsr_bits & 0xFFFFFF00U);
    TEST_ASSERT_EQUAL_HEX32(0x00U, bfsr_bits & 0xFFFF00FFU);
    TEST_ASSERT_EQUAL_HEX32(0x00U, mmfsr_bits & bfsr_bits);
}

/** SCB_ICSR's PendSV/SysTick set and clear bits sit at their PM0214 positions and are distinct. */
void test_scb_icsr_pend_bits(void)
{
    TEST_ASSERT_EQUAL_HEX32(1UL << 28, SCB_ICSR_PENDSVSET);
    TEST_ASSERT_EQUAL_HEX32(1UL << 27, SCB_ICSR_PENDSVCLR);
    TEST_ASSERT_EQUAL_HEX32(1UL << 26, SCB_ICSR_PENDSTSET);
    TEST_ASSERT_EQUAL_HEX32(1UL << 25, SCB_ICSR_PENDSTCLR);
    TEST_ASSERT_EQUAL_HEX32(0x1FFUL, SCB_ICSR_VECTACTIVE_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x00U, SCB_ICSR_VECTACTIVE_Msk
                                       & (SCB_ICSR_PENDSVSET | SCB_ICSR_PENDSVCLR
                                          | SCB_ICSR_PENDSTSET | SCB_ICSR_PENDSTCLR));
}

/** SCB_SCR and SCB_CCR additions are single, distinct bits at their PM0214 positions. */
void test_scb_scr_and_ccr_bits(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x02U, SCB_SCR_SLEEPONEXIT);
    TEST_ASSERT_EQUAL_HEX32(0x04U, SCB_SCR_SLEEPDEEP);
    TEST_ASSERT_EQUAL_HEX32(0x10U, SCB_SCR_SEVONPEND);
    TEST_ASSERT_EQUAL_HEX32(0x02U, SCB_CCR_USERSETMPEND);
    TEST_ASSERT_EQUAL_HEX32(0x00U, SCB_CCR_USERSETMPEND & SCB_CCR_NONBASETHRDENA);
}

/** SCB_SHCSR's active, pended and enable bits must not overlap. */
void test_scb_shcsr_bits_do_not_overlap(void)
{
    uint32_t active = SCB_SHCSR_MEMFAULTACT | SCB_SHCSR_BUSFAULTACT | SCB_SHCSR_USGFAULTACT
                      | SCB_SHCSR_SVCALLACT | SCB_SHCSR_PENDSVACT | SCB_SHCSR_SYSTICKACT;
    uint32_t pended = SCB_SHCSR_USGFAULTPENDED | SCB_SHCSR_MEMFAULTPENDED | SCB_SHCSR_BUSFAULTPENDED
                      | SCB_SHCSR_SVCALLPENDED;
    uint32_t enable = SCB_SHCSR_MEMFAULTENA | SCB_SHCSR_BUSFAULTENA | SCB_SHCSR_USGFAULTENA;

    TEST_ASSERT_EQUAL_HEX32(0x0C8BU, active);
    TEST_ASSERT_EQUAL_HEX32(0xF000U, pended);
    TEST_ASSERT_EQUAL_HEX32(0x70000U, enable);
}

/** SHPR1-3 priority byte fields must tile their registers without overlapping. */
void test_scb_shpr_fields_do_not_overlap(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x00FFFFFFU,
                            SCB_SHPR1_PRI_4_Msk | SCB_SHPR1_PRI_5_Msk | SCB_SHPR1_PRI_6_Msk);
    TEST_ASSERT_EQUAL_HEX32(0xFF000000U, SCB_SHPR3_PRI_15_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x00U, SCB_SHPR3_PRI_14_Msk & SCB_SHPR3_PRI_15_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x00U, SCB_SHPR1_PRI_4_Msk & SCB_SHPR1_PRI_5_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x00U, SCB_SHPR1_PRI_5_Msk & SCB_SHPR1_PRI_6_Msk);
}

/** EXC_RETURN values match PM0214: bit 4 clear selects the extended (FPU) frame. */
void test_scb_exc_return_values(void)
{
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFF1UL, EXC_RETURN_HANDLER);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFF9UL, EXC_RETURN_THREAD_MSP);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFDUL, EXC_RETURN_THREAD_PSP);
    TEST_ASSERT_EQUAL_HEX32(EXC_RETURN_HANDLER & ~(1UL << 4), EXC_RETURN_HANDLER_FPU);
    TEST_ASSERT_EQUAL_HEX32(EXC_RETURN_THREAD_MSP & ~(1UL << 4), EXC_RETURN_THREAD_MSP_FPU);
    TEST_ASSERT_EQUAL_HEX32(EXC_RETURN_THREAD_PSP & ~(1UL << 4), EXC_RETURN_THREAD_PSP_FPU);
}

/** Literal addresses, offsets and encodings (from ARM's CMSIS core_cm4.h), including the constants
 *  startup_stm32f446re.s hard-codes (CPACR address, FPU enable mask, AIRCR address, PRIGROUP write)
 *  so the assembly and the header cannot drift apart unnoticed. */
void test_scb_reg_literal_pins(void)
{
    TEST_ASSERT_EQUAL_HEX32(0xE000ED00UL, SCB_BASE);
    TEST_ASSERT_EQUAL_HEX32(SCB_BASE, (uintptr_t)SCB);
    TEST_ASSERT_EQUAL_HEX32(0xE000EF34UL, FPU_BASE);
    TEST_ASSERT_EQUAL_HEX32(FPU_BASE, (uintptr_t)FPU);
    TEST_ASSERT_EQUAL_HEX32(0x04, offsetof(ScbRegisters_t, ICSR));
    TEST_ASSERT_EQUAL_HEX32(0x08, offsetof(ScbRegisters_t, VTOR));
    TEST_ASSERT_EQUAL_HEX32(0x0C, offsetof(ScbRegisters_t, AIRCR));
    TEST_ASSERT_EQUAL_HEX32(0x10, offsetof(ScbRegisters_t, SCR));
    TEST_ASSERT_EQUAL_HEX32(0x14, offsetof(ScbRegisters_t, CCR));
    TEST_ASSERT_EQUAL_HEX32(0x18, offsetof(ScbRegisters_t, SHPR1));
    TEST_ASSERT_EQUAL_HEX32(0x1C, offsetof(ScbRegisters_t, SHPR2));
    TEST_ASSERT_EQUAL_HEX32(0x20, offsetof(ScbRegisters_t, SHPR3));
    TEST_ASSERT_EQUAL_HEX32(0x24, offsetof(ScbRegisters_t, SHCSR));
    TEST_ASSERT_EQUAL_HEX32(0x28, offsetof(ScbRegisters_t, CFSR));
    TEST_ASSERT_EQUAL_HEX32(0x2C, offsetof(ScbRegisters_t, HFSR));
    TEST_ASSERT_EQUAL_HEX32(0x30, offsetof(ScbRegisters_t, DFSR));
    TEST_ASSERT_EQUAL_HEX32(0x34, offsetof(ScbRegisters_t, MMFAR));
    TEST_ASSERT_EQUAL_HEX32(0x38, offsetof(ScbRegisters_t, BFAR));
    TEST_ASSERT_EQUAL_HEX32(0x3C, offsetof(ScbRegisters_t, AFSR));
    TEST_ASSERT_EQUAL_HEX32(0x04, offsetof(FpuRegisters_t, FPCAR));
    TEST_ASSERT_EQUAL_HEX32(0x08, offsetof(FpuRegisters_t, FPDSCR));

    /* startup_stm32f446re.s: ldr r0, =0xE000ED88 ; orr 0x00F00000 (CP10|CP11 full access) */
    TEST_ASSERT_EQUAL_HEX32(0xE000ED88UL, SCB_BASE + offsetof(ScbRegisters_t, CPACR));
    TEST_ASSERT_EQUAL_HEX32(0x00F00000U, SCB_CPACR_CP10_Msk | SCB_CPACR_CP11_Msk);
    /* startup_stm32f446re.s: AIRCR at 0xE000ED0C, clear 0xFFFF0700, write 0x05FA0300 (PRIGROUP=3)
     */
    TEST_ASSERT_EQUAL_HEX32(0xE000ED0CUL, SCB_BASE + offsetof(ScbRegisters_t, AIRCR));
    TEST_ASSERT_EQUAL_HEX32(0xFFFF0700U, SCB_AIRCR_VECTKEY_Msk | SCB_AIRCR_PRIGROUP_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x05FA0000U, SCB_AIRCR_VECTKEY_WRITE);
    TEST_ASSERT_EQUAL_HEX32(0x05FA0300U, SCB_AIRCR_VECTKEY_WRITE | (3U << SCB_AIRCR_PRIGROUP_Pos));
    TEST_ASSERT_EQUAL_HEX32(0x00000700U, SCB_AIRCR_PRIGROUP_Msk);

    TEST_ASSERT_EQUAL_HEX32(0xFF000000U, SCB_SHPR2_PRI_11_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x00FF0000U, SCB_SHPR3_PRI_14_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x00300000U, SCB_CPACR_CP10_Msk);
    TEST_ASSERT_EQUAL_HEX32(0x00C00000U, SCB_CPACR_CP11_Msk);
}
