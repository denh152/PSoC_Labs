/* ========================================
 * Lab 1, task 1 - clock tree and timer interrupt (variant 5)
 *
 * Board: CY8CKIT-042 (CY8C4245AXI-483)
 * IMO = 48 MHz, SYSCLK (BUS CLK) = 24 MHz, Clock_1 = 4 MHz (tick = 0.25 us)
 * Timer_1 interrupt period = 100 us, main loop delay = 1 ms
 * ========================================
*/
#include "project.h"

/* 100 us / 0.25 us = 400 ticks. The TCPWM counts from 0 to Period inclusive,
 * so the Terminal Count event fires every (Period + 1) ticks -> Period = 399. */
#define TIMER_PERIOD_TICKS   (400u - 1u)

/* Main loop delay according to the variant */
#define MAIN_DELAY_MS        (1u)

volatile uint32_t isr_timer_count = 0u;   /* number of timer interrupts */

/* Timer_1 Terminal Count interrupt handler: toggles Pin_ISR (P0[2]) */
CY_ISR(Timer_ISR_Handler)
{
    Pin_ISR_Write((uint8)(!Pin_ISR_Read()));

    isr_timer_count++;

    /* Mandatory: clear the TC flag, otherwise the interrupt fires only once */
    Timer_1_ClearInterrupt(Timer_1_INTR_MASK_TC);
}

int main(void)
{
    CyGlobalIntEnable;                               /* Enable global interrupts */

    Clock_1_Start();                                 /* 4 MHz clock for the timer */
    Timer_1_Start();                                 /* Init and start the TCPWM */
    Timer_1_WritePeriod(TIMER_PERIOD_TICKS);         /* 399 -> 400 ticks = 100 us */
    Timer_1_SetInterruptMode(Timer_1_INTR_MASK_TC);  /* Interrupt on Terminal Count */
    isr_Timer_StartEx(Timer_ISR_Handler);            /* Register the handler, enable NVIC */

    for (;;)
    {
        /* "Slow" main loop: Pin_Main (P0[3]) toggles every 1 ms -> 2 ms period.
         * It runs independently of the timer interrupt. */
        Pin_Main_Write(1u);
        CyDelay(MAIN_DELAY_MS);
        Pin_Main_Write(0u);
        CyDelay(MAIN_DELAY_MS);
    }
}

/* [] END OF FILE */
