/*
 * startup_stm32f446re.S
 *
 * STM32F446RE (Cortex-M4F) startup file - written from scratch.
 *
 * Responsibilities:
 *   1. Define the vector table (isr_vector) - the CPU reads this directly
 *      on reset: word 0 = initial SP, word 1 = Reset_Handler address,
 *      remaining words = exception/interrupt handler addresses indexed
 *      by IRQ number.
 *   2. Implement Reset_Handler - enables the FPU and sets the interrupt
 *      priority grouping (see the SCB_CPACR/SCB_AIRCR comment below),
 *      copies .data from flash to RAM, zeroes .bss, then calls main().
 *      Uses symbols exported by the linker script
 *      (linker/STM32F446RE.ld): _sidata, _sdata, _edata, _sbss, _ebss,
 *      _estack.
 *   3. Provide a Default_Handler and weak aliases for every interrupt,
 *      so an unimplemented interrupt has somewhere safe to land instead
 *      of jumping to garbage.
 *
 * No .fpu directive is set below: this file contains no VFP instructions
 * of its own, so the assembler needs no FPU mode for it, and the actual
 * hard-float codegen for main.c and everything above it is already
 * governed by the Makefile's -mfpu=fpv4-sp-d16 -mfloat-abi=hard on each
 * C translation unit, not by anything in this .s file.
 */

.syntax unified
.cpu cortex-m4
.thumb

.global vector_table
.global Reset_Handler
.global Default_Handler

/* ========================================================================
 * Reset_Handler
 * First code the CPU executes after reset. Runs before any C runtime
 * setup exists, so this has to be assembly.
 * ==================================================================== */
.section .text.Reset_Handler
.weak Reset_Handler
.type Reset_Handler, %function

Reset_Handler:
    ldr   sp, =_estack          /* set stack pointer to top of RAM */

    /* Enable the FPU: SCB->CPACR CP10/CP11 = full access (core/inc/scb_reg.h
     * SCB_CPACR_CP10_Msk|SCB_CPACR_CP11_Msk, PM0214 4.6.7). Every C file is
     * built with -mfpu=fpv4-sp-d16 -mfloat-abi=hard, so any VFP instruction
     * the compiler emits before this runs would trap as a UsageFault
     * (NOCP) - do this before any C code, not lazily whenever float use
     * first appears. FPCCR.ASPEN/LSPEN (lazy FP stacking) are left at
     * their reset default of enabled; nothing here needs changing that. */
    ldr   r0, =0xE000ED88       /* SCB->CPACR */
    ldr   r1, [r0]
    ldr   r2, =0x00F00000       /* CP10 (bits 21:20) | CP11 (bits 23:22), both 0b11 */
    orr   r1, r1, r2
    str   r1, [r0]
    dsb
    isb

    /* Set interrupt priority grouping: SCB->AIRCR PRIGROUP = 3, all 4
     * implemented priority bits (core/inc/nvic_reg.h NVIC_PRIO_BITS) used
     * as preemption priority, 0 as subpriority (PM0214 4.4.5's PRIGROUP
     * table). This is the standard STM32F4/FreeRTOS-style grouping and
     * must be settled before rtos/kernel/ starts assigning PendSV/SVCall/
     * SysTick priorities relative to peripheral IRQs - done once, here,
     * rather than left to whichever driver happens to touch AIRCR first.
     * Read-modify-write, not a bare store: AIRCR requires the VECTKEY
     * field rewritten on every write (any other value is ignored), so the
     * current value is read, VECTKEY+PRIGROUP cleared, then both written
     * back together. */
    ldr   r0, =0xE000ED0C       /* SCB->AIRCR */
    ldr   r1, [r0]
    ldr   r2, =0xFFFF0700       /* VECTKEY_Msk (bits 31:16) | PRIGROUP_Msk (bits 10:8) */
    bic   r1, r1, r2
    ldr   r2, =0x05FA0300       /* VECTKEY_WRITE key | PRIGROUP=3 */
    orr   r1, r1, r2
    str   r1, [r0]

    /* Copy .data initializers from flash (_sidata) to RAM ([_sdata, _edata)) */
    movs  r1, #0
    b     LoopCopyDataInit

CopyDataInit:
    ldr   r3, =_sidata
    ldr   r3, [r3, r1]
    str   r3, [r0, r1]
    adds  r1, r1, #4

