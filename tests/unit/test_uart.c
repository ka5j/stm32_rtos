/**
 * @file test_uart.c
 * @brief Host-side tests for drivers/src/uart.c. Each test operates on a
 *        plain in-memory UartRegisters_t standing in for a real
 *        peripheral - uart.c's functions take the register block as a
 *        parameter rather than reaching for a hardware USARTx macro,
 *        which is what makes this testable off-target, including the
 *        SR.TXE/RXNE polls: SR (what a test pre-sets to simulate hardware
 *        state) and what the driver itself writes are separate fields,
 *        so a test can hold TXE/RXNE clear indefinitely to exercise the
 *        timeout branches with no real hardware or timer involved.
 *        Compiled into tests/unit/test_runner.c's run_tests binary.
 *
 *        BRR test vectors below were computed independently (Python,
 *        not this driver's own arithmetic) and are asserted as exact
 *        expected values, not just "not zero" - see each test's comment
 *        for the pclk/baud pair and the expected BRR value.
 */
#include "uart.h"
#include "unity.h"

#include <stddef.h>

/* --- uartInit: BRR computation --- */

/** 45 MHz APB1, 115200 baud (common USART2 config): pclk/baud =
 *  390.625, rounds to 391. */
void test_uart_driver_init_computes_brr_for_45mhz_115200(void)
{
    UartRegisters_t uart = {0};

    DriverStatus_e status =
        uartInit(&uart, 45000000U, 115200U, USART_CR2_STOP_1, 0U, 0U, USART_CR1_TE);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(391U, uart.BRR);
}

/** 16 MHz HSI default, 9600 baud: pclk/baud = 1666.666..., rounds to
 *  1667. */
void test_uart_driver_init_computes_brr_for_16mhz_9600(void)
{
    UartRegisters_t uart = {0};

    DriverStatus_e status =
        uartInit(&uart, 16000000U, 9600U, USART_CR2_STOP_1, 0U, 0U, USART_CR1_TE);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(1667U, uart.BRR);
}

/** 1 MHz / 1025 baud: pclk/baud = 975.6097..., rounds up to 976 since
 *  the fractional part (0.6097) is past the halfway point. Kept as a
 *  regression vector from when this function computed a separate
 *  mantissa and fraction and had to carry a rounded-up fraction into the
 *  mantissa - this formula has no such carry step, but the expected
 *  result is unchanged. */
void test_uart_driver_init_computes_brr_rounds_up_to_976(void)
{
    UartRegisters_t uart = {0};

    DriverStatus_e status =
        uartInit(&uart, 1000000U, 1025U, USART_CR2_STOP_1, 0U, 0U, USART_CR1_TE);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(976U, uart.BRR);
}

/** 42 MHz APB2, 115200 baud: pclk/baud = 364.5833..., which rounds to
 *  365. Regression test for a real accuracy bug in an earlier version of
 *  uartComputeBrr(): that version computed USARTDIV*100 via integer
 *  division (truncating to 2 decimal digits of the true ratio) before
 *  rounding to the nearest 16th, so the truncation already discarded the
 *  0.5833's higher-precision digits by the time rounding happened,
 *  producing BRR 364 instead of the correctly-rounded 365 - one off, in
 *  the wrong direction. This function's direct round(pclk_hz / baud)
 *  has no intermediate truncation step to lose precision in. */
void test_uart_driver_init_computes_brr_rounds_correctly_past_old_truncation_bug(void)
{
    UartRegisters_t uart = {0};

    DriverStatus_e status =
        uartInit(&uart, 42000000U, 115200U, USART_CR2_STOP_1, 0U, 0U, USART_CR1_TE);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(365U, uart.BRR);
}

/** pclk_hz == 0 must be rejected before any write. */
void test_uart_driver_init_rejects_zero_pclk(void)
{
    UartRegisters_t uart = {0};

    DriverStatus_e status = uartInit(&uart, 0U, 115200U, USART_CR2_STOP_1, 0U, 0U, USART_CR1_TE);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, uart.CR1);
}

/** baud == 0 must be rejected before any write. */
void test_uart_driver_init_rejects_zero_baud(void)
{
    UartRegisters_t uart = {0};

    DriverStatus_e status = uartInit(&uart, 16000000U, 0U, USART_CR2_STOP_1, 0U, 0U, USART_CR1_TE);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, uart.CR1);
}

/** baud too low for pclk_hz (90 MHz / 300 baud -> pclk/baud = 300000,
 *  past BRR's 16-bit/65535 field) must be rejected. */
