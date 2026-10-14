/**
 * @file scb_reg.h
 * @brief SCB and FPU context-control registers (PM0214, SCB and FPU chapters).
 *
 * Covers what fault handling and a PendSV context switch need: exception
 * control and status, fault status, handler priorities, EXC_RETURN values
 * and FPU enable/lazy stacking. The CPU feature registers (0x40-0x84) are
 * not modelled. FPCCR/FPCAR/FPDSCR live at 0xE000EF34, past the MPU, so they
 * have their own block.
 */
#ifndef SCB_REG_H
#define SCB_REG_H

#include <stdint.h>

/**
 * @addtogroup scb_registers
 * @{
 */

#define SCB_BASE (0xE000ED00UL) ///< SCB base address (System Control Space)

/** @} */

/**
 * @addtogroup fpu_registers
 * @{
 */

#define FPU_BASE (0xE000EF34UL) ///< FPU context-control base address (System Control Space)

/** @} */

/**
 * @addtogroup scb_registers
 * @{
 */

/** @brief SCB register block. */
typedef struct ScbRegisters_t
{
    volatile const uint32_t CPUID;   ///< 0x00: CPU ID, \regaccess{ro}
    volatile uint32_t ICSR;          ///< 0x04: interrupt control and state
    volatile uint32_t VTOR;          ///< 0x08: vector table offset
    volatile uint32_t AIRCR;         ///< 0x0C: application interrupt and reset control
    volatile uint32_t SCR;           ///< 0x10: system control (sleep)
    volatile uint32_t CCR;           ///< 0x14: configuration and control
    volatile uint32_t SHPR1;         ///< 0x18: priority of MemManage, BusFault, UsageFault
    volatile uint32_t SHPR2;         ///< 0x1C: priority of SVCall
    volatile uint32_t SHPR3;         ///< 0x20: priority of PendSV, SysTick
    volatile uint32_t SHCSR;         ///< 0x24: system handler control and state
    volatile uint32_t CFSR;          ///< 0x28: configurable fault status (MMFSR, BFSR, UFSR)
    volatile uint32_t HFSR;          ///< 0x2C: HardFault status
    volatile uint32_t DFSR;          ///< 0x30: debug fault status
    volatile uint32_t MMFAR;         ///< 0x34: MemManage fault address
    volatile uint32_t BFAR;          ///< 0x38: BusFault address
    volatile uint32_t AFSR;          ///< 0x3C: auxiliary fault status
    volatile uint32_t RESERVED0[18]; ///< 0x40-0x84: CPU feature registers, not modelled
    volatile uint32_t CPACR;         ///< 0x88: coprocessor access control (FPU enable)
} ScbRegisters_t;

#define SCB ((ScbRegisters_t *)SCB_BASE) ///< The SCB instance

/** @} */

/**
 * @addtogroup fpu_registers
 * @{
 */

/** @brief FPU context-control register block. */
typedef struct FpuRegisters_t
{
    volatile uint32_t FPCCR;  ///< 0x00 (0xE000EF34): context control
    volatile uint32_t FPCAR;  ///< 0x04 (0xE000EF38): context address
    volatile uint32_t FPDSCR; ///< 0x08 (0xE000EF3C): default status control
} FpuRegisters_t;

#define FPU ((FpuRegisters_t *)FPU_BASE) ///< The FPU context-control instance

/** @} */

/**
 * @addtogroup scb_registers
 * @{
 */

/** @name AIRCR - application interrupt and reset control (0x0C)
 *  Every write must carry SCB_AIRCR_VECTKEY_WRITE or it is ignored.
 *  @{ */
