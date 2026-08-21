#ifndef SYSTICK_REG_H
#define SYSTICK_REG_H

#include <stdint.h>

#define SYSTICK_BASE (0xE000E010UL)

typedef struct
{
    volatile uint32_t CTRL;  // 0x00: control and status register
    volatile uint32_t LOAD;  // 0x04: reload value register
    volatile uint32_t VAL;   // 0x08: current value register
    volatile uint32_t CALIB; // 0x0C: calibration value register
} SysTickRegisters_t;

#define SYSTICK ((SysTickRegisters_t *)SYSTICK_BASE)

/* --- SYSTICK_CTRL bit definitions --- */
#define SYSTICK_CTRL_ENABLE_Pos    (0U)
#define SYSTICK_CTRL_ENABLE        (1U << SYSTICK_CTRL_ENABLE_Pos)    // WRITE
#define SYSTICK_CTRL_CLKSOURCE_Pos (2U)
#define SYSTICK_CTRL_CLKSOURCE     (1U << SYSTICK_CTRL_CLKSOURCE_Pos) // WRITE, 1=processor clock
#define SYSTICK_CTRL_COUNTFLAG_Pos (16U)
#define SYSTICK_CTRL_COUNTFLAG     (1U << SYSTICK_CTRL_COUNTFLAG_Pos) // READ only, clears on read

#endif /* SYSTICK_REG_H */