void test_uart_driver_init_rejects_baud_too_low_for_pclk(void)
{
    UartRegisters_t uart = {0};

    DriverStatus_e status =
        uartInit(&uart, 90000000U, 300U, USART_CR2_STOP_1, 0U, 0U, USART_CR1_TE);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, uart.BRR);
}

/** baud too high for pclk_hz (1 MHz / 10 Mbaud -> pclk/baud rounds to
 *  0, a BRR of 0 being invalid per RM0390) must be rejected. */
void test_uart_driver_init_rejects_baud_too_high_for_pclk(void)
{
    UartRegisters_t uart = {0};

    DriverStatus_e status =
        uartInit(&uart, 1000000U, 10000000U, USART_CR2_STOP_1, 0U, 0U, USART_CR1_TE);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, uart.BRR);
}

/** 1 MHz / 1,000,000 baud: pclk/baud rounds to 1, the smallest BRR
 *  value this driver accepts - distinct from the pclk/baud-rounds-to-0
 *  case above, which is the one value RM0390 forbids. */
void test_uart_driver_init_accepts_smallest_nonzero_brr(void)
{
    UartRegisters_t uart = {0};

    DriverStatus_e status =
        uartInit(&uart, 1000000U, 1000000U, USART_CR2_STOP_1, 0U, 0U, USART_CR1_TE);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(1U, uart.BRR);
}

/* --- uartInit: parameter validation --- */

/** stop_bits outside the field's whole range must be rejected. */
void test_uart_driver_init_rejects_invalid_stop_bits(void)
{
    UartRegisters_t uart = {0};

    DriverStatus_e status = uartInit(&uart, 16000000U, 9600U, 0xFFU, 0U, 0U, USART_CR1_TE);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, uart.CR2);
}

/** USART_CR2_STOP_0_5 and _1_5 are within CR2.STOP's 2-bit field width
 *  and are real RM0390 encodings, but this driver never enables the
 *  smartcard mode they belong to (see uart.h's file-level comment) and
 *  rejects both - distinct from test_uart_driver_init_rejects_invalid_
 *  stop_bits above, which only proves a value outside the field's whole
 *  range is rejected, not that these two in-range-but-unsupported values
 *  are. Regression test: an earlier revision of this driver accepted
 *  both, silently writing a CR2.STOP encoding it had no way to honor. */
void test_uart_driver_init_rejects_smartcard_only_stop_bits(void)
{
    static const uint32_t smartcard_stop_values[] = {USART_CR2_STOP_0_5, USART_CR2_STOP_1_5};

    for (size_t i = 0; i < sizeof(smartcard_stop_values) / sizeof(smartcard_stop_values[0]); i++)
    {
        UartRegisters_t uart = {0};

        DriverStatus_e status =
            uartInit(&uart, 16000000U, 9600U, smartcard_stop_values[i], 0U, 0U, USART_CR1_TE);

        TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
        TEST_ASSERT_EQUAL_HEX32(0U, uart.CR2);
    }
}

/** Every one of STOP's 2 documented values must independently report OK
 *  and land in CR2.STOP - the chained != comparisons short-circuit at a
 *  different clause for each value, so a single valid-value test only
 *  exercises one clause's false outcome; gcov branch coverage requires
 *  each of the 2 individually (same reasoning as rcc.c's HPRE/PPRE
 *  exhaustive tests). Asserting the field's placement, not just the
 *  status, catches a validator that accepts a value but writes it to the
 *  wrong field or shift - status alone would not. */
void test_uart_driver_init_accepts_every_documented_stop_bits(void)
{
    static const uint32_t stop_values[] = {USART_CR2_STOP_1, USART_CR2_STOP_2};

    for (size_t i = 0; i < sizeof(stop_values) / sizeof(stop_values[0]); i++)
    {
        UartRegisters_t uart = {0};

        DriverStatus_e status =
            uartInit(&uart, 16000000U, 9600U, stop_values[i], 0U, 0U, USART_CR1_TE);

        TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
        TEST_ASSERT_EQUAL_HEX32(stop_values[i],
                                (uart.CR2 & USART_CR2_STOP_Msk) >> USART_CR2_STOP_Pos);
    }
}

/** parity_enable outside {0, PCE} must be rejected. */
void test_uart_driver_init_rejects_invalid_parity_enable(void)
{
    UartRegisters_t uart = {0};

    DriverStatus_e status =
        uartInit(&uart, 16000000U, 9600U, USART_CR2_STOP_1, 0xFFU, 0U, USART_CR1_TE);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, uart.CR1);
}