#define SCB_AIRCR_VECTRESET_Pos (0U) ///< Bit 0
/** \regaccess{wo} Local reset; for debug use only. */
#define SCB_AIRCR_VECTRESET ((uint32_t)1U << SCB_AIRCR_VECTRESET_Pos)
#define SCB_AIRCR_SYSRESETREQ_Pos (2U) ///< Bit 2
/** \regaccess{wo} Request a system reset. */
#define SCB_AIRCR_SYSRESETREQ ((uint32_t)1U << SCB_AIRCR_SYSRESETREQ_Pos)
#define SCB_AIRCR_PRIGROUP_Pos (8U) ///< Bits 10:8
/** \regaccess{rw} Priority grouping; Reset_Handler sets 3 (all 4 bits preempt). */
#define SCB_AIRCR_PRIGROUP_Msk ((uint32_t)0x7U << SCB_AIRCR_PRIGROUP_Pos)
#define SCB_AIRCR_VECTKEY_Pos (16U) ///< Bits 31:16
/** \regaccess{rw} Write key; reads back 0xFA05. */
#define SCB_AIRCR_VECTKEY_Msk ((uint32_t)0xFFFFU << SCB_AIRCR_VECTKEY_Pos)
/** Key value that makes an AIRCR write take effect. */
#define SCB_AIRCR_VECTKEY_WRITE ((uint32_t)0x5FAU << SCB_AIRCR_VECTKEY_Pos)
/** @} */

/** @name ICSR - interrupt control and state (0x04)
 *  @{ */
#define SCB_ICSR_VECTACTIVE_Pos (0U) ///< Bits 8:0
/** \regaccess{ro} Number of the active exception (0 in Thread mode). */
#define SCB_ICSR_VECTACTIVE_Msk ((uint32_t)0x1FFU << SCB_ICSR_VECTACTIVE_Pos)
#define SCB_ICSR_PENDSTCLR_Pos (25U) ///< Bit 25
/** \regaccess{wo} Clear a pending SysTick. */
#define SCB_ICSR_PENDSTCLR ((uint32_t)1U << SCB_ICSR_PENDSTCLR_Pos)
#define SCB_ICSR_PENDSTSET_Pos (26U) ///< Bit 26
/** \regaccess{rw} Pend SysTick; reads 1 while pending. */
#define SCB_ICSR_PENDSTSET ((uint32_t)1U << SCB_ICSR_PENDSTSET_Pos)
#define SCB_ICSR_PENDSVCLR_Pos (27U) ///< Bit 27
/** \regaccess{wo} Clear a pending PendSV. */
#define SCB_ICSR_PENDSVCLR ((uint32_t)1U << SCB_ICSR_PENDSVCLR_Pos)
#define SCB_ICSR_PENDSVSET_Pos (28U) ///< Bit 28
/** \regaccess{rw} Pend PendSV (requests a context switch); reads 1 while pending. */
#define SCB_ICSR_PENDSVSET ((uint32_t)1U << SCB_ICSR_PENDSVSET_Pos)
/** @} */

/** @name SCR - system control (0x10)
 *  Every bit is \regaccess{rw}.
 *  @{ */
#define SCB_SCR_SLEEPONEXIT_Pos (1U) ///< Bit 1
/** Sleep on return from an exception to Thread mode. */
#define SCB_SCR_SLEEPONEXIT ((uint32_t)1U << SCB_SCR_SLEEPONEXIT_Pos)
#define SCB_SCR_SLEEPDEEP_Pos (2U)                                ///< Bit 2
#define SCB_SCR_SLEEPDEEP ((uint32_t)1U << SCB_SCR_SLEEPDEEP_Pos) ///< WFI/WFE enters deep sleep
#define SCB_SCR_SEVONPEND_Pos (4U)                                ///< Bit 4
#define SCB_SCR_SEVONPEND ((uint32_t)1U << SCB_SCR_SEVONPEND_Pos) ///< A pended interrupt wakes WFE
/** @} */

/** @name CCR - configuration and control (0x14)
 *  Every bit is \regaccess{rw}.
 *  @{ */
