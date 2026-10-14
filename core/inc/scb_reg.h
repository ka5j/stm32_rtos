/**
 * @file scb_reg.h
 * @brief SCB (System Control Block) register definitions for the ARM
 *        Cortex-M4 (Armv7-M architecture), including the FPU context
 *        control registers.
 *
 * Register layout derived from PM0214 (Cortex-M4 programming manual).
 * Architectural rather than vendor-specific, fixed by Armv7-M's System
 * Control Space memory map. Covers exception/fault configuration and
 * status (ICSR, SCR, CCR, AIRCR, SHPR1-3, SHCSR, CFSR, HFSR, MMFAR,
 * BFAR), the EXC_RETURN values, and FPU enable/lazy-stacking control
 * (CPACR, FPCCR): what a HardFault handler and a PendSV context switch
 * need, plus the SHPR priority fields drivers/nvic.c already uses.
 *
 * The CPU feature/ID registers (PFR0/1, DFR0, AFR0, MMFR0-3, ISAR0-4 at
 * 0xE000ED40-0xE000ED84) are omitted, with RESERVED0 covering that
 * range, since this RTOS performs no runtime CPU-feature detection.
 *
 * FPCCR/FPCAR/FPDSCR reside at 0xE000EF34, not contiguous with the main
 * SCB block at 0xE000ED00 (the architecture places the MPU's register
 * bank, see mpu_reg.h, in between); this is why FpuRegisters_t is
 * defined as a separate struct with its own base address below.
 */
#ifndef SCB_REG_H
#define SCB_REG_H

#include <stdint.h>

/**
 * @addtogroup scb_registers
 * @{
 */

#define SCB_BASE (0xE000ED00UL) ///< SCB peripheral base address (Armv7-M SCS)

/** @} */

/**
 * @addtogroup fpu_registers
 * @{
 */

#define FPU_BASE (0xE000EF34UL) ///< FPU context control register base address (Armv7-M SCS)

/** @} */

/**
 * @addtogroup scb_registers
 * @{
 */

/**
 * @brief SCB register map (PM0214, Armv7-M SCS SCB region).
 *
 * RESERVED0 preserves the fixed architectural offset to CPACR (see the
 * top-of-file note above); this is an actual gap in the memory map, not
 * a struct-packing artifact.
 */
typedef struct ScbRegisters_t
{
    volatile const uint32_t CPUID; ///< 0x00: CPUID base register (READ only)
    volatile uint32_t ICSR;        ///< 0x04: interrupt control and state register
    volatile uint32_t VTOR;        ///< 0x08: vector table offset register
    volatile uint32_t AIRCR;       ///< 0x0C: application interrupt and reset control register
    volatile uint32_t SCR;         ///< 0x10: system control register
    volatile uint32_t CCR;         ///< 0x14: configuration and control register
    volatile uint32_t SHPR1; ///< 0x18: system handler priority register 1 (MemManage/Bus/Usage)
    volatile uint32_t SHPR2; ///< 0x1C: system handler priority register 2 (SVCall)
    volatile uint32_t SHPR3; ///< 0x20: system handler priority register 3 (PendSV/SysTick)
    volatile uint32_t SHCSR; ///< 0x24: system handler control and state register
    volatile uint32_t CFSR;  ///< 0x28: configurable fault status register (MMFSR/BFSR/UFSR)
    volatile uint32_t HFSR;  ///< 0x2C: HardFault status register
    volatile uint32_t DFSR;  ///< 0x30: debug fault status register
    volatile uint32_t MMFAR; ///< 0x34: MemManage fault address register
    volatile uint32_t BFAR;  ///< 0x38: BusFault address register
    volatile uint32_t AFSR;  ///< 0x3C: auxiliary fault status register
    volatile uint32_t RESERVED0[18]; ///< 0x40-0x84: CPU ID/feature registers, unused by this RTOS
    volatile uint32_t CPACR;         ///< 0x88: coprocessor access control register (FPU enable)
} ScbRegisters_t;