/** parity_select outside {0, PS} must be rejected. */
void test_uart_driver_init_rejects_invalid_parity_select(void)
{
    UartRegisters_t uart = {0};

    DriverStatus_e status =
        uartInit(&uart, 16000000U, 9600U, USART_CR2_STOP_1, 0U, 0xFFU, USART_CR1_TE);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, uart.CR1);
}

/** direction outside {0, TE, RE, TE|RE} must be rejected. */
void test_uart_driver_init_rejects_invalid_direction(void)
{
    UartRegisters_t uart = {0};

    DriverStatus_e status =
        uartInit(&uart, 16000000U, 9600U, USART_CR2_STOP_1, 0U, 0U, USART_CR1_SBK);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_INVALID_PARAM, status);
    TEST_ASSERT_EQUAL_HEX32(0U, uart.CR1);
}

/* --- uartInit: field placement --- */

/** A valid call sets PCE/PS/TE/RE/UE in CR1, STOP in CR2, and BRR -
 *  preserves every other CR1/CR2 bit, and leaves CR1.M clear (8 data
 *  bits - this driver never sets it; see the dedicated M-clearing test
 *  below for the case where it starts set). */
void test_uart_driver_init_sets_fields_and_preserves_other_bits(void)
{
    UartRegisters_t uart = {.CR1 = 0xFFFFFFFFU
                                   & ~(USART_CR1_M | USART_CR1_PCE | USART_CR1_PS | USART_CR1_TE
                                       | USART_CR1_RE | USART_CR1_UE),
                            .CR2 = 0xFFFFFFFFU & ~USART_CR2_STOP_Msk};

    DriverStatus_e status = uartInit(&uart, 45000000U, 115200U, USART_CR2_STOP_2, USART_CR1_PCE,
                                     USART_CR1_PS, USART_CR1_TE | USART_CR1_RE);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(USART_CR1_PCE | USART_CR1_PS | USART_CR1_TE | USART_CR1_RE
                                | USART_CR1_UE,
                            uart.CR1
                                & (USART_CR1_M | USART_CR1_PCE | USART_CR1_PS | USART_CR1_TE
                                   | USART_CR1_RE | USART_CR1_UE));
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU
                                & ~(USART_CR1_M | USART_CR1_PCE | USART_CR1_PS | USART_CR1_TE
                                    | USART_CR1_RE | USART_CR1_UE),
                            uart.CR1
                                & ~(USART_CR1_M | USART_CR1_PCE | USART_CR1_PS | USART_CR1_TE
                                    | USART_CR1_RE | USART_CR1_UE));
    TEST_ASSERT_EQUAL_HEX32(USART_CR2_STOP_2,
                            (uart.CR2 & USART_CR2_STOP_Msk) >> USART_CR2_STOP_Pos);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU & ~USART_CR2_STOP_Msk, uart.CR2 & ~USART_CR2_STOP_Msk);
}

/** CR1.M starting set (e.g. left over from external tooling, or simply
 *  the register's undefined post-reset content in this fake struct) is
 *  unconditionally cleared - this driver only ever supports 8 data bits
 *  (see uart.h's file-level comment on why 9-bit is not offered), so a
 *  stale M bit must never survive a fresh uartInit() call. */
void test_uart_driver_init_clears_preexisting_m_bit(void)
{
    UartRegisters_t uart = {.CR1 = USART_CR1_M};

    DriverStatus_e status =
        uartInit(&uart, 16000000U, 9600U, USART_CR2_STOP_1, 0U, 0U, USART_CR1_TE);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(0U, uart.CR1 & USART_CR1_M);
}

/** No parity, TE only clears PCE/PS/RE while still setting UE - the
 *  all-defaults path the case above's non-default values don't cover. */
void test_uart_driver_init_defaults_clear_parity_and_re(void)
{
    UartRegisters_t uart = {.CR1 = USART_CR1_PCE | USART_CR1_PS | USART_CR1_RE};

    DriverStatus_e status =
        uartInit(&uart, 16000000U, 9600U, USART_CR2_STOP_1, 0U, 0U, USART_CR1_TE);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(USART_CR1_TE | USART_CR1_UE, uart.CR1);
}

/** Reconfiguring an instance that is already enabled still ends with UE
 *  set and every field at its new value. The driver clears UE before
 *  touching BRR and the framing fields, as RM0390 requires them to be
 *  programmed with the USART disabled, then sets it again last - so a
 *  baud-rate change on a running port is a legal sequence rather than a
 *  write into a live peripheral. */
