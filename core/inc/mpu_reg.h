/**
 * @file mpu_reg.h
 * @brief MPU (Memory Protection Unit) register definitions for the ARM
 *        Cortex-M4 (Armv7-M architecture).
 *
 * Register layout derived from PM0214 (Cortex-M4 programming manual).
 * Architectural rather than vendor-specific, fixed by Armv7-M's System
 * Control Space memory map. The STM32F446's Cortex-M4 implements 8
 * regions (readable at runtime via TYPE.DREGION).
 *
 * The RBAR_A1/RASR_A1..A3 alias pairs allow a driver to reprogram up to
 * 4 regions consecutively without rewriting RNR between each one. This
 * is not required for correctness (RNR, RBAR, and RASR alone can
 * configure any region); it is an optional throughput optimization not
 * currently used by this RTOS.
 */
#ifndef MPU_REG_H
#define MPU_REG_H

#include <stdint.h>

/**
 * @addtogroup mpu_registers
 * @{
 */

#define MPU_BASE (0xE000ED90UL) ///< MPU peripheral base address (Armv7-M SCS)

/**
 * @brief MPU register map (PM0214, Armv7-M SCS MPU region).
 */
typedef struct MpuRegisters_t
{
  volatile const uint32_t TYPE; ///< 0x00: MPU type register (READ only)
  volatile uint32_t CTRL;       ///< 0x04: MPU control register
  volatile uint32_t RNR;        ///< 0x08: MPU region number register
  volatile uint32_t RBAR;       ///< 0x0C: MPU region base address register
  volatile uint32_t RASR;       ///< 0x10: MPU region attribute and size register
  volatile uint32_t RBAR_A1;    ///< 0x14: RBAR alias 1
  volatile uint32_t RASR_A1;    ///< 0x18: RASR alias 1
  volatile uint32_t RBAR_A2;    ///< 0x1C: RBAR alias 2
  volatile uint32_t RASR_A2;    ///< 0x20: RASR alias 2
  volatile uint32_t RBAR_A3;    ///< 0x24: RBAR alias 3
  volatile uint32_t RASR_A3;    ///< 0x28: RASR alias 3
} MpuRegisters_t;

#define MPU ((MpuRegisters_t *)MPU_BASE) ///< Pointer to the MPU register block

/* --- MPU_TYPE bit definitions --- */
#define MPU_TYPE_SEPARATE_Pos (0U)                      ///< Bit position within MPU_TYPE
#define MPU_TYPE_SEPARATE (1U << MPU_TYPE_SEPARATE_Pos) ///< READ only - separate I/D memory map
#define MPU_TYPE_DREGION_Pos (8U)                       ///< Bit position within MPU_TYPE
#define MPU_TYPE_DREGION_Msk (0xFFU << MPU_TYPE_DREGION_Pos) ///< bits 15:8, data region count

/* --- MPU_CTRL bit definitions --- */
#define MPU_CTRL_ENABLE_Pos (0U)                        ///< Bit position within MPU_CTRL
#define MPU_CTRL_ENABLE (1U << MPU_CTRL_ENABLE_Pos)     ///< WRITE - enable the MPU
#define MPU_CTRL_HFNMIENA_Pos (1U)                      ///< Bit position within MPU_CTRL
#define MPU_CTRL_HFNMIENA (1U << MPU_CTRL_HFNMIENA_Pos) ///< WRITE - keep MPU active in fault/NMI
#define MPU_CTRL_PRIVDEFENA_Pos (2U)                    ///< Bit position within MPU_CTRL
#define MPU_CTRL_PRIVDEFENA (1U << MPU_CTRL_PRIVDEFENA_Pos) ///< WRITE - default map as background

/* --- MPU_RNR bit definitions --- */
#define MPU_RNR_REGION_Pos (0U)                          ///< Bit position within MPU_RNR
#define MPU_RNR_REGION_Msk (0xFFU << MPU_RNR_REGION_Pos) ///< bits 7:0, selected region number

/* --- MPU_RBAR bit definitions --- */
#define MPU_RBAR_REGION_Pos (0U)                          ///< Bit position within MPU_RBAR
#define MPU_RBAR_REGION_Msk (0xFU << MPU_RBAR_REGION_Pos) ///< bits 3:0, region number
#define MPU_RBAR_VALID_Pos (4U)                           ///< Bit position within MPU_RBAR
#define MPU_RBAR_VALID (1U << MPU_RBAR_VALID_Pos) ///< WRITE - also load RNR from REGION field
#define MPU_RBAR_ADDR_Pos (5U)                    ///< Bit position within MPU_RBAR
#define MPU_RBAR_ADDR_Msk (0x7FFFFFFU << MPU_RBAR_ADDR_Pos) ///< bits 31:5, region base address

/* --- MPU_RASR bit definitions --- */
#define MPU_RASR_ENABLE_Pos (0U)                       ///< Bit position within MPU_RASR
#define MPU_RASR_ENABLE (1U << MPU_RASR_ENABLE_Pos)    ///< WRITE - enable this region
#define MPU_RASR_SIZE_Pos (1U)                         ///< Bit position within MPU_RASR
#define MPU_RASR_SIZE_Msk (0x1FU << MPU_RASR_SIZE_Pos) ///< bits 5:1, region size = 2^(SIZE+1) bytes
#define MPU_RASR_SRD_Pos (8U)                          ///< Bit position within MPU_RASR
#define MPU_RASR_SRD_Msk (0xFFU << MPU_RASR_SRD_Pos)   ///< bits 15:8, subregion disable
#define MPU_RASR_B_Pos (16U)                           ///< Bit position within MPU_RASR
#define MPU_RASR_B (1U << MPU_RASR_B_Pos)              ///< bufferable
#define MPU_RASR_C_Pos (17U)                           ///< Bit position within MPU_RASR
#define MPU_RASR_C (1U << MPU_RASR_C_Pos)              ///< cacheable
#define MPU_RASR_S_Pos (18U)                           ///< Bit position within MPU_RASR
#define MPU_RASR_S (1U << MPU_RASR_S_Pos)              ///< shareable
#define MPU_RASR_TEX_Pos (19U)                         ///< Bit position within MPU_RASR
#define MPU_RASR_TEX_Msk (0x7U << MPU_RASR_TEX_Pos)    ///< bits 21:19, type extension
#define MPU_RASR_AP_Pos (24U)                          ///< Bit position within MPU_RASR
#define MPU_RASR_AP_Msk (0x7U << MPU_RASR_AP_Pos)      ///< bits 26:24, access permission
#define MPU_RASR_XN_Pos (28U)                          ///< Bit position within MPU_RASR
#define MPU_RASR_XN (1U << MPU_RASR_XN_Pos)            ///< execute never

/* --- MPU_RASR.AP field values (PM0214 Table "AP encoding") --- */
#define MPU_RASR_AP_NONE (0x0U)    ///< AP field value: no access, any privilege
#define MPU_RASR_AP_PRIV_RW (0x1U) ///< AP field value: privileged RW, no unprivileged access
#define MPU_RASR_AP_PRIV_RW_UNPRIV_RO (0x2U) ///< AP field value: privileged RW, unprivileged RO
#define MPU_RASR_AP_FULL_RW (0x3U)           ///< AP field value: full read/write, any privilege
#define MPU_RASR_AP_PRIV_RO (0x5U)           ///< AP field value: privileged read-only
#define MPU_RASR_AP_FULL_RO (0x6U)           ///< AP field value: read-only, any privilege

/** @} */

#endif /* MPU_REG_H */