#define SCB ((ScbRegisters_t *)SCB_BASE) ///< Pointer to the SCB register block

/** @} */

/**
 * @addtogroup fpu_registers
 * @{
 */

/**
 * @brief FPU context control register map (PM0214, Armv7-M SCS FPU region).
 */
typedef struct FpuRegisters_t
{
    volatile uint32_t FPCCR;  ///< 0x00 (0xE000EF34): FP context control register
    volatile uint32_t FPCAR;  ///< 0x04 (0xE000EF38): FP context address register
    volatile uint32_t FPDSCR; ///< 0x08 (0xE000EF3C): FP default status control register
} FpuRegisters_t;

#define FPU ((FpuRegisters_t *)FPU_BASE) ///< Pointer to the FPU context control register block

/** @} */

/**
 * @addtogroup scb_registers
 * @{
 */

/* --- SCB_AIRCR bit definitions --- */
#define SCB_AIRCR_VECTRESET_Pos (0U) ///< Bit position within SCB_AIRCR
#define SCB_AIRCR_VECTRESET                                                                        \
    ((uint32_t)1U << SCB_AIRCR_VECTRESET_Pos) ///< WRITE - local reset, debug only
#define SCB_AIRCR_SYSRESETREQ_Pos (2U)        ///< Bit position within SCB_AIRCR
#define SCB_AIRCR_SYSRESETREQ                                                                      \
    ((uint32_t)1U << SCB_AIRCR_SYSRESETREQ_Pos) ///< WRITE - request system reset
#define SCB_AIRCR_PRIGROUP_Pos (8U)             ///< Bit position within SCB_AIRCR
#define SCB_AIRCR_PRIGROUP_Msk                                                                     \
    ((uint32_t)0x7U << SCB_AIRCR_PRIGROUP_Pos) ///< bits 10:8, priority grouping
#define SCB_AIRCR_VECTKEY_Pos (16U)            ///< Bit position within SCB_AIRCR
#define SCB_AIRCR_VECTKEY_Msk ((uint32_t)0xFFFFU << SCB_AIRCR_VECTKEY_Pos) ///< bits 31:16
#define SCB_AIRCR_VECTKEY_WRITE                                                                    \
    ((uint32_t)0x5FAU << SCB_AIRCR_VECTKEY_Pos) ///< required key to permit write

/* --- SCB_ICSR bit definitions --- */
#define SCB_ICSR_VECTACTIVE_Pos (0U) ///< Bit position within SCB_ICSR
#define SCB_ICSR_VECTACTIVE_Msk                                                                    \
    ((uint32_t)0x1FFU << SCB_ICSR_VECTACTIVE_Pos) ///< READ only, bits 8:0, active exception number
#define SCB_ICSR_PENDSTCLR_Pos (25U)              ///< Bit position within SCB_ICSR
#define SCB_ICSR_PENDSTCLR                                                                         \
    ((uint32_t)1U << SCB_ICSR_PENDSTCLR_Pos) ///< WRITE - clear SysTick pending
#define SCB_ICSR_PENDSTSET_Pos (26U)         ///< Bit position within SCB_ICSR
#define SCB_ICSR_PENDSTSET                                                                         \
    ((uint32_t)1U << SCB_ICSR_PENDSTSET_Pos) ///< READ/WRITE - set SysTick pending
#define SCB_ICSR_PENDSVCLR_Pos (27U)         ///< Bit position within SCB_ICSR
#define SCB_ICSR_PENDSVCLR                                                                         \
    ((uint32_t)1U << SCB_ICSR_PENDSVCLR_Pos) ///< WRITE - clear PendSV pending
#define SCB_ICSR_PENDSVSET_Pos (28U)         ///< Bit position within SCB_ICSR
#define SCB_ICSR_PENDSVSET                                                                         \
    ((uint32_t)1U << SCB_ICSR_PENDSVSET_Pos) ///< READ/WRITE - set PendSV pending

