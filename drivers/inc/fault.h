/**
 * @file fault.h
 * @brief Fault driver: enable the configurable faults and record a fault for
 *        the debugger.
 *
 * HardFault, MemManage, BusFault and UsageFault all enter `Fault_Handler` in
 * the startup file. It passes the stacked frame to faultCapture(), which
 * fills ::faultRecord, then spins. Nothing is returned to: a fault is fatal
 * until the kernel has a policy for it. Halt the core and read the record:
 * @code
 * faultEnable(SCB); // once, early in main()
 * // after a fault, in GDB:
 * // (gdb) p/x faultRecord
 * @endcode
 *
 * Without faultEnable(), MemManage, BusFault and UsageFault escalate to
 * HardFault (HFSR.FORCED). CFSR still names the cause, so the record is just
 * as useful; only the exception number differs.
 */
#ifndef FAULT_H
#define FAULT_H

#include "scb_reg.h"

#include <stdbool.h>
#include <stdint.h>

/**
 * @addtogroup fault_driver
 * @{
 */

/** @brief What `Fault_Handler` saw. All zero until the first fault. */
typedef struct FaultRecord_t
{
    uint32_t exception;  ///< ICSR.VECTACTIVE: 3 HardFault, 4 MemManage, 5 BusFault, 6 UsageFault
    uint32_t exc_return; ///< LR on entry; bit 2 set means the frame is on PSP
    uint32_t cfsr;       ///< CFSR: MMFSR (7:0), BFSR (15:8), UFSR (31:16)
    uint32_t hfsr;       ///< HFSR: FORCED means a configurable fault escalated
    uint32_t mmfar;      ///< MMFAR; the fault address only if CFSR.MMARVALID
    uint32_t bfar;       ///< BFAR; the fault address only if CFSR.BFARVALID
    /** The registers below were read from the stacked frame. False when
     *  CFSR.MSTKERR or STKERR says stacking failed, so the frame is not there;
     *  they are then 0. */
    bool frame_valid;
    uint32_t r0;   ///< Stacked R0
    uint32_t r1;   ///< Stacked R1
    uint32_t r2;   ///< Stacked R2
    uint32_t r3;   ///< Stacked R3
    uint32_t r12;  ///< Stacked R12
    uint32_t lr;   ///< Stacked LR: the faulting function's return address
    uint32_t pc;   ///< Stacked PC: the faulting instruction, or the next one for an imprecise fault
    uint32_t xpsr; ///< Stacked xPSR
} FaultRecord_t;

/** Filled by `Fault_Handler`; read it from the debugger. */
extern FaultRecord_t faultRecord;

/**
 * @brief Give MemManage, BusFault and UsageFault their own handlers, and trap
 *        integer division by zero.
 *
 * Sets SHCSR.MEMFAULTENA, BUSFAULTENA and USGFAULTENA, and CCR.DIV_0_TRP.
 * Without the trap, `SDIV`/`UDIV` by zero quietly return 0. Other bits are
 * kept.
 *
 * @param scb SCB block (e.g. SCB).
 */
void faultEnable(ScbRegisters_t *scb);

/**
 * @brief Fill @p record from the fault status registers and the stacked frame.
 *
 * Called by `Fault_Handler` with the frame from MSP or PSP, as EXC_RETURN
 * bit 2 selects. The frame is read only if CFSR.MSTKERR and STKERR are clear:
 * when stacking failed, the frame is missing and reading it could fault again
 * inside HardFault, which locks up the core.
 *
 * @param record     Receives the record; every field is written.
 * @param scb        SCB block (e.g. SCB); read only, so the status bits stay set for the debugger.
 * @param frame      The stacked R0, R1, R2, R3, R12, LR, PC, xPSR, in that order.
 * @param exc_return LR on handler entry.
 */
void faultCapture(FaultRecord_t *record, const ScbRegisters_t *scb, const uint32_t *frame,
                  uint32_t exc_return);

/** @} */

#endif /* FAULT_H */
