/**
 * @file mpu_reg.h
 * @brief MPU registers: the Armv7-M memory protection unit (PM0214, MPU chapter).
 *
 * The F446's Cortex-M4 has 8 regions (TYPE.DREGION). A region is selected
 * through RNR, or through RBAR with VALID set. The RBAR/RASR aliases let up to
 * four regions be written back to back without rewriting RNR.
 */
#ifndef MPU_REG_H
#define MPU_REG_H

#include <stdint.h>

/**
 * @addtogroup mpu_registers
 * @{
 */

#define MPU_BASE (0xE000ED90UL) ///< MPU base address (System Control Space)

/** @brief MPU register block. */
typedef struct MpuRegisters_t
{
    volatile const uint32_t TYPE; ///< 0x00: type, \regaccess{ro}
    volatile uint32_t CTRL;       ///< 0x04: control
    volatile uint32_t RNR;        ///< 0x08: region number
    volatile uint32_t RBAR;       ///< 0x0C: region base address
    volatile uint32_t RASR;       ///< 0x10: region attribute and size
    volatile uint32_t RBAR_A1;    ///< 0x14: RBAR alias 1
    volatile uint32_t RASR_A1;    ///< 0x18: RASR alias 1
    volatile uint32_t RBAR_A2;    ///< 0x1C: RBAR alias 2
    volatile uint32_t RASR_A2;    ///< 0x20: RASR alias 2
    volatile uint32_t RBAR_A3;    ///< 0x24: RBAR alias 3
    volatile uint32_t RASR_A3;    ///< 0x28: RASR alias 3
} MpuRegisters_t;

#define MPU ((MpuRegisters_t *)MPU_BASE) ///< The MPU instance

/** @name TYPE - MPU type (0x00)
 *  @{ */
#define MPU_TYPE_SEPARATE_Pos (0U) ///< Bit 0
/** \regaccess{ro} Separate instruction and data maps (0 on the Cortex-M4). */
#define MPU_TYPE_SEPARATE ((uint32_t)1U << MPU_TYPE_SEPARATE_Pos)
#define MPU_TYPE_DREGION_Pos (8U) ///< Bits 15:8
/** \regaccess{ro} Number of regions. */
#define MPU_TYPE_DREGION_Msk ((uint32_t)0xFFU << MPU_TYPE_DREGION_Pos)
/** @} */

/** @name CTRL - control (0x04)
 *  @{ */
#define MPU_CTRL_ENABLE_Pos (0U) ///< Bit 0
/** \regaccess{rw} Enable the MPU. */
#define MPU_CTRL_ENABLE ((uint32_t)1U << MPU_CTRL_ENABLE_Pos)
#define MPU_CTRL_HFNMIENA_Pos (1U) ///< Bit 1
/** \regaccess{rw} Keep the MPU on in HardFault, NMI and FAULTMASK handlers. */
#define MPU_CTRL_HFNMIENA ((uint32_t)1U << MPU_CTRL_HFNMIENA_Pos)
#define MPU_CTRL_PRIVDEFENA_Pos (2U) ///< Bit 2
/** \regaccess{rw} Privileged code may use the default memory map as a background region. */
#define MPU_CTRL_PRIVDEFENA ((uint32_t)1U << MPU_CTRL_PRIVDEFENA_Pos)
/** @} */

/** @name RNR - region number (0x08)
 *  @{ */
#define MPU_RNR_REGION_Pos (0U) ///< Bits 7:0
/** \regaccess{rw} Region that RBAR and RASR access. */
#define MPU_RNR_REGION_Msk ((uint32_t)0xFFU << MPU_RNR_REGION_Pos)
/** @} */

/** @name RBAR - region base address (0x0C)
 *  @{ */
#define MPU_RBAR_REGION_Pos (0U) ///< Bits 3:0
/** \regaccess{rw} Region number, used when VALID is set. */
#define MPU_RBAR_REGION_Msk ((uint32_t)0xFU << MPU_RBAR_REGION_Pos)
#define MPU_RBAR_VALID_Pos (4U) ///< Bit 4
/** \regaccess{wo} Write REGION into RNR as part of this write. */
#define MPU_RBAR_VALID ((uint32_t)1U << MPU_RBAR_VALID_Pos)
#define MPU_RBAR_ADDR_Pos (5U) ///< Bits 31:5
/** \regaccess{rw} Region base address, aligned to the region size. */
#define MPU_RBAR_ADDR_Msk ((uint32_t)0x7FFFFFFU << MPU_RBAR_ADDR_Pos)
/** @} */

/** @name RASR - region attribute and size (0x10)
 *  Every field is \regaccess{rw}.
 *  @{ */
#define MPU_RASR_ENABLE_Pos (0U)                              ///< Bit 0
#define MPU_RASR_ENABLE ((uint32_t)1U << MPU_RASR_ENABLE_Pos) ///< Enable this region
#define MPU_RASR_SIZE_Pos (1U)                                ///< Bits 5:1
/** Region size: 2^(SIZE + 1) bytes, minimum 32. */
#define MPU_RASR_SIZE_Msk ((uint32_t)0x1FU << MPU_RASR_SIZE_Pos)
#define MPU_RASR_SRD_Pos (8U) ///< Bits 15:8
/** Subregion disable: one bit per eighth of the region. */
#define MPU_RASR_SRD_Msk ((uint32_t)0xFFU << MPU_RASR_SRD_Pos)
#define MPU_RASR_B_Pos (16U)                                  ///< Bit 16
#define MPU_RASR_B ((uint32_t)1U << MPU_RASR_B_Pos)           ///< Bufferable
#define MPU_RASR_C_Pos (17U)                                  ///< Bit 17
#define MPU_RASR_C ((uint32_t)1U << MPU_RASR_C_Pos)           ///< Cacheable
#define MPU_RASR_S_Pos (18U)                                  ///< Bit 18
#define MPU_RASR_S ((uint32_t)1U << MPU_RASR_S_Pos)           ///< Shareable
#define MPU_RASR_TEX_Pos (19U)                                ///< Bits 21:19
#define MPU_RASR_TEX_Msk ((uint32_t)0x7U << MPU_RASR_TEX_Pos) ///< Type extension
#define MPU_RASR_AP_Pos (24U)                                 ///< Bits 26:24
#define MPU_RASR_AP_Msk ((uint32_t)0x7U << MPU_RASR_AP_Pos)   ///< Access permission; values below
#define MPU_RASR_XN_Pos (28U)                                 ///< Bit 28
#define MPU_RASR_XN ((uint32_t)1U << MPU_RASR_XN_Pos)         ///< Execute never
/** @} */

/** @name RASR.AP values
 *  @{ */
#define MPU_RASR_AP_NONE (0x0U)              ///< No access
#define MPU_RASR_AP_PRIV_RW (0x1U)           ///< Privileged read/write only
#define MPU_RASR_AP_PRIV_RW_UNPRIV_RO (0x2U) ///< Privileged read/write, unprivileged read
#define MPU_RASR_AP_FULL_RW (0x3U)           ///< Full read/write
#define MPU_RASR_AP_PRIV_RO (0x5U)           ///< Privileged read only
#define MPU_RASR_AP_FULL_RO (0x6U)           ///< Read only
/** @} */

/** @} */

#endif /* MPU_REG_H */