/* --- SCB_SCR bit definitions --- */
#define SCB_SCR_SLEEPONEXIT_Pos (1U) ///< Bit position within SCB_SCR
#define SCB_SCR_SLEEPONEXIT                                                                        \
    ((uint32_t)1U << SCB_SCR_SLEEPONEXIT_Pos) ///< WRITE - sleep on return to thread mode
#define SCB_SCR_SLEEPDEEP_Pos (2U)            ///< Bit position within SCB_SCR
#define SCB_SCR_SLEEPDEEP                                                                          \
    ((uint32_t)1U << SCB_SCR_SLEEPDEEP_Pos) ///< WRITE - WFI/WFE enters deep sleep
#define SCB_SCR_SEVONPEND_Pos (4U)          ///< Bit position within SCB_SCR
#define SCB_SCR_SEVONPEND                                                                          \
    ((uint32_t)1U << SCB_SCR_SEVONPEND_Pos) ///< WRITE - pended interrupt wakes WFE

/* --- SCB_CCR bit definitions --- */
#define SCB_CCR_NONBASETHRDENA_Pos (0U) ///< Bit position within SCB_CCR
#define SCB_CCR_NONBASETHRDENA                                                                     \
    ((uint32_t)1U << SCB_CCR_NONBASETHRDENA_Pos) ///< thread mode entry from ISR
#define SCB_CCR_USERSETMPEND_Pos (1U)            ///< Bit position within SCB_CCR
#define SCB_CCR_USERSETMPEND                                                                       \
    ((uint32_t)1U << SCB_CCR_USERSETMPEND_Pos) ///< WRITE - unprivileged STIR access
#define SCB_CCR_UNALIGN_TRP_Pos (3U)           ///< Bit position within SCB_CCR
#define SCB_CCR_UNALIGN_TRP                                                                        \
    ((uint32_t)1U << SCB_CCR_UNALIGN_TRP_Pos)                     ///< WRITE - trap unaligned access
#define SCB_CCR_DIV_0_TRP_Pos (4U)                                ///< Bit position within SCB_CCR
#define SCB_CCR_DIV_0_TRP ((uint32_t)1U << SCB_CCR_DIV_0_TRP_Pos) ///< WRITE - trap divide-by-zero
#define SCB_CCR_BFHFNMIGN_Pos (8U)                                ///< Bit position within SCB_CCR
#define SCB_CCR_BFHFNMIGN                                                                          \
    ((uint32_t)1U << SCB_CCR_BFHFNMIGN_Pos) ///< ignore faults in fault/NMI handlers
#define SCB_CCR_STKALIGN_Pos (9U)           ///< Bit position within SCB_CCR
#define SCB_CCR_STKALIGN                                                                           \
    ((uint32_t)1U << SCB_CCR_STKALIGN_Pos) ///< force 8-byte stack align on exception

/* --- SCB_SHCSR bit definitions --- */
#define SCB_SHCSR_MEMFAULTACT_Pos (0U) ///< Bit position within SCB_SHCSR
#define SCB_SHCSR_MEMFAULTACT                                                                      \
    ((uint32_t)1U << SCB_SHCSR_MEMFAULTACT_Pos) ///< READ/WRITE - MemManage active
#define SCB_SHCSR_BUSFAULTACT_Pos (1U)          ///< Bit position within SCB_SHCSR
#define SCB_SHCSR_BUSFAULTACT                                                                      \
    ((uint32_t)1U << SCB_SHCSR_BUSFAULTACT_Pos) ///< READ/WRITE - BusFault active
#define SCB_SHCSR_USGFAULTACT_Pos (3U)          ///< Bit position within SCB_SHCSR
#define SCB_SHCSR_USGFAULTACT                                                                      \
    ((uint32_t)1U << SCB_SHCSR_USGFAULTACT_Pos) ///< READ/WRITE - UsageFault active
