#include "project.h"

/* Step 5: minimal program that checks the whole path PC -> KitProg -> PSoC -> PC.
 * Every byte received over UART is sent back (echo). */
int main(void)
{
    CyGlobalIntEnable;
    UART_1_Start();
    UART_1_UartPutString("\r\n=== Lab_2 UART ===\r\n");

    for (;;)
    {
        if (UART_1_SpiUartGetRxBufferSize() > 0u)
        {
            UART_1_UartPutChar(UART_1_SpiUartReadRxData());
        }
    }
}

/* [] END OF FILE */
