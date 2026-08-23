/**
 * @file scb_reg.h
 * @brief SCB (System Control Block) register definitions for the ARM
 *        Cortex-M4 (Armv7-M architecture), including the FPU context
 *        control registers.
 *
 * Register layout derived from PM0214 (Cortex-M4 programming manual).
 * Architectural, not vendor-specific - fixed by Armv7-M's System
 * Control Space memory map. Covers exception/fault configuration and
 * status (AIRCR, SHPR1-3, SHCSR, CFSR, HFSR, MMFAR, BFAR) and FPU
 * enable/lazy-stacking control (CPACR, FPCCR) - the two things a
 * HardFault handler and a PendSV context switch respectively need.
 *
 * The CPU feature/ID registers (PFR0/1, DFR0, AFR0, MMFR0-3, ISAR0-4 at
 * 0xE000ED40-0xE000ED84) are omitted - RESERVED0 covers that range -
 * since nothing in this RTOS does runtime CPU-feature detection.
 *
 * FPCCR/FPCAR/FPDSCR sit at 0xE000EF34, not contiguous with the main
 * SCB block at 0xE000ED00 (the architecture places the MPU's register
 * bank, see mpu_reg.h, in between) - hence the separate FpuRegisters_t
 * struct and base address below.
 */
#ifndef SCB_REG_H
#define SCB_REG_H

#include <stdint.h>

#define SCB_BASE (0xE000ED00UL) ///< SCB peripheral base address (Armv7-M SCS)
#define FPU_BASE (0xE000EF34UL) ///< FPU context control register base address (Armv7-M SCS)

/**
 * @brief SCB register map (PM0214, Armv7-M SCS SCB region).
 *
 * RESERVED0 preserves the fixed architectural offset to CPACR (see the
 * top-of-file note above) - it's a real gap in the memory map, not a
 * struct-packing artifact.
 */