#define SCB_SHCSR_SVCALLACT_Pos (7U)            ///< Bit position within SCB_SHCSR
#define SCB_SHCSR_SVCALLACT                                                                        \
    ((uint32_t)1U << SCB_SHCSR_SVCALLACT_Pos) ///< READ/WRITE - SVCall active
#define SCB_SHCSR_PENDSVACT_Pos (10U)         ///< Bit position within SCB_SHCSR
#define SCB_SHCSR_PENDSVACT                                                                        \
    ((uint32_t)1U << SCB_SHCSR_PENDSVACT_Pos) ///< READ/WRITE - PendSV active
#define SCB_SHCSR_SYSTICKACT_Pos (11U)        ///< Bit position within SCB_SHCSR
#define SCB_SHCSR_SYSTICKACT                                                                       \
    ((uint32_t)1U << SCB_SHCSR_SYSTICKACT_Pos) ///< READ/WRITE - SysTick active
#define SCB_SHCSR_USGFAULTPENDED_Pos (12U)     ///< Bit position within SCB_SHCSR
#define SCB_SHCSR_USGFAULTPENDED                                                                   \
    ((uint32_t)1U << SCB_SHCSR_USGFAULTPENDED_Pos) ///< READ/WRITE - UsageFault pended
#define SCB_SHCSR_MEMFAULTPENDED_Pos (13U)         ///< Bit position within SCB_SHCSR
#define SCB_SHCSR_MEMFAULTPENDED                                                                   \
    ((uint32_t)1U << SCB_SHCSR_MEMFAULTPENDED_Pos) ///< READ/WRITE - MemManage pended
#define SCB_SHCSR_BUSFAULTPENDED_Pos (14U)         ///< Bit position within SCB_SHCSR
#define SCB_SHCSR_BUSFAULTPENDED                                                                   \
    ((uint32_t)1U << SCB_SHCSR_BUSFAULTPENDED_Pos) ///< READ/WRITE - BusFault pended
#define SCB_SHCSR_SVCALLPENDED_Pos (15U)           ///< Bit position within SCB_SHCSR
#define SCB_SHCSR_SVCALLPENDED                                                                     \
    ((uint32_t)1U << SCB_SHCSR_SVCALLPENDED_Pos) ///< READ/WRITE - SVCall pended
#define SCB_SHCSR_MEMFAULTENA_Pos (16U)          ///< Bit position within SCB_SHCSR
#define SCB_SHCSR_MEMFAULTENA                                                                      \
    ((uint32_t)1U << SCB_SHCSR_MEMFAULTENA_Pos) ///< WRITE - enable MemManage
#define SCB_SHCSR_BUSFAULTENA_Pos (17U)         ///< Bit position within SCB_SHCSR
#define SCB_SHCSR_BUSFAULTENA                                                                      \
    ((uint32_t)1U << SCB_SHCSR_BUSFAULTENA_Pos) ///< WRITE - enable BusFault
#define SCB_SHCSR_USGFAULTENA_Pos (18U)         ///< Bit position within SCB_SHCSR
#define SCB_SHCSR_USGFAULTENA                                                                      \
    ((uint32_t)1U << SCB_SHCSR_USGFAULTENA_Pos) ///< WRITE - enable UsageFault

/* --- SCB_CFSR bit definitions (MMFSR: bits 7:0, BFSR: bits 15:8, UFSR: bits 31:16) --- */
#define SCB_CFSR_IACCVIOL_Pos (0U) ///< Bit position within SCB_CFSR (MMFSR)
#define SCB_CFSR_IACCVIOL                                                                          \
    ((uint32_t)1U << SCB_CFSR_IACCVIOL_Pos) ///< READ/WRITE 1 to clear - instr access violation