#define SCB_CCR_NONBASETHRDENA_Pos (0U) ///< Bit 0
/** Allow Thread mode entry from any exception level. */
#define SCB_CCR_NONBASETHRDENA ((uint32_t)1U << SCB_CCR_NONBASETHRDENA_Pos)
#define SCB_CCR_USERSETMPEND_Pos (1U) ///< Bit 1
/** Allow unprivileged code to write STIR. */
#define SCB_CCR_USERSETMPEND ((uint32_t)1U << SCB_CCR_USERSETMPEND_Pos)
#define SCB_CCR_UNALIGN_TRP_Pos (3U)                                  ///< Bit 3
#define SCB_CCR_UNALIGN_TRP ((uint32_t)1U << SCB_CCR_UNALIGN_TRP_Pos) ///< Trap unaligned accesses
#define SCB_CCR_DIV_0_TRP_Pos (4U)                                    ///< Bit 4
#define SCB_CCR_DIV_0_TRP ((uint32_t)1U << SCB_CCR_DIV_0_TRP_Pos)     ///< Trap divide by zero
#define SCB_CCR_BFHFNMIGN_Pos (8U)                                    ///< Bit 8
/** Ignore data bus faults in HardFault, NMI and FAULTMASK handlers. */
#define SCB_CCR_BFHFNMIGN ((uint32_t)1U << SCB_CCR_BFHFNMIGN_Pos)
#define SCB_CCR_STKALIGN_Pos (9U) ///< Bit 9
/** Align the stack to 8 bytes on exception entry. */
#define SCB_CCR_STKALIGN ((uint32_t)1U << SCB_CCR_STKALIGN_Pos)
/** @} */

/** @name SHCSR - system handler control and state (0x24)
 *  Every bit is \regaccess{rw}: ACT = active, PENDED = pending, ENA = enabled.
 *  @{ */
#define SCB_SHCSR_MEMFAULTACT_Pos (0U) ///< Bit 0
/** MemManage active. */
#define SCB_SHCSR_MEMFAULTACT ((uint32_t)1U << SCB_SHCSR_MEMFAULTACT_Pos)
#define SCB_SHCSR_BUSFAULTACT_Pos (1U) ///< Bit 1
/** BusFault active. */
#define SCB_SHCSR_BUSFAULTACT ((uint32_t)1U << SCB_SHCSR_BUSFAULTACT_Pos)
#define SCB_SHCSR_USGFAULTACT_Pos (3U) ///< Bit 3
/** UsageFault active. */
#define SCB_SHCSR_USGFAULTACT ((uint32_t)1U << SCB_SHCSR_USGFAULTACT_Pos)
#define SCB_SHCSR_SVCALLACT_Pos (7U)                                  ///< Bit 7
#define SCB_SHCSR_SVCALLACT ((uint32_t)1U << SCB_SHCSR_SVCALLACT_Pos) ///< SVCall active
#define SCB_SHCSR_PENDSVACT_Pos (10U)                                 ///< Bit 10
#define SCB_SHCSR_PENDSVACT ((uint32_t)1U << SCB_SHCSR_PENDSVACT_Pos) ///< PendSV active
#define SCB_SHCSR_SYSTICKACT_Pos (11U)                                ///< Bit 11
/** SysTick active. */
#define SCB_SHCSR_SYSTICKACT ((uint32_t)1U << SCB_SHCSR_SYSTICKACT_Pos)
#define SCB_SHCSR_USGFAULTPENDED_Pos (12U) ///< Bit 12
/** UsageFault pending. */
#define SCB_SHCSR_USGFAULTPENDED ((uint32_t)1U << SCB_SHCSR_USGFAULTPENDED_Pos)
#define SCB_SHCSR_MEMFAULTPENDED_Pos (13U) ///< Bit 13
/** MemManage pending. */
#define SCB_SHCSR_MEMFAULTPENDED ((uint32_t)1U << SCB_SHCSR_MEMFAULTPENDED_Pos)
#define SCB_SHCSR_BUSFAULTPENDED_Pos (14U) ///< Bit 14
/** BusFault pending. */
#define SCB_SHCSR_BUSFAULTPENDED ((uint32_t)1U << SCB_SHCSR_BUSFAULTPENDED_Pos)
#define SCB_SHCSR_SVCALLPENDED_Pos (15U) ///< Bit 15
/** SVCall pending. */
#define SCB_SHCSR_SVCALLPENDED ((uint32_t)1U << SCB_SHCSR_SVCALLPENDED_Pos)
#define SCB_SHCSR_MEMFAULTENA_Pos (16U) ///< Bit 16
/** Enable MemManage (otherwise it escalates to HardFault). */
#define SCB_SHCSR_MEMFAULTENA ((uint32_t)1U << SCB_SHCSR_MEMFAULTENA_Pos)
#define SCB_SHCSR_BUSFAULTENA_Pos (17U) ///< Bit 17
/** Enable BusFault (otherwise it escalates to HardFault). */
#define SCB_SHCSR_BUSFAULTENA ((uint32_t)1U << SCB_SHCSR_BUSFAULTENA_Pos)
#define SCB_SHCSR_USGFAULTENA_Pos (18U) ///< Bit 18
/** Enable UsageFault (otherwise it escalates to HardFault). */
#define SCB_SHCSR_USGFAULTENA ((uint32_t)1U << SCB_SHCSR_USGFAULTENA_Pos)
/** @} */