void test_uart_driver_init_reconfigures_an_already_enabled_instance(void)
{
    UartRegisters_t uart = {.CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE, .BRR = 0xFFFFU};

    DriverStatus_e status =
        uartInit(&uart, 16000000U, 9600U, USART_CR2_STOP_1, 0U, 0U, USART_CR1_TE);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    /* 16 MHz / 9600 -> USARTDIV 104.1667 -> mantissa 104, fraction 3. */
    TEST_ASSERT_EQUAL_HEX32((104U << USART_BRR_DIV_MANTISSA_Pos) | 3U, uart.BRR);
    TEST_ASSERT_EQUAL_HEX32(USART_CR1_TE | USART_CR1_UE, uart.CR1);
}

/* --- uartDeinit --- */

/** Resets CR1/CR2/CR3/BRR/GTPR to 0. */
void test_uart_driver_deinit_resets_all_registers(void)
{
    UartRegisters_t uart = {.CR1 = 0xFFFFFFFFU,
                            .CR2 = 0xFFFFFFFFU,
                            .CR3 = 0xFFFFFFFFU,
                            .BRR = 0xFFFFFFFFU,
                            .GTPR = 0xFFFFFFFFU};

    uartDeinit(&uart);

    TEST_ASSERT_EQUAL_HEX32(0U, uart.CR1);
    TEST_ASSERT_EQUAL_HEX32(0U, uart.CR2);
    TEST_ASSERT_EQUAL_HEX32(0U, uart.CR3);
    TEST_ASSERT_EQUAL_HEX32(0U, uart.BRR);
    TEST_ASSERT_EQUAL_HEX32(0U, uart.GTPR);
}

/* --- uartTransmitByte / uartTransmit --- */

/** TXE (pre-set here) is observed immediately -> the byte is written to
 *  DR and OK is reported. */
void test_uart_driver_transmit_byte_writes_dr_when_txe_ready(void)
{
    UartRegisters_t uart = {.SR = USART_SR_TXE};

    DriverStatus_e status = uartTransmitByte(&uart, 0x42U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(0x42U, uart.DR);
}

/** TXE never sets -> the bounded retry exhausts and reports a timeout;
 *  DR is never written. */
void test_uart_driver_transmit_byte_times_out_when_txe_never_sets(void)
{
    UartRegisters_t uart = {0};

    DriverStatus_e status = uartTransmitByte(&uart, 0x42U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, status);
    TEST_ASSERT_EQUAL_HEX32(0U, uart.DR);
}

/** Every byte in the buffer is written to DR in order, when TXE and TC
 *  are always ready. */
void test_uart_driver_transmit_sends_every_byte_in_order(void)
{
    UartRegisters_t uart = {.SR = USART_SR_TXE | USART_SR_TC};
    static const uint8_t data[] = {0x11U, 0x22U, 0x33U};

    DriverStatus_e status = uartTransmit(&uart, data, sizeof(data));

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    /* Only the last byte written is observable in this fake register -
     *  DR overwrites each call, matching real hardware. Confirms the
     *  loop reached the final element without an early failure. */
    TEST_ASSERT_EQUAL_HEX32(0x33U, uart.DR);
}

/** A zero-length buffer is a trivial success - no bytes, no TXE poll,
 *  and crucially no TC wait either: nothing was started, so TC still
 *  reflects whatever a previous transfer left behind and waiting on it
 *  would block on unrelated state. SR is cleared here, so a TC wait
 *  would time out and this would fail. */
void test_uart_driver_transmit_zero_length_reports_ok(void)
{
    UartRegisters_t uart = {0};

    DriverStatus_e status = uartTransmit(&uart, NULL, 0U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
}

/** A non-empty transmit waits for TC before reporting success: every
 *  byte reaches DR (TXE always ready), but TC never sets, so the closing
 *  flush times out rather than claiming the buffer is on the wire. This
 *  is the truncation case - returning OK here would let a caller gate
 *  the clock off mid-character. */
void test_uart_driver_transmit_times_out_when_tc_never_sets(void)
{
    UartRegisters_t uart = {.SR = USART_SR_TXE};
    static const uint8_t data[] = {0x11U, 0x22U};

    DriverStatus_e status = uartTransmit(&uart, data, sizeof(data));

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, status);
    TEST_ASSERT_EQUAL_HEX32(0x22U, uart.DR);
}

/** A mid-buffer timeout stops the loop and propagates the failure -
 *  TXE is pre-cleared here, so the very first byte times out. */
void test_uart_driver_transmit_propagates_timeout(void)
{
    UartRegisters_t uart = {0};
    static const uint8_t data[] = {0x11U, 0x22U};

    DriverStatus_e status = uartTransmit(&uart, data, sizeof(data));

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, status);
}