#define SCB_CFSR_DACCVIOL_Pos (1U)          ///< Bit position within SCB_CFSR (MMFSR)
#define SCB_CFSR_DACCVIOL                                                                          \
    ((uint32_t)1U << SCB_CFSR_DACCVIOL_Pos) ///< READ/WRITE 1 to clear - data access violation
#define SCB_CFSR_MUNSTKERR_Pos (3U)         ///< Bit position within SCB_CFSR (MMFSR)
#define SCB_CFSR_MUNSTKERR                                                                         \
    ((uint32_t)1U << SCB_CFSR_MUNSTKERR_Pos) ///< READ/WRITE 1 to clear - fault on exc. return
#define SCB_CFSR_MSTKERR_Pos (4U)            ///< Bit position within SCB_CFSR (MMFSR)
#define SCB_CFSR_MSTKERR                                                                           \
    ((uint32_t)1U << SCB_CFSR_MSTKERR_Pos) ///< READ/WRITE 1 to clear - fault on exception entry
#define SCB_CFSR_MLSPERR_Pos (5U)          ///< Bit position within SCB_CFSR (MMFSR)
#define SCB_CFSR_MLSPERR                                                                           \
    ((uint32_t)1U                                                                                  \
     << SCB_CFSR_MLSPERR_Pos)       ///< READ/WRITE 1 to clear - fault on FP lazy state preservation
#define SCB_CFSR_MMARVALID_Pos (7U) ///< Bit position within SCB_CFSR (MMFSR)
#define SCB_CFSR_MMARVALID                                                                         \
    ((uint32_t)1U << SCB_CFSR_MMARVALID_Pos) ///< READ/WRITE 1 to clear - MMFAR is valid
#define SCB_CFSR_IBUSERR_Pos (8U)            ///< Bit position within SCB_CFSR (BFSR)
#define SCB_CFSR_IBUSERR                                                                           \
    ((uint32_t)1U << SCB_CFSR_IBUSERR_Pos) ///< READ/WRITE 1 to clear - fault on instr fetch
#define SCB_CFSR_PRECISERR_Pos (9U)        ///< Bit position within SCB_CFSR (BFSR)
#define SCB_CFSR_PRECISERR                                                                         \
    ((uint32_t)1U << SCB_CFSR_PRECISERR_Pos) ///< READ/WRITE 1 to clear - precise err, BFAR valid
#define SCB_CFSR_IMPRECISERR_Pos (10U)       ///< Bit position within SCB_CFSR (BFSR)
#define SCB_CFSR_IMPRECISERR                                                                       \
    ((uint32_t)1U << SCB_CFSR_IMPRECISERR_Pos) ///< READ/WRITE 1 to clear - imprecise bus error
#define SCB_CFSR_UNSTKERR_Pos (11U)            ///< Bit position within SCB_CFSR (BFSR)
#define SCB_CFSR_UNSTKERR                                                                          \
    ((uint32_t)1U << SCB_CFSR_UNSTKERR_Pos) ///< READ/WRITE 1 to clear - fault on exception return
#define SCB_CFSR_STKERR_Pos (12U)           ///< Bit position within SCB_CFSR (BFSR)
#define SCB_CFSR_STKERR                                                                            \
    ((uint32_t)1U << SCB_CFSR_STKERR_Pos) ///< READ/WRITE 1 to clear - fault on exception entry
#define SCB_CFSR_LSPERR_Pos (13U)         ///< Bit position within SCB_CFSR (BFSR)
#define SCB_CFSR_LSPERR                                                                            \
    ((uint32_t)1U                                                                                  \
     << SCB_CFSR_LSPERR_Pos) ///< READ/WRITE 1 to clear - fault on FP lazy state preservation
#define SCB_CFSR_BFARVALID_Pos (15U) ///< Bit position within SCB_CFSR (BFSR)
#define SCB_CFSR_BFARVALID                                                                         \
    ((uint32_t)1U << SCB_CFSR_BFARVALID_Pos) ///< READ/WRITE 1 to clear - BFAR is valid