LoopCopyDataInit:
    ldr   r0, =_sdata
    ldr   r3, =_edata
    adds  r2, r0, r1
    cmp   r2, r3
    bcc   CopyDataInit

    /* Zero-fill .bss ([_sbss, _ebss)) */
    ldr   r2, =_sbss
    b     LoopFillZerobss

FillZerobss:
    movs  r3, #0
    str   r3, [r2], #4

LoopFillZerobss:
    ldr   r3, =_ebss
    cmp   r2, r3
    bcc   FillZerobss

    /* Hand off to C. main() should never return, but bx lr is the
     * defined fallback if it somehow does. */
    bl    main
    bx    lr

.size Reset_Handler, .-Reset_Handler

/* ========================================================================
 * Default_Handler
 * Landing spot for any interrupt that doesn't have a real handler
 * written yet. Infinite loop - safe, and easy to spot in a debugger
 * (PC stuck here means an unexpected/unimplemented interrupt fired).
 * ==================================================================== */
.section .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
    b Infinite_Loop
.size Default_Handler, .-Default_Handler

/* ========================================================================
 * Vector table
 * Must be the very first thing in flash (enforced by the linker script
 * placing .isr_vector first). KEEP() in the linker script stops the
 * linker's dead-code elimination from discarding it, since nothing in
 * C code ever references it directly - the hardware does.
 * ==================================================================== */
.section .isr_vector,"a",%progbits
.type vector_table, %object
.size vector_table, .-vector_table

vector_table:
    .word _estack                       /* initial stack pointer */
    .word Reset_Handler
    .word NMI_Handler
    .word HardFault_Handler
    .word MemManage_Handler
    .word BusFault_Handler
    .word UsageFault_Handler
    .word 0                             /* reserved */
    .word 0
    .word 0
    .word 0
    .word SVC_Handler
    .word DebugMon_Handler
    .word 0                             /* reserved */
    .word PendSV_Handler
    .word SysTick_Handler

    /* STM32F446-specific peripheral interrupts, IRQ 0.. in order */
    .word WWDG_IRQHandler
    .word PVD_IRQHandler
    .word TAMP_STAMP_IRQHandler
    .word RTC_WKUP_IRQHandler
    .word FLASH_IRQHandler
    .word RCC_IRQHandler
    .word EXTI0_IRQHandler
    .word EXTI1_IRQHandler
    .word EXTI2_IRQHandler
    .word EXTI3_IRQHandler
    .word EXTI4_IRQHandler
    .word DMA1_Stream0_IRQHandler
    .word DMA1_Stream1_IRQHandler
    .word DMA1_Stream2_IRQHandler
    .word DMA1_Stream3_IRQHandler
    .word DMA1_Stream4_IRQHandler
    .word DMA1_Stream5_IRQHandler
    .word DMA1_Stream6_IRQHandler
    .word ADC_IRQHandler
    .word CAN1_TX_IRQHandler
    .word CAN1_RX0_IRQHandler
    .word CAN1_RX1_IRQHandler
    .word CAN1_SCE_IRQHandler
    .word EXTI9_5_IRQHandler
    .word TIM1_BRK_TIM9_IRQHandler
    .word TIM1_UP_TIM10_IRQHandler
    .word TIM1_TRG_COM_TIM11_IRQHandler
    .word TIM1_CC_IRQHandler
    .word TIM2_IRQHandler
    .word TIM3_IRQHandler
    .word TIM4_IRQHandler
    .word I2C1_EV_IRQHandler
    .word I2C1_ER_IRQHandler
    .word I2C2_EV_IRQHandler
    .word I2C2_ER_IRQHandler
    .word SPI1_IRQHandler
    .word SPI2_IRQHandler
    .word USART1_IRQHandler
    .word USART2_IRQHandler
    .word USART3_IRQHandler
    .word EXTI15_10_IRQHandler
    .word RTC_Alarm_IRQHandler
    .word OTG_FS_WKUP_IRQHandler
    .word TIM8_BRK_TIM12_IRQHandler
    .word TIM8_UP_TIM13_IRQHandler
    .word TIM8_TRG_COM_TIM14_IRQHandler
    .word TIM8_CC_IRQHandler
    .word DMA1_Stream7_IRQHandler
    .word FMC_IRQHandler
    .word SDIO_IRQHandler
    .word TIM5_IRQHandler
    .word SPI3_IRQHandler
    .word UART4_IRQHandler
    .word UART5_IRQHandler
    .word TIM6_DAC_IRQHandler
    .word TIM7_IRQHandler
    .word DMA2_Stream0_IRQHandler
    .word DMA2_Stream1_IRQHandler
    .word DMA2_Stream2_IRQHandler
    .word DMA2_Stream3_IRQHandler
    .word DMA2_Stream4_IRQHandler
    .word 0                             /* reserved */
    .word 0                             /* reserved */
    .word CAN2_TX_IRQHandler
    .word CAN2_RX0_IRQHandler
    .word CAN2_RX1_IRQHandler
    .word CAN2_SCE_IRQHandler
    .word OTG_FS_IRQHandler
    .word DMA2_Stream5_IRQHandler
    .word DMA2_Stream6_IRQHandler
    .word DMA2_Stream7_IRQHandler
    .word USART6_IRQHandler
    .word I2C3_EV_IRQHandler
    .word I2C3_ER_IRQHandler
    .word OTG_HS_EP1_OUT_IRQHandler
    .word OTG_HS_EP1_IN_IRQHandler
    .word OTG_HS_WKUP_IRQHandler
    .word OTG_HS_IRQHandler
    .word DCMI_IRQHandler
    .word 0                             /* reserved */
    .word 0                             /* reserved */
    .word FPU_IRQHandler
    .word 0                             /* reserved */
    .word 0                             /* reserved */
    .word SPI4_IRQHandler
    .word 0                             /* reserved */
    .word 0                             /* reserved */
    .word SAI1_IRQHandler
    .word 0                             /* reserved */
    .word 0                             /* reserved */
    .word 0                             /* reserved */
    .word SAI2_IRQHandler
    .word QUADSPI_IRQHandler
    .word CEC_IRQHandler
    .word SPDIF_RX_IRQHandler
    .word FMPI2C1_Event_IRQHandler
    .word FMPI2C1_Error_IRQHandler