/* --- uartFlush --- */

/** TC (pre-set here) is observed immediately -> OK. */
void test_uart_driver_flush_reports_ok_when_tc_set(void)
{
    UartRegisters_t uart = {.SR = USART_SR_TC};

    DriverStatus_e status = uartFlush(&uart);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
}

/** TC never sets -> the bounded retry exhausts and reports a timeout. */
void test_uart_driver_flush_times_out_when_tc_never_sets(void)
{
    UartRegisters_t uart = {0};

    DriverStatus_e status = uartFlush(&uart);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, status);
}

/** The flush only reads SR - it must not write DR, which would both
 *  start a spurious transmission and clear TC as a side effect. */
void test_uart_driver_flush_does_not_write_dr(void)
{
    UartRegisters_t uart = {.SR = USART_SR_TC, .DR = 0x5AU};

    DriverStatus_e status = uartFlush(&uart);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(0x5AU, uart.DR);
}

/* --- uartReceiveByte / uartReceive --- */

/** RXNE (pre-set here) is observed immediately, with no fault flags set
 *  -> DR's value is read into *byte and OK is reported. */
void test_uart_driver_receive_byte_reads_dr_when_no_fault(void)
{
    UartRegisters_t uart = {.SR = USART_SR_RXNE, .DR = 0x7AU};
    uint8_t byte = 0U;

    DriverStatus_e status = uartReceiveByte(&uart, &byte);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX8(0x7AU, byte);
}

/** RXNE never sets -> the bounded retry exhausts and reports a timeout;
 *  *byte is never touched. */
void test_uart_driver_receive_byte_times_out_when_rxne_never_sets(void)
{
    UartRegisters_t uart = {0};
    uint8_t byte = 0xAAU;

    DriverStatus_e status = uartReceiveByte(&uart, &byte);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, status);
    TEST_ASSERT_EQUAL_HEX8(0xAAU, byte);
}

/** RXNE set alongside ORE -> DR is still read into *byte (draining the
 *  byte and, on real hardware, clearing the flag), but HW_FAULT is
 *  reported. */
void test_uart_driver_receive_byte_reports_fault_but_still_reads_dr(void)
{
    UartRegisters_t uart = {.SR = USART_SR_RXNE | USART_SR_ORE, .DR = 0x55U};
    uint8_t byte = 0U;

    DriverStatus_e status = uartReceiveByte(&uart, &byte);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_HW_FAULT, status);
    TEST_ASSERT_EQUAL_HEX8(0x55U, byte);
}

/** Each of NE/FE/PE independently triggers the same fault path as ORE
 *  above - confirms the driver checks all four documented fault bits,
 *  not just ORE. */
void test_uart_driver_receive_byte_reports_fault_for_ne_fe_pe(void)
{
    static const uint32_t fault_bits[] = {USART_SR_NE, USART_SR_FE, USART_SR_PE};

    for (size_t i = 0; i < sizeof(fault_bits) / sizeof(fault_bits[0]); i++)
    {
        UartRegisters_t uart = {.SR = USART_SR_RXNE | fault_bits[i]};
        uint8_t byte = 0U;

        DriverStatus_e status = uartReceiveByte(&uart, &byte);

        TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_HW_FAULT, status);
    }
}

/** Every byte in the buffer is read from DR in order, when RXNE is
 *  always ready with no fault. */
void test_uart_driver_receive_reads_every_byte_in_order(void)
{
    UartRegisters_t uart = {.SR = USART_SR_RXNE, .DR = 0x99U};
    uint8_t data[3] = {0};

    DriverStatus_e status = uartReceive(&uart, data, sizeof(data));

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX8(0x99U, data[0]);
    TEST_ASSERT_EQUAL_HEX8(0x99U, data[1]);
    TEST_ASSERT_EQUAL_HEX8(0x99U, data[2]);
}

/** A zero-length buffer is a trivial success - no bytes, no RXNE poll. */
void test_uart_driver_receive_zero_length_reports_ok(void)
{
    UartRegisters_t uart = {0};

    DriverStatus_e status = uartReceive(&uart, NULL, 0U);

    TEST_ASSERT_EQUAL(DRIVER_STATUS_OK, status);
}

/** A mid-buffer timeout stops the loop and propagates the failure. */
void test_uart_driver_receive_propagates_timeout(void)
{
    UartRegisters_t uart = {0};
    uint8_t data[2] = {0};

    DriverStatus_e status = uartReceive(&uart, data, sizeof(data));

    TEST_ASSERT_EQUAL(DRIVER_STATUS_ERR_TIMEOUT, status);
}