#define SCB_CFSR_UNDEFINSTR_Pos (16U)        ///< Bit position within SCB_CFSR (UFSR)
#define SCB_CFSR_UNDEFINSTR                                                                        \
    ((uint32_t)1U << SCB_CFSR_UNDEFINSTR_Pos) ///< READ/WRITE 1 to clear - undefined instruction
#define SCB_CFSR_INVSTATE_Pos (17U)           ///< Bit position within SCB_CFSR (UFSR)
#define SCB_CFSR_INVSTATE                                                                          \
    ((uint32_t)1U << SCB_CFSR_INVSTATE_Pos) ///< READ/WRITE 1 to clear - invalid EPSR state
#define SCB_CFSR_INVPC_Pos (18U)            ///< Bit position within SCB_CFSR (UFSR)
#define SCB_CFSR_INVPC                                                                             \
    ((uint32_t)1U << SCB_CFSR_INVPC_Pos) ///< READ/WRITE 1 to clear - invalid PC load/exc return
#define SCB_CFSR_NOCP_Pos (19U)          ///< Bit position within SCB_CFSR (UFSR)
#define SCB_CFSR_NOCP                                                                              \
    ((uint32_t)1U << SCB_CFSR_NOCP_Pos) ///< READ/WRITE 1 to clear - coprocessor access denied
#define SCB_CFSR_UNALIGNED_Pos (24U)    ///< Bit position within SCB_CFSR (UFSR)
#define SCB_CFSR_UNALIGNED                                                                         \
    ((uint32_t)1U << SCB_CFSR_UNALIGNED_Pos) ///< READ/WRITE 1 to clear - unaligned access trap
#define SCB_CFSR_DIVBYZERO_Pos (25U)         ///< Bit position within SCB_CFSR (UFSR)
#define SCB_CFSR_DIVBYZERO                                                                         \
    ((uint32_t)1U << SCB_CFSR_DIVBYZERO_Pos) ///< READ/WRITE 1 to clear - divide-by-zero trap

/* --- SCB_HFSR bit definitions --- */
#define SCB_HFSR_VECTTBL_Pos (1U) ///< Bit position within SCB_HFSR
#define SCB_HFSR_VECTTBL                                                                           \
    ((uint32_t)1U << SCB_HFSR_VECTTBL_Pos) ///< READ/WRITE 1 to clear - fault reading vector table
#define SCB_HFSR_FORCED_Pos (30U)          ///< Bit position within SCB_HFSR
#define SCB_HFSR_FORCED                                                                            \
    ((uint32_t)1U << SCB_HFSR_FORCED_Pos) ///< READ/WRITE 1 to clear - fault escalated to HardFault
#define SCB_HFSR_DEBUGEVT_Pos (31U)       ///< Bit position within SCB_HFSR
#define SCB_HFSR_DEBUGEVT                                                                          \
    ((uint32_t)1U << SCB_HFSR_DEBUGEVT_Pos) ///< READ/WRITE 1 to clear - debug event

/* --- SCB_SHPR1 bit definitions (system handler priority fields) --- */
#define SCB_SHPR1_PRI_4_Pos (0U) ///< Bit position within SCB_SHPR1
#define SCB_SHPR1_PRI_4_Msk                                                                        \
    ((uint32_t)0xFFU << SCB_SHPR1_PRI_4_Pos) ///< bits 7:0, MemManage priority
#define SCB_SHPR1_PRI_5_Pos (8U)             ///< Bit position within SCB_SHPR1
#define SCB_SHPR1_PRI_5_Msk                                                                        \
    ((uint32_t)0xFFU << SCB_SHPR1_PRI_5_Pos) ///< bits 15:8, BusFault priority
#define SCB_SHPR1_PRI_6_Pos (16U)            ///< Bit position within SCB_SHPR1
#define SCB_SHPR1_PRI_6_Msk                                                                        \
    ((uint32_t)0xFFU << SCB_SHPR1_PRI_6_Pos) ///< bits 23:16, UsageFault priority