/** @name CFSR - configurable fault status (0x28)
 *  Bits 7:0 are MMFSR, 15:8 BFSR, 31:16 UFSR. Every bit is \regaccess{rc_w1}.
 *  @{ */
#define SCB_CFSR_IACCVIOL_Pos (0U) ///< Bit 0
/** MemManage: instruction access violation. */
#define SCB_CFSR_IACCVIOL ((uint32_t)1U << SCB_CFSR_IACCVIOL_Pos)
#define SCB_CFSR_DACCVIOL_Pos (1U) ///< Bit 1
/** MemManage: data access violation; MMFAR holds the address if MMARVALID. */
#define SCB_CFSR_DACCVIOL ((uint32_t)1U << SCB_CFSR_DACCVIOL_Pos)
#define SCB_CFSR_MUNSTKERR_Pos (3U) ///< Bit 3
/** MemManage: fault while unstacking on exception return. */
#define SCB_CFSR_MUNSTKERR ((uint32_t)1U << SCB_CFSR_MUNSTKERR_Pos)
#define SCB_CFSR_MSTKERR_Pos (4U) ///< Bit 4
/** MemManage: fault while stacking on exception entry. */
#define SCB_CFSR_MSTKERR ((uint32_t)1U << SCB_CFSR_MSTKERR_Pos)
#define SCB_CFSR_MLSPERR_Pos (5U) ///< Bit 5
/** MemManage: fault during lazy FP state preservation. */
#define SCB_CFSR_MLSPERR ((uint32_t)1U << SCB_CFSR_MLSPERR_Pos)
#define SCB_CFSR_MMARVALID_Pos (7U) ///< Bit 7
/** MMFAR holds a valid fault address. */
#define SCB_CFSR_MMARVALID ((uint32_t)1U << SCB_CFSR_MMARVALID_Pos)
#define SCB_CFSR_IBUSERR_Pos (8U) ///< Bit 8
/** BusFault: instruction fetch. */
#define SCB_CFSR_IBUSERR ((uint32_t)1U << SCB_CFSR_IBUSERR_Pos)
#define SCB_CFSR_PRECISERR_Pos (9U) ///< Bit 9
/** BusFault: precise data access; BFAR holds the address if BFARVALID. */
#define SCB_CFSR_PRECISERR ((uint32_t)1U << SCB_CFSR_PRECISERR_Pos)
#define SCB_CFSR_IMPRECISERR_Pos (10U) ///< Bit 10
/** BusFault: imprecise data access (no address). */
#define SCB_CFSR_IMPRECISERR ((uint32_t)1U << SCB_CFSR_IMPRECISERR_Pos)
#define SCB_CFSR_UNSTKERR_Pos (11U) ///< Bit 11
/** BusFault: fault while unstacking on exception return. */
#define SCB_CFSR_UNSTKERR ((uint32_t)1U << SCB_CFSR_UNSTKERR_Pos)
#define SCB_CFSR_STKERR_Pos (12U) ///< Bit 12
/** BusFault: fault while stacking on exception entry. */
#define SCB_CFSR_STKERR ((uint32_t)1U << SCB_CFSR_STKERR_Pos)
#define SCB_CFSR_LSPERR_Pos (13U) ///< Bit 13
/** BusFault: fault during lazy FP state preservation. */
#define SCB_CFSR_LSPERR ((uint32_t)1U << SCB_CFSR_LSPERR_Pos)
#define SCB_CFSR_BFARVALID_Pos (15U) ///< Bit 15
/** BFAR holds a valid fault address. */
#define SCB_CFSR_BFARVALID ((uint32_t)1U << SCB_CFSR_BFARVALID_Pos)
#define SCB_CFSR_UNDEFINSTR_Pos (16U) ///< Bit 16
/** UsageFault: undefined instruction. */
#define SCB_CFSR_UNDEFINSTR ((uint32_t)1U << SCB_CFSR_UNDEFINSTR_Pos)
#define SCB_CFSR_INVSTATE_Pos (17U) ///< Bit 17
/** UsageFault: invalid EPSR state (for example a Thumb bit of 0). */
#define SCB_CFSR_INVSTATE ((uint32_t)1U << SCB_CFSR_INVSTATE_Pos)
#define SCB_CFSR_INVPC_Pos (18U) ///< Bit 18
/** UsageFault: invalid PC load or EXC_RETURN. */
#define SCB_CFSR_INVPC ((uint32_t)1U << SCB_CFSR_INVPC_Pos)
#define SCB_CFSR_NOCP_Pos (19U) ///< Bit 19
/** UsageFault: coprocessor (FPU) access while disabled in CPACR. */
#define SCB_CFSR_NOCP ((uint32_t)1U << SCB_CFSR_NOCP_Pos)
#define SCB_CFSR_UNALIGNED_Pos (24U) ///< Bit 24
/** UsageFault: unaligned access with CCR.UNALIGN_TRP set. */
#define SCB_CFSR_UNALIGNED ((uint32_t)1U << SCB_CFSR_UNALIGNED_Pos)
#define SCB_CFSR_DIVBYZERO_Pos (25U) ///< Bit 25
/** UsageFault: divide by zero with CCR.DIV_0_TRP set. */
#define SCB_CFSR_DIVBYZERO ((uint32_t)1U << SCB_CFSR_DIVBYZERO_Pos)
/** @} */

