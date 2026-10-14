/**
 * @file driver_status.h
 * @brief The status every fallible driver function returns.
 *
 * - A function that can fail returns ::DriverStatus_e and is
 *   ::DRIVER_MUST_CHECK.
 * - One that cannot fail returns `void`.
 * - Compare explicitly: `if (status != DRIVER_STATUS_OK)`, never `if (status)`.
 *
 * See [Error contract](DESIGN.md#error-contract) for the rules and their
 * one exception.
 */
#ifndef DRIVER_STATUS_H
#define DRIVER_STATUS_H

/**
 * @addtogroup driver_layer
 * @{
 */

/** Make ignoring a function's result a compile error (with `-Werror`). */
#define DRIVER_MUST_CHECK __attribute__((warn_unused_result))

/**
 * @brief Outcome of a driver call: a category for control flow, not a diagnosis.
 *
 * Success is deliberately not 0, so a status variable a code path forgot
 * to assign never reads as success. Add a per-module detail code beside it
 * only when a driver needs one; never add peripheral-specific values here.
 */
typedef enum DriverStatus_e
{
    /** Never returned. The variable was read before any call assigned it. */
    DRIVER_STATUS_UNINITIALIZED = 0,
    /** Succeeded. */
    DRIVER_STATUS_OK = 1,
    /** A parameter is outside its valid domain. Nothing was written. */
    DRIVER_STATUS_ERR_INVALID_PARAM = 2,
    /** The peripheral or clock source was used before it was enabled or ready. */
    DRIVER_STATUS_ERR_NOT_INITIALIZED = 3,
    /** A bounded wait for a hardware flag ran out. */
    DRIVER_STATUS_ERR_TIMEOUT = 4,
    /** The hardware answered with a fault, such as a UART framing error or a rejected write. */
    DRIVER_STATUS_ERR_HW_FAULT = 5,
    /** The hardware is in a state that makes the request unsafe, such as a running PLL. */
    DRIVER_STATUS_ERR_BUSY = 6,
    /** Valid on the hardware, but not implemented by this driver. */
    DRIVER_STATUS_ERR_UNSUPPORTED = 7,
} DriverStatus_e;

/** @} */

#endif /* DRIVER_STATUS_H */
