/**
 * @file driver_status.h
 * @brief Shared error-status contract for every driver in drivers/.
 *
 * DriverStatus_e is a small, closed set of failure *categories* used
 * for control-flow decisions at the call site (retry, halt, propagate).
 * It is not a substitute for module-specific failure detail (which
 * check failed, on which peripheral, at which register). Introduce a
 * per-module detail enum, returned alongside DriverStatus_e via an
 * out-parameter, only once a specific driver actually requires that
 * granularity. Do not add peripheral-specific values to this enum.
 *
 * Every driver function that can genuinely fail - a bounded hardware
 * wait times out, the hardware reports a real fault via a status
 * flag, a caller uses a peripheral before it is initialized, or a
 * runtime-supplied parameter is outside its valid domain - returns
 * DriverStatus_e, marked ::DRIVER_MUST_CHECK. A function with no
 * possible failure mode (a single deterministic register write, with
 * no polling and nothing hardware-reportable) returns void instead;
 * never return DriverStatus_e defensively where nothing can fail.
 *
 * Callers compare explicitly: `if (status != DRIVER_STATUS_OK)`. Never
 * `if (status)` / `if (!status)` - this relies implicitly on
 * DRIVER_STATUS_OK's numeric value and breaks silently if the enum is
 * ever reordered.
 */
#ifndef DRIVER_STATUS_H
#define DRIVER_STATUS_H

/**
 * @addtogroup driver_layer
 * @{
 */

/**
 * @brief Marks a function's return value as mandatory to check.
 *
 * Wraps the underlying compiler attribute so the mechanism lives in
 * one place. Supported by both toolchains this project builds
 * drivers with: arm-none-eabi-gcc (target) and the host gcc/clang
 * `cc` used by tests/unit/ (see the Makefile's HOST_CC).
 */
#define DRIVER_MUST_CHECK __attribute__((warn_unused_result))

/**
 * @brief Result of a driver operation.
 *
 * DRIVER_STATUS_OK is deliberately not 0; DRIVER_STATUS_UNINITIALIZED
 * occupies 0 instead. This project builds drivers at -O0 (see the
 * Makefile's CFLAGS), where the compiler's uninitialized-variable
 * analysis is weakest. If a DriverStatus_e variable is declared and a
 * code path fails to assign it, it must not read as success by
 * coincidence; naming 0 explicitly also gives a `switch` over this
 * enum a concrete, documented value for its `default` case to catch,
 * rather than an implicit, unnamed gap. Combined with the mandatory
 * explicit-comparison rule above, DRIVER_STATUS_UNINITIALIZED is never
 * a value a driver function deliberately returns.
 */
typedef enum DriverStatus_e
{
  /** A DriverStatus_e variable was read before being assigned by a
   *  driver function. Never returned deliberately - its presence
   *  means a code path failed to set the status it claims to report. */
  DRIVER_STATUS_UNINITIALIZED = 0,

  /** Operation completed successfully. */
  DRIVER_STATUS_OK = 1,

  /** A runtime-supplied parameter is outside its valid domain. */
  DRIVER_STATUS_ERR_INVALID_PARAM = 2,

  /** Peripheral was used before its init/config function ran. */
  DRIVER_STATUS_ERR_NOT_INITIALIZED = 3,

  /** A bounded wait on a hardware status flag expired before the
   *  expected condition became true. */
  DRIVER_STATUS_ERR_TIMEOUT = 4,

  /** The hardware reported a real fault via a status/error flag
   *  (e.g. USART_SR.ORE/FE/PE, FLASH_SR.WRPERR). Distinct from
   *  DRIVER_STATUS_ERR_TIMEOUT: the hardware responded, and what it
   *  reported was a fault. */
  DRIVER_STATUS_ERR_HW_FAULT = 5,

  /** The peripheral is mid-operation and cannot accept a new request
   *  right now. Reserved for non-blocking/interrupt-driven drivers;
   *  unused by a purely blocking/polled driver. */
  DRIVER_STATUS_ERR_BUSY = 6,

  /** The requested configuration is not implemented by this driver,
   *  even if it is architecturally valid on the peripheral. */
  DRIVER_STATUS_ERR_UNSUPPORTED = 7,
} DriverStatus_e;

/** @} */

#endif /* DRIVER_STATUS_H */