/** @name HFSR - HardFault status (0x2C)
 *  Every bit is \regaccess{rc_w1}.
 *  @{ */
#define SCB_HFSR_VECTTBL_Pos (1U) ///< Bit 1
/** Bus fault while reading the vector table. */
#define SCB_HFSR_VECTTBL ((uint32_t)1U << SCB_HFSR_VECTTBL_Pos)
#define SCB_HFSR_FORCED_Pos (30U) ///< Bit 30
/** A configurable fault escalated to HardFault; see CFSR. */
#define SCB_HFSR_FORCED ((uint32_t)1U << SCB_HFSR_FORCED_Pos)
#define SCB_HFSR_DEBUGEVT_Pos (31U)                               ///< Bit 31
#define SCB_HFSR_DEBUGEVT ((uint32_t)1U << SCB_HFSR_DEBUGEVT_Pos) ///< Debug event
/** @} */

/** @name SHPR1-3 - system handler priorities (0x18-0x20)
 *  One \regaccess{rw} byte per handler; only the top 4 bits are implemented.
 *  @{ */
#define SCB_SHPR1_PRI_4_Pos (0U)                                       ///< Bits 7:0
#define SCB_SHPR1_PRI_4_Msk ((uint32_t)0xFFU << SCB_SHPR1_PRI_4_Pos)   ///< MemManage priority
#define SCB_SHPR1_PRI_5_Pos (8U)                                       ///< Bits 15:8
#define SCB_SHPR1_PRI_5_Msk ((uint32_t)0xFFU << SCB_SHPR1_PRI_5_Pos)   ///< BusFault priority
#define SCB_SHPR1_PRI_6_Pos (16U)                                      ///< Bits 23:16
#define SCB_SHPR1_PRI_6_Msk ((uint32_t)0xFFU << SCB_SHPR1_PRI_6_Pos)   ///< UsageFault priority
#define SCB_SHPR2_PRI_11_Pos (24U)                                     ///< Bits 31:24
#define SCB_SHPR2_PRI_11_Msk ((uint32_t)0xFFU << SCB_SHPR2_PRI_11_Pos) ///< SVCall priority
#define SCB_SHPR3_PRI_14_Pos (16U)                                     ///< Bits 23:16
#define SCB_SHPR3_PRI_14_Msk ((uint32_t)0xFFU << SCB_SHPR3_PRI_14_Pos) ///< PendSV priority
#define SCB_SHPR3_PRI_15_Pos (24U)                                     ///< Bits 31:24
#define SCB_SHPR3_PRI_15_Msk ((uint32_t)0xFFU << SCB_SHPR3_PRI_15_Pos) ///< SysTick priority
/** @} */