/* --- SCB_SHPR2/SHPR3 bit definitions (system handler priority fields) --- */
#define SCB_SHPR2_PRI_11_Pos (24U) ///< Bit position within SCB_SHPR2
#define SCB_SHPR2_PRI_11_Msk                                                                       \
    ((uint32_t)0xFFU << SCB_SHPR2_PRI_11_Pos) ///< bits 31:24, SVCall priority
#define SCB_SHPR3_PRI_14_Pos (16U)            ///< Bit position within SCB_SHPR3
#define SCB_SHPR3_PRI_14_Msk                                                                       \
    ((uint32_t)0xFFU << SCB_SHPR3_PRI_14_Pos) ///< bits 23:16, PendSV priority
#define SCB_SHPR3_PRI_15_Pos (24U)            ///< Bit position within SCB_SHPR3
#define SCB_SHPR3_PRI_15_Msk                                                                       \
    ((uint32_t)0xFFU << SCB_SHPR3_PRI_15_Pos) ///< bits 31:24, SysTick priority

/* --- EXC_RETURN values (LR on exception entry; PM0214 Table 2-17) --- */
#define EXC_RETURN_HANDLER (0xFFFFFFF1UL)     ///< return to Handler mode, MSP, basic frame
#define EXC_RETURN_THREAD_MSP (0xFFFFFFF9UL)  ///< return to Thread mode, MSP, basic frame
#define EXC_RETURN_THREAD_PSP (0xFFFFFFFDUL)  ///< return to Thread mode, PSP, basic frame
#define EXC_RETURN_HANDLER_FPU (0xFFFFFFE1UL) ///< return to Handler mode, MSP, extended (FPU) frame
#define EXC_RETURN_THREAD_MSP_FPU                                                                  \
    (0xFFFFFFE9UL) ///< return to Thread mode, MSP, extended (FPU) frame
#define EXC_RETURN_THREAD_PSP_FPU                                                                  \
    (0xFFFFFFEDUL) ///< return to Thread mode, PSP, extended (FPU) frame

/* --- SCB_CPACR bit definitions --- */
#define SCB_CPACR_CP10_Pos (20U) ///< Bit position within SCB_CPACR
#define SCB_CPACR_CP10_Msk                                                                         \
    ((uint32_t)0x3U << SCB_CPACR_CP10_Pos) ///< bits 21:20, FPU access, 0b11=full
#define SCB_CPACR_CP11_Pos (22U)           ///< Bit position within SCB_CPACR
#define SCB_CPACR_CP11_Msk                                                                         \
    ((uint32_t)0x3U << SCB_CPACR_CP11_Pos) ///< bits 23:22, FPU access, 0b11=full

/** @} */

/**
 * @addtogroup fpu_registers
 * @{
 */

/* --- FPU_FPCCR bit definitions --- */
#define FPU_FPCCR_LSPACT_Pos (0U) ///< Bit position within FPU_FPCCR
#define FPU_FPCCR_LSPACT                                                                           \
    ((uint32_t)1U << FPU_FPCCR_LSPACT_Pos) ///< READ only - lazy state preservation active
#define FPU_FPCCR_ASPEN_Pos (31U)          ///< Bit position within FPU_FPCCR
#define FPU_FPCCR_ASPEN                                                                            \
    ((uint32_t)1U << FPU_FPCCR_ASPEN_Pos) ///< WRITE - auto FP state save on exception
#define FPU_FPCCR_LSPEN_Pos (30U)         ///< Bit position within FPU_FPCCR
#define FPU_FPCCR_LSPEN                                                                            \
    ((uint32_t)1U << FPU_FPCCR_LSPEN_Pos) ///< WRITE - enable lazy FP context stacking

/** @} */

#endif /* SCB_REG_H */
