/**
 * @file fault.c
 * @brief Fault driver implementation (API in fault.h).
 */
#include "fault.h"

/**
 * @addtogroup fault_driver
 * @{
 */

FaultRecord_t faultRecord;

void faultEnable(ScbRegisters_t *scb)
{
    scb->SHCSR |= SCB_SHCSR_MEMFAULTENA | SCB_SHCSR_BUSFAULTENA | SCB_SHCSR_USGFAULTENA;
    scb->CCR |= SCB_CCR_DIV_0_TRP;
}

void faultCapture(FaultRecord_t *record, const ScbRegisters_t *scb, const uint32_t *frame,
                  uint32_t exc_return)
{
    uint32_t cfsr = scb->CFSR;

    record->exception = (scb->ICSR & SCB_ICSR_VECTACTIVE_Msk) >> SCB_ICSR_VECTACTIVE_Pos;
    record->exc_return = exc_return;
    record->cfsr = cfsr;
    record->hfsr = scb->HFSR;
    record->mmfar = scb->MMFAR;
    record->bfar = scb->BFAR;
    record->frame_valid = ((cfsr & (SCB_CFSR_MSTKERR | SCB_CFSR_STKERR)) == 0U);

    if (record->frame_valid)
    {
        record->r0 = frame[0];
        record->r1 = frame[1];
        record->r2 = frame[2];
        record->r3 = frame[3];
        record->r12 = frame[4];
        record->lr = frame[5];
        record->pc = frame[6];
        record->xpsr = frame[7];
    }
    else
    {
        record->r0 = 0U;
        record->r1 = 0U;
        record->r2 = 0U;
        record->r3 = 0U;
        record->r12 = 0U;
        record->lr = 0U;
        record->pc = 0U;
        record->xpsr = 0U;
    }
}

/** @} */