/** @name CPACR - coprocessor access control (0x88)
 *  @{ */
#define SCB_CPACR_CP10_Pos (20U) ///< Bits 21:20
/** \regaccess{rw} FPU access (CP10); 0b11 = full. Reset_Handler enables it. */
#define SCB_CPACR_CP10_Msk ((uint32_t)0x3U << SCB_CPACR_CP10_Pos)
#define SCB_CPACR_CP11_Pos (22U) ///< Bits 23:22
/** \regaccess{rw} FPU access (CP11); must match CP10. */
#define SCB_CPACR_CP11_Msk ((uint32_t)0x3U << SCB_CPACR_CP11_Pos)
/** @} */

/** @name EXC_RETURN values
 *  The LR value on exception entry; branching to it returns (PM0214 Table 2-17).
 *  The `_FPU` variants pop an extended frame with FPU state.
 *  @{ */
#define EXC_RETURN_HANDLER (0xFFFFFFF1UL)        ///< Handler mode, MSP, basic frame
#define EXC_RETURN_THREAD_MSP (0xFFFFFFF9UL)     ///< Thread mode, MSP, basic frame
#define EXC_RETURN_THREAD_PSP (0xFFFFFFFDUL)     ///< Thread mode, PSP, basic frame
#define EXC_RETURN_HANDLER_FPU (0xFFFFFFE1UL)    ///< Handler mode, MSP, extended frame
#define EXC_RETURN_THREAD_MSP_FPU (0xFFFFFFE9UL) ///< Thread mode, MSP, extended frame
#define EXC_RETURN_THREAD_PSP_FPU (0xFFFFFFEDUL) ///< Thread mode, PSP, extended frame
/** @} */

/** @} */

/**
 * @addtogroup fpu_registers
 * @{
 */

/** @name FPCCR - FP context control (0xE000EF34)
 *  @{ */
#define FPU_FPCCR_LSPACT_Pos (0U) ///< Bit 0
/** \regaccess{rw} Lazy state preservation is active: space is reserved but FP state not saved. */
#define FPU_FPCCR_LSPACT ((uint32_t)1U << FPU_FPCCR_LSPACT_Pos)
#define FPU_FPCCR_LSPEN_Pos (30U) ///< Bit 30
/** \regaccess{rw} Enable lazy FP context stacking (reset value 1). */
#define FPU_FPCCR_LSPEN ((uint32_t)1U << FPU_FPCCR_LSPEN_Pos)
#define FPU_FPCCR_ASPEN_Pos (31U) ///< Bit 31
/** \regaccess{rw} Save FP state automatically on exception entry (reset value 1). */
#define FPU_FPCCR_ASPEN ((uint32_t)1U << FPU_FPCCR_ASPEN_Pos)
/** @} */

/** @} */

#endif /* SCB_REG_H */
