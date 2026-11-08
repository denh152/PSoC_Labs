/* ========================================
 * Lab 1, task 2 - push button interrupt (variant 5)
 *
 * Board: CY8CKIT-042 (CY8C4245AXI-483)
 * The timer interrupt from task 1 is kept (Pin_ISR toggles every 100 us).
 * The user button SW2 (P0[7]) generates an interrupt on the falling edge;
 * its handler turns the LED on Pin_Main (P0[3], blue) on/off.
 * ========================================
*/
#include "project.h"

/* 100 us / 0.25 us = 400 ticks; the TCPWM counts 0..Period, so Period = 399 */
#define TIMER_PERIOD_TICKS   (400u - 1u)

volatile uint32_t isr_timer_count = 0u;   /* number of timer interrupts  */
volatile uint32_t button_count    = 0u;   /* number of button presses    */

/* Timer_1 Terminal Count interrupt handler: toggles Pin_ISR (P0[2]) */
CY_ISR(Timer_ISR_Handler)
{
    Pin_ISR_Write((uint8)(!Pin_ISR_Read()));

    isr_timer_count++;

    Timer_1_ClearInterrupt(Timer_1_INTR_MASK_TC);
}

/* Button (P0[7], SW2) interrupt handler: toggles the LED on Pin_Main (P0[3]) */
CY_ISR(Button_Main_Handler)
{
    uint8 current_state = Pin_Main_Read();
    Pin_Main_Write((uint8)(!current_state));

    button_count++;

    Button_ClearInterrupt();                         /* Clear the pin interrupt flag */
}

int main(void)
{
    CyGlobalIntEnable;                               /* Enable global interrupts */

    Clock_1_Start();
    Timer_1_Start();
    Timer_1_WritePeriod(TIMER_PERIOD_TICKS);
    Timer_1_SetInterruptMode(Timer_1_INTR_MASK_TC);

    isr_Timer_StartEx(Timer_ISR_Handler);            /* Timer interrupt (task 1) */
    isr_Button_StartEx(Button_Main_Handler);         /* Button interrupt (task 2) */

    for (;;)
    {
        /* Nothing to do here: Pin_Main is driven only by the button ISR */
    }
}

/* [] END OF FILE */