typedef struct ScbRegisters_t
{
  volatile uint32_t CPUID; ///< 0x00: CPUID base register (READ only)
  volatile uint32_t ICSR;  ///< 0x04: interrupt control and state register
  volatile uint32_t VTOR;  ///< 0x08: vector table offset register
  volatile uint32_t AIRCR; ///< 0x0C: application interrupt and reset control register
  volatile uint32_t SCR;   ///< 0x10: system control register
  volatile uint32_t CCR;   ///< 0x14: configuration and control register
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

/* --- SCB_AIRCR bit definitions --- */
#define SCB_AIRCR_VECTRESET_Pos (0U)                            ///< Bit position within SCB_AIRCR
#define SCB_AIRCR_VECTRESET (1U << SCB_AIRCR_VECTRESET_Pos)     ///< WRITE - local reset, debug only
#define SCB_AIRCR_SYSRESETREQ_Pos (2U)                          ///< Bit position within SCB_AIRCR
#define SCB_AIRCR_SYSRESETREQ (1U << SCB_AIRCR_SYSRESETREQ_Pos) ///< WRITE - request system reset
#define SCB_AIRCR_PRIGROUP_Pos (8U)                             ///< Bit position within SCB_AIRCR
#define SCB_AIRCR_PRIGROUP_Msk (0x7U << SCB_AIRCR_PRIGROUP_Pos) ///< bits 10:8, priority grouping
#define SCB_AIRCR_VECTKEY_Pos (16U)                             ///< Bit position within SCB_AIRCR
#define SCB_AIRCR_VECTKEY_Msk (0xFFFFU << SCB_AIRCR_VECTKEY_Pos)  ///< bits 31:16
#define SCB_AIRCR_VECTKEY_WRITE (0x5FAU << SCB_AIRCR_VECTKEY_Pos) ///< required key to permit write

/* --- SCB_CCR bit definitions --- */
#define SCB_CCR_NONBASETHRDENA_Pos (0U)                           ///< Bit position within SCB_CCR
#define SCB_CCR_NONBASETHRDENA (1U << SCB_CCR_NONBASETHRDENA_Pos) ///< thread mode entry from ISR
#define SCB_CCR_UNALIGN_TRP_Pos (3U)                              ///< Bit position within SCB_CCR
#define SCB_CCR_UNALIGN_TRP (1U << SCB_CCR_UNALIGN_TRP_Pos)       ///< WRITE - trap unaligned access
#define SCB_CCR_DIV_0_TRP_Pos (4U)                                ///< Bit position within SCB_CCR
#define SCB_CCR_DIV_0_TRP (1U << SCB_CCR_DIV_0_TRP_Pos)           ///< WRITE - trap divide-by-zero
#define SCB_CCR_BFHFNMIGN_Pos (8U)                                ///< Bit position within SCB_CCR
#define SCB_CCR_BFHFNMIGN (1U << SCB_CCR_BFHFNMIGN_Pos) ///< ignore faults in fault/NMI handlers
#define SCB_CCR_STKALIGN_Pos (9U)                       ///< Bit position within SCB_CCR
#define SCB_CCR_STKALIGN (1U << SCB_CCR_STKALIGN_Pos)   ///< force 8-byte stack align on exception

/* --- SCB_SHCSR bit definitions --- */
#define SCB_SHCSR_MEMFAULTENA_Pos (16U)                         ///< Bit position within SCB_SHCSR
#define SCB_SHCSR_MEMFAULTENA (1U << SCB_SHCSR_MEMFAULTENA_Pos) ///< WRITE - enable MemManage
#define SCB_SHCSR_BUSFAULTENA_Pos (17U)                         ///< Bit position within SCB_SHCSR
#define SCB_SHCSR_BUSFAULTENA (1U << SCB_SHCSR_BUSFAULTENA_Pos) ///< WRITE - enable BusFault
#define SCB_SHCSR_USGFAULTENA_Pos (18U)                         ///< Bit position within SCB_SHCSR
#define SCB_SHCSR_USGFAULTENA (1U << SCB_SHCSR_USGFAULTENA_Pos) ///< WRITE - enable UsageFault

/* --- SCB_CFSR bit definitions (MMFSR: bits 7:0, BFSR: bits 15:8, UFSR: bits 31:16) --- */
#define SCB_CFSR_IACCVIOL_Pos (0U)                        ///< Bit position within SCB_CFSR (MMFSR)
#define SCB_CFSR_IACCVIOL (1U << SCB_CFSR_IACCVIOL_Pos)   ///< READ only - instr access violation
#define SCB_CFSR_DACCVIOL_Pos (1U)                        ///< Bit position within SCB_CFSR (MMFSR)
#define SCB_CFSR_DACCVIOL (1U << SCB_CFSR_DACCVIOL_Pos)   ///< READ only - data access violation
#define SCB_CFSR_MUNSTKERR_Pos (3U)                       ///< Bit position within SCB_CFSR (MMFSR)
#define SCB_CFSR_MUNSTKERR (1U << SCB_CFSR_MUNSTKERR_Pos) ///< READ only - fault on exc. return
#define SCB_CFSR_MSTKERR_Pos (4U)                         ///< Bit position within SCB_CFSR (MMFSR)
#define SCB_CFSR_MSTKERR (1U << SCB_CFSR_MSTKERR_Pos)     ///< READ only - fault on exception entry
#define SCB_CFSR_MMARVALID_Pos (7U)                       ///< Bit position within SCB_CFSR (MMFSR)
#define SCB_CFSR_MMARVALID (1U << SCB_CFSR_MMARVALID_Pos) ///< READ only - MMFAR is valid
#define SCB_CFSR_IBUSERR_Pos (8U)                         ///< Bit position within SCB_CFSR (BFSR)
#define SCB_CFSR_IBUSERR (1U << SCB_CFSR_IBUSERR_Pos)     ///< READ only - fault on instr fetch
#define SCB_CFSR_PRECISERR_Pos (9U)                       ///< Bit position within SCB_CFSR (BFSR)
#define SCB_CFSR_PRECISERR (1U << SCB_CFSR_PRECISERR_Pos) ///< READ only - precise err, BFAR valid
#define SCB_CFSR_IMPRECISERR_Pos (10U)                    ///< Bit position within SCB_CFSR (BFSR)
#define SCB_CFSR_IMPRECISERR (1U << SCB_CFSR_IMPRECISERR_Pos) ///< READ only - imprecise bus error
#define SCB_CFSR_UNSTKERR_Pos (11U)                       ///< Bit position within SCB_CFSR (BFSR)
#define SCB_CFSR_UNSTKERR (1U << SCB_CFSR_UNSTKERR_Pos)   ///< READ only - fault on exception return
#define SCB_CFSR_STKERR_Pos (12U)                         ///< Bit position within SCB_CFSR (BFSR)
#define SCB_CFSR_STKERR (1U << SCB_CFSR_STKERR_Pos)       ///< READ only - fault on exception entry
#define SCB_CFSR_BFARVALID_Pos (15U)                      ///< Bit position within SCB_CFSR (BFSR)
#define SCB_CFSR_BFARVALID (1U << SCB_CFSR_BFARVALID_Pos) ///< READ only - BFAR is valid
#define SCB_CFSR_UNDEFINSTR_Pos (16U)                     ///< Bit position within SCB_CFSR (UFSR)
#define SCB_CFSR_UNDEFINSTR (1U << SCB_CFSR_UNDEFINSTR_Pos) ///< READ only - undefined instruction
#define SCB_CFSR_INVSTATE_Pos (17U)                         ///< Bit position within SCB_CFSR (UFSR)
#define SCB_CFSR_INVSTATE (1U << SCB_CFSR_INVSTATE_Pos)     ///< READ only - invalid EPSR state
#define SCB_CFSR_INVPC_Pos (18U)                            ///< Bit position within SCB_CFSR (UFSR)
#define SCB_CFSR_INVPC (1U << SCB_CFSR_INVPC_Pos) ///< READ only - invalid PC load/exc return
#define SCB_CFSR_NOCP_Pos (19U)                   ///< Bit position within SCB_CFSR (UFSR)
#define SCB_CFSR_NOCP (1U << SCB_CFSR_NOCP_Pos)   ///< READ only - coprocessor access denied
#define SCB_CFSR_UNALIGNED_Pos (24U)              ///< Bit position within SCB_CFSR (UFSR)
#define SCB_CFSR_UNALIGNED (1U << SCB_CFSR_UNALIGNED_Pos) ///< READ only - unaligned access trap
#define SCB_CFSR_DIVBYZERO_Pos (25U)                      ///< Bit position within SCB_CFSR (UFSR)
#define SCB_CFSR_DIVBYZERO (1U << SCB_CFSR_DIVBYZERO_Pos) ///< READ only - divide-by-zero trap

/* --- SCB_HFSR bit definitions --- */
#define SCB_HFSR_VECTTBL_Pos (1U)                     ///< Bit position within SCB_HFSR
#define SCB_HFSR_VECTTBL (1U << SCB_HFSR_VECTTBL_Pos) ///< READ only - fault reading vector table
#define SCB_HFSR_FORCED_Pos (30U)                     ///< Bit position within SCB_HFSR
#define SCB_HFSR_FORCED (1U << SCB_HFSR_FORCED_Pos)   ///< READ only - fault escalated to HardFault
#define SCB_HFSR_DEBUGEVT_Pos (31U)                   ///< Bit position within SCB_HFSR
#define SCB_HFSR_DEBUGEVT (1U << SCB_HFSR_DEBUGEVT_Pos) ///< READ only - debug event

/* --- SCB_SHPR2/SHPR3 bit definitions (system handler priority fields) --- */
#define SCB_SHPR2_PRI_11_Pos (24U)                           ///< Bit position within SCB_SHPR2
#define SCB_SHPR2_PRI_11_Msk (0xFFU << SCB_SHPR2_PRI_11_Pos) ///< bits 31:24, SVCall priority
#define SCB_SHPR3_PRI_14_Pos (16U)                           ///< Bit position within SCB_SHPR3
#define SCB_SHPR3_PRI_14_Msk (0xFFU << SCB_SHPR3_PRI_14_Pos) ///< bits 23:16, PendSV priority
#define SCB_SHPR3_PRI_15_Pos (24U)                           ///< Bit position within SCB_SHPR3
#define SCB_SHPR3_PRI_15_Msk (0xFFU << SCB_SHPR3_PRI_15_Pos) ///< bits 31:24, SysTick priority

/* --- SCB_CPACR bit definitions --- */
#define SCB_CPACR_CP10_Pos (20U)                        ///< Bit position within SCB_CPACR
#define SCB_CPACR_CP10_Msk (0x3U << SCB_CPACR_CP10_Pos) ///< bits 21:20, FPU access, 0b11=full
#define SCB_CPACR_CP11_Pos (22U)                        ///< Bit position within SCB_CPACR
#define SCB_CPACR_CP11_Msk (0x3U << SCB_CPACR_CP11_Pos) ///< bits 23:22, FPU access, 0b11=full

/* --- FPU_FPCCR bit definitions --- */
#define FPU_FPCCR_LSPACT_Pos (0U)                     ///< Bit position within FPU_FPCCR
#define FPU_FPCCR_LSPACT (1U << FPU_FPCCR_LSPACT_Pos) ///< READ only - lazy save pending
#define FPU_FPCCR_ASPEN_Pos (31U)                     ///< Bit position within FPU_FPCCR
#define FPU_FPCCR_ASPEN (1U << FPU_FPCCR_ASPEN_Pos)   ///< WRITE - auto FP state save on exception
#define FPU_FPCCR_LSPEN_Pos (30U)                     ///< Bit position within FPU_FPCCR
#define FPU_FPCCR_LSPEN (1U << FPU_FPCCR_LSPEN_Pos)   ///< WRITE - enable lazy FP context stacking

#endif /* SCB_REG_H */
