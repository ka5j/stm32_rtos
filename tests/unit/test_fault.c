/**
 * @file test_fault.c
 * @brief Host-side tests for drivers/src/fault.c, on an in-memory
 *        ScbRegisters_t and a stacked frame built as an array. They check
 *        what faultEnable() writes and what faultCapture() records; whether
 *        Fault_Handler is entered with the right frame is a target check.
 */
#include "fault.h"
#include "unity.h"

#include <stddef.h>
#include <string.h>

#define FAULT_ENABLES (SCB_SHCSR_MEMFAULTENA | SCB_SHCSR_BUSFAULTENA | SCB_SHCSR_USGFAULTENA)

/** A frame whose words are all distinct, so a swapped field shows. */
static const uint32_t FRAME[8] = {0x10U, 0x11U,       0x12U,       0x13U,
                                  0x1CU, 0x0800012BU, 0x08000150U, 0x01000000U};

/** Sets the three fault enables and the divide-by-zero trap. */
void test_fault_driver_enable_sets_enables_and_div0_trap(void)
{
    ScbRegisters_t scb = {0};

    faultEnable(&scb);

    TEST_ASSERT_EQUAL_HEX32(FAULT_ENABLES, scb.SHCSR);
    TEST_ASSERT_EQUAL_HEX32(SCB_CCR_DIV_0_TRP, scb.CCR);
}

/** Keeps the other SHCSR and CCR bits, such as STKALIGN (set at reset). */
void test_fault_driver_enable_preserves_other_bits(void)
{
    ScbRegisters_t scb = {.SHCSR = SCB_SHCSR_SVCALLACT, .CCR = SCB_CCR_STKALIGN};

    faultEnable(&scb);

    TEST_ASSERT_EQUAL_HEX32(SCB_SHCSR_SVCALLACT | FAULT_ENABLES, scb.SHCSR);
    TEST_ASSERT_EQUAL_HEX32(SCB_CCR_STKALIGN | SCB_CCR_DIV_0_TRP, scb.CCR);
}

/** Records every status register and every frame word, each in its own field. */
void test_fault_driver_capture_records_status_and_frame(void)
{
    ScbRegisters_t scb = {
        .ICSR = 6U, /* VECTACTIVE: UsageFault */
        .CFSR = SCB_CFSR_DIVBYZERO,
        .HFSR = SCB_HFSR_FORCED,
        .MMFAR = 0xAAAA0000U,
        .BFAR = 0xBBBB0000U,
    };
    FaultRecord_t record = {0};

    faultCapture(&record, &scb, FRAME, EXC_RETURN_THREAD_PSP);

    TEST_ASSERT_EQUAL_UINT32(6U, record.exception);
    TEST_ASSERT_EQUAL_HEX32(EXC_RETURN_THREAD_PSP, record.exc_return);
    TEST_ASSERT_EQUAL_HEX32(SCB_CFSR_DIVBYZERO, record.cfsr);
    TEST_ASSERT_EQUAL_HEX32(SCB_HFSR_FORCED, record.hfsr);
    TEST_ASSERT_EQUAL_HEX32(0xAAAA0000U, record.mmfar);
    TEST_ASSERT_EQUAL_HEX32(0xBBBB0000U, record.bfar);
    TEST_ASSERT_TRUE(record.frame_valid);
    TEST_ASSERT_EQUAL_HEX32(FRAME[0], record.r0);
    TEST_ASSERT_EQUAL_HEX32(FRAME[1], record.r1);
    TEST_ASSERT_EQUAL_HEX32(FRAME[2], record.r2);
    TEST_ASSERT_EQUAL_HEX32(FRAME[3], record.r3);
    TEST_ASSERT_EQUAL_HEX32(FRAME[4], record.r12);
    TEST_ASSERT_EQUAL_HEX32(FRAME[5], record.lr);
    TEST_ASSERT_EQUAL_HEX32(FRAME[6], record.pc);
    TEST_ASSERT_EQUAL_HEX32(FRAME[7], record.xpsr);
}

/** VECTACTIVE is the low 9 bits of ICSR; the pend bits above it are not part of it. */
void test_fault_driver_capture_masks_vectactive(void)
{
    ScbRegisters_t scb = {.ICSR = SCB_ICSR_PENDSVSET | 3U};
    FaultRecord_t record = {0};

    faultCapture(&record, &scb, FRAME, EXC_RETURN_HANDLER);

    TEST_ASSERT_EQUAL_UINT32(3U, record.exception);
}

/** A failed MemManage stacking leaves no frame: it is not read, and the frame fields are 0. */
void test_fault_driver_capture_skips_frame_after_mstkerr(void)
{
    ScbRegisters_t scb = {.ICSR = 4U, .CFSR = SCB_CFSR_MSTKERR};
    FaultRecord_t record;

    (void)memset(&record, 0xA5, sizeof record);

    /* NULL: any read of the frame would crash, under ASan or not. */
    faultCapture(&record, &scb, NULL, EXC_RETURN_THREAD_PSP);

    TEST_ASSERT_FALSE(record.frame_valid);
    TEST_ASSERT_EQUAL_HEX32(SCB_CFSR_MSTKERR, record.cfsr);
    TEST_ASSERT_EQUAL_UINT32(4U, record.exception);
    TEST_ASSERT_EQUAL_HEX32(0U, record.r0);
    TEST_ASSERT_EQUAL_HEX32(0U, record.r1);
    TEST_ASSERT_EQUAL_HEX32(0U, record.r2);
    TEST_ASSERT_EQUAL_HEX32(0U, record.r3);
    TEST_ASSERT_EQUAL_HEX32(0U, record.r12);
    TEST_ASSERT_EQUAL_HEX32(0U, record.lr);
    TEST_ASSERT_EQUAL_HEX32(0U, record.pc);
    TEST_ASSERT_EQUAL_HEX32(0U, record.xpsr);
}

/** A failed BusFault stacking leaves no frame either. */
void test_fault_driver_capture_skips_frame_after_stkerr(void)
{
    ScbRegisters_t scb = {.CFSR = SCB_CFSR_STKERR};
    FaultRecord_t record = {0};

    faultCapture(&record, &scb, NULL, EXC_RETURN_THREAD_MSP);

    TEST_ASSERT_FALSE(record.frame_valid);
}

/** An unstacking error (on return) does not invalidate the entry frame. */
void test_fault_driver_capture_reads_frame_after_unstacking_error(void)
{
    ScbRegisters_t scb = {.CFSR = SCB_CFSR_MUNSTKERR | SCB_CFSR_UNSTKERR};
    FaultRecord_t record = {0};

    faultCapture(&record, &scb, FRAME, EXC_RETURN_THREAD_PSP);

    TEST_ASSERT_TRUE(record.frame_valid);
    TEST_ASSERT_EQUAL_HEX32(FRAME[6], record.pc);
}

/** Capturing only reads the SCB: CFSR and HFSR are rc_w1, so the bits stay for the debugger. */
void test_fault_driver_capture_leaves_scb_unchanged(void)
{
    ScbRegisters_t scb = {.ICSR = 5U,
                          .CFSR = SCB_CFSR_PRECISERR | SCB_CFSR_BFARVALID,
                          .HFSR = SCB_HFSR_FORCED,
                          .BFAR = 0x40000000U};
    ScbRegisters_t before = scb;
    FaultRecord_t record = {0};

    faultCapture(&record, &scb, FRAME, EXC_RETURN_HANDLER);

    TEST_ASSERT_EQUAL_MEMORY(&before, &scb, sizeof scb);
}