/* ========================================================================
 * Weak aliases - every handler defaults to Default_Handler unless you
 * write a real one later (e.g. in drivers/src/uart.c you'll eventually
 * define USART2_IRQHandler, and the linker uses that strong definition
 * instead of this weak one - no changes needed here when that happens).
 * ==================================================================== */
.weak NMI_Handler
.thumb_set NMI_Handler,Default_Handler
.weak HardFault_Handler
.thumb_set HardFault_Handler,Default_Handler
.weak MemManage_Handler
.thumb_set MemManage_Handler,Default_Handler
.weak BusFault_Handler
.thumb_set BusFault_Handler,Default_Handler
.weak UsageFault_Handler
.thumb_set UsageFault_Handler,Default_Handler
.weak SVC_Handler
.thumb_set SVC_Handler,Default_Handler
.weak DebugMon_Handler
.thumb_set DebugMon_Handler,Default_Handler
.weak PendSV_Handler
.thumb_set PendSV_Handler,Default_Handler
.weak SysTick_Handler
.thumb_set SysTick_Handler,Default_Handler

.weak WWDG_IRQHandler
.thumb_set WWDG_IRQHandler,Default_Handler
.weak PVD_IRQHandler
.thumb_set PVD_IRQHandler,Default_Handler
.weak TAMP_STAMP_IRQHandler
.thumb_set TAMP_STAMP_IRQHandler,Default_Handler
.weak RTC_WKUP_IRQHandler
.thumb_set RTC_WKUP_IRQHandler,Default_Handler
.weak FLASH_IRQHandler
.thumb_set FLASH_IRQHandler,Default_Handler
.weak RCC_IRQHandler
.thumb_set RCC_IRQHandler,Default_Handler
.weak EXTI0_IRQHandler
.thumb_set EXTI0_IRQHandler,Default_Handler
.weak EXTI1_IRQHandler
.thumb_set EXTI1_IRQHandler,Default_Handler
.weak EXTI2_IRQHandler
.thumb_set EXTI2_IRQHandler,Default_Handler
.weak EXTI3_IRQHandler
.thumb_set EXTI3_IRQHandler,Default_Handler
.weak EXTI4_IRQHandler
.thumb_set EXTI4_IRQHandler,Default_Handler
.weak DMA1_Stream0_IRQHandler
.thumb_set DMA1_Stream0_IRQHandler,Default_Handler
.weak DMA1_Stream1_IRQHandler
.thumb_set DMA1_Stream1_IRQHandler,Default_Handler
.weak DMA1_Stream2_IRQHandler
.thumb_set DMA1_Stream2_IRQHandler,Default_Handler
.weak DMA1_Stream3_IRQHandler
.thumb_set DMA1_Stream3_IRQHandler,Default_Handler
.weak DMA1_Stream4_IRQHandler
.thumb_set DMA1_Stream4_IRQHandler,Default_Handler
.weak DMA1_Stream5_IRQHandler
.thumb_set DMA1_Stream5_IRQHandler,Default_Handler
.weak DMA1_Stream6_IRQHandler
.thumb_set DMA1_Stream6_IRQHandler,Default_Handler
.weak ADC_IRQHandler
.thumb_set ADC_IRQHandler,Default_Handler
.weak CAN1_TX_IRQHandler
.thumb_set CAN1_TX_IRQHandler,Default_Handler
.weak CAN1_RX0_IRQHandler
.thumb_set CAN1_RX0_IRQHandler,Default_Handler
.weak CAN1_RX1_IRQHandler
.thumb_set CAN1_RX1_IRQHandler,Default_Handler
.weak CAN1_SCE_IRQHandler
.thumb_set CAN1_SCE_IRQHandler,Default_Handler
.weak EXTI9_5_IRQHandler
.thumb_set EXTI9_5_IRQHandler,Default_Handler
.weak TIM1_BRK_TIM9_IRQHandler
.thumb_set TIM1_BRK_TIM9_IRQHandler,Default_Handler
.weak TIM1_UP_TIM10_IRQHandler
.thumb_set TIM1_UP_TIM10_IRQHandler,Default_Handler
.weak TIM1_TRG_COM_TIM11_IRQHandler
.thumb_set TIM1_TRG_COM_TIM11_IRQHandler,Default_Handler
.weak TIM1_CC_IRQHandler
.thumb_set TIM1_CC_IRQHandler,Default_Handler
.weak TIM2_IRQHandler
.thumb_set TIM2_IRQHandler,Default_Handler
.weak TIM3_IRQHandler
.thumb_set TIM3_IRQHandler,Default_Handler
.weak TIM4_IRQHandler
.thumb_set TIM4_IRQHandler,Default_Handler
.weak I2C1_EV_IRQHandler
.thumb_set I2C1_EV_IRQHandler,Default_Handler
.weak I2C1_ER_IRQHandler
.thumb_set I2C1_ER_IRQHandler,Default_Handler
.weak I2C2_EV_IRQHandler
.thumb_set I2C2_EV_IRQHandler,Default_Handler
.weak I2C2_ER_IRQHandler
.thumb_set I2C2_ER_IRQHandler,Default_Handler
.weak SPI1_IRQHandler
.thumb_set SPI1_IRQHandler,Default_Handler
.weak SPI2_IRQHandler
.thumb_set SPI2_IRQHandler,Default_Handler
.weak USART1_IRQHandler
.thumb_set USART1_IRQHandler,Default_Handler
.weak USART2_IRQHandler
.thumb_set USART2_IRQHandler,Default_Handler
.weak USART3_IRQHandler
.thumb_set USART3_IRQHandler,Default_Handler
.weak EXTI15_10_IRQHandler
.thumb_set EXTI15_10_IRQHandler,Default_Handler
.weak RTC_Alarm_IRQHandler
.thumb_set RTC_Alarm_IRQHandler,Default_Handler
.weak OTG_FS_WKUP_IRQHandler
.thumb_set OTG_FS_WKUP_IRQHandler,Default_Handler
.weak TIM8_BRK_TIM12_IRQHandler
.thumb_set TIM8_BRK_TIM12_IRQHandler,Default_Handler
.weak TIM8_UP_TIM13_IRQHandler
.thumb_set TIM8_UP_TIM13_IRQHandler,Default_Handler
.weak TIM8_TRG_COM_TIM14_IRQHandler
.thumb_set TIM8_TRG_COM_TIM14_IRQHandler,Default_Handler
.weak TIM8_CC_IRQHandler
.thumb_set TIM8_CC_IRQHandler,Default_Handler
.weak DMA1_Stream7_IRQHandler
.thumb_set DMA1_Stream7_IRQHandler,Default_Handler
.weak FMC_IRQHandler
.thumb_set FMC_IRQHandler,Default_Handler
.weak SDIO_IRQHandler
.thumb_set SDIO_IRQHandler,Default_Handler
.weak TIM5_IRQHandler
.thumb_set TIM5_IRQHandler,Default_Handler
.weak SPI3_IRQHandler
.thumb_set SPI3_IRQHandler,Default_Handler
.weak UART4_IRQHandler
.thumb_set UART4_IRQHandler,Default_Handler
.weak UART5_IRQHandler
.thumb_set UART5_IRQHandler,Default_Handler
.weak TIM6_DAC_IRQHandler
.thumb_set TIM6_DAC_IRQHandler,Default_Handler
.weak TIM7_IRQHandler
.thumb_set TIM7_IRQHandler,Default_Handler
.weak DMA2_Stream0_IRQHandler
.thumb_set DMA2_Stream0_IRQHandler,Default_Handler
.weak DMA2_Stream1_IRQHandler
.thumb_set DMA2_Stream1_IRQHandler,Default_Handler
.weak DMA2_Stream2_IRQHandler
.thumb_set DMA2_Stream2_IRQHandler,Default_Handler
.weak DMA2_Stream3_IRQHandler
.thumb_set DMA2_Stream3_IRQHandler,Default_Handler
.weak DMA2_Stream4_IRQHandler
.thumb_set DMA2_Stream4_IRQHandler,Default_Handler
.weak CAN2_TX_IRQHandler
.thumb_set CAN2_TX_IRQHandler,Default_Handler
.weak CAN2_RX0_IRQHandler
.thumb_set CAN2_RX0_IRQHandler,Default_Handler
.weak CAN2_RX1_IRQHandler
.thumb_set CAN2_RX1_IRQHandler,Default_Handler
.weak CAN2_SCE_IRQHandler
.thumb_set CAN2_SCE_IRQHandler,Default_Handler
.weak OTG_FS_IRQHandler
.thumb_set OTG_FS_IRQHandler,Default_Handler
.weak DMA2_Stream5_IRQHandler
.thumb_set DMA2_Stream5_IRQHandler,Default_Handler
.weak DMA2_Stream6_IRQHandler
.thumb_set DMA2_Stream6_IRQHandler,Default_Handler
.weak DMA2_Stream7_IRQHandler
.thumb_set DMA2_Stream7_IRQHandler,Default_Handler
.weak USART6_IRQHandler
.thumb_set USART6_IRQHandler,Default_Handler
.weak I2C3_EV_IRQHandler
.thumb_set I2C3_EV_IRQHandler,Default_Handler
.weak I2C3_ER_IRQHandler
.thumb_set I2C3_ER_IRQHandler,Default_Handler
.weak OTG_HS_EP1_OUT_IRQHandler
.thumb_set OTG_HS_EP1_OUT_IRQHandler,Default_Handler
.weak OTG_HS_EP1_IN_IRQHandler
.thumb_set OTG_HS_EP1_IN_IRQHandler,Default_Handler
.weak OTG_HS_WKUP_IRQHandler
.thumb_set OTG_HS_WKUP_IRQHandler,Default_Handler
.weak OTG_HS_IRQHandler
.thumb_set OTG_HS_IRQHandler,Default_Handler
.weak DCMI_IRQHandler
.thumb_set DCMI_IRQHandler,Default_Handler
.weak FPU_IRQHandler
.thumb_set FPU_IRQHandler,Default_Handler
.weak SPI4_IRQHandler
.thumb_set SPI4_IRQHandler,Default_Handler
.weak SAI1_IRQHandler
.thumb_set SAI1_IRQHandler,Default_Handler
.weak SAI2_IRQHandler
.thumb_set SAI2_IRQHandler,Default_Handler
.weak QUADSPI_IRQHandler
.thumb_set QUADSPI_IRQHandler,Default_Handler
.weak CEC_IRQHandler
.thumb_set CEC_IRQHandler,Default_Handler
.weak SPDIF_RX_IRQHandler
.thumb_set SPDIF_RX_IRQHandler,Default_Handler
.weak FMPI2C1_Event_IRQHandler
.thumb_set FMPI2C1_Event_IRQHandler,Default_Handler
.weak FMPI2C1_Error_IRQHandler
.thumb_set FMPI2C1_Error_IRQHandler,Default_Handler
