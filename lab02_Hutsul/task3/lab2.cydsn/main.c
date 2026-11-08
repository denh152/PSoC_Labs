#include "project.h"
#include <string.h>

#define LINE_MAX_LEN   (48u)
#define MAX_TOKENS     (4u)
#define LED_COUNT      (3u)
#define LED_ON_LEVEL   (0u)   /* the on-board LEDs are active low */
#define LED_ALL        (0xFFu)

#define BLINK_MIN_MS   (10u)
#define BLINK_MAX_MS   (60000u)

typedef uint8 (*CommandHandler)(uint8 argc, char *argv[]);

typedef struct
{
    const char    *name;
    const char    *help;
    CommandHandler handler;
} Command;

static volatile uint32 uptimeMs = 0u;

static char  lineBuf[LINE_MAX_LEN];
static uint8 lineLen = 0u;
static uint8 lastWasCr = 0u;

static uint8 ledOn[LED_COUNT];

/* Non-blocking blinking: half-period and the moment of the next toggle for every LED */
static uint32 blinkIntervalMs[LED_COUNT];
static uint32 blinkNextMs[LED_COUNT];

static uint32 rxByteCount = 0u;
static uint32 cmdOkCount = 0u;
static uint32 cmdErrCount = 0u;
static uint32 lineOverflowCount = 0u;

static void SysTickTick(void)
{
    uptimeMs++;
}

static void Print(const char *text)
{
    UART_1_UartPutString(text);
}

static void PrintU32(uint32 value)
{
    char  digits[11];
    uint8 pos = (uint8)sizeof(digits);

    digits[--pos] = '\0';
    do
    {
        digits[--pos] = (char)('0' + (value % 10u));
        value /= 10u;
    } while (value != 0u);

    Print(&digits[pos]);
}

/* Decimal number parser: 1 - success, 0 - error (not a digit or overflow). */
static uint8 ParseU32(const char *text, uint32 *result)
{
    uint32 value = 0u;
    uint32 digit;

    if (*text == '\0')
    {
        return 0u;
    }

    while (*text != '\0')
    {
        if ((*text < '0') || (*text > '9'))
        {
            return 0u;
        }
        digit = (uint32)(*text - '0');
        if (value > ((0xFFFFFFFFu - digit) / 10u))
        {
            return 0u;
        }
        value = (value * 10u) + digit;
        text++;
    }

    *result = value;
    return 1u;
}

/* Prints the usage message and returns 0 ("command failed"). */
static uint8 ErrorUsage(const char *usage)
{
    Print("ERR usage: ");
    Print(usage);
    Print("\r\n");
    return 0u;
}

/* Drives the LED pin: index 0 = R, 1 = G, 2 = B. */
static void LedWriteHw(uint8 index, uint8 on)
{
    uint8 level = on ? LED_ON_LEVEL : (uint8)(1u - LED_ON_LEVEL);

    switch (index)
    {
        case 0u: Pin_LED_R_Write(level); break;
        case 1u: Pin_LED_G_Write(level); break;
        default: Pin_LED_B_Write(level); break;
    }
}

/* Converts "r", "g", "b" to 0, 1, 2 and "all" to LED_ALL (if allowed). Returns 1 on success. */
static uint8 ParseLedName(const char *text, uint8 allowAll, uint8 *index)
{
    if (strcmp(text, "r") == 0)
    {
        *index = 0u;
    }
    else if (strcmp(text, "g") == 0)
    {
        *index = 1u;
    }
    else if (strcmp(text, "b") == 0)
    {
        *index = 2u;
    }
    else if (allowAll && (strcmp(text, "all") == 0))
    {
        *index = LED_ALL;
    }
    else
    {
        return 0u;
    }
    return 1u;
}

/* Sets the LED state, stores it in ledOn[] and cancels blinking of that LED. */
static void LedSet(uint8 index, uint8 on)
{
    ledOn[index] = on;
    blinkIntervalMs[index] = 0u;
    LedWriteHw(index, on);
}

/* ---------- Built-in commands ---------- */

static uint8 CmdHelp(uint8 argc, char *argv[]);

static uint8 CmdPing(uint8 argc, char *argv[])
{
    (void)argc; (void)argv;
    Print("pong\r\n");
    return 1u;
}

static uint8 CmdInfo(uint8 argc, char *argv[])
{
    (void)argc; (void)argv;
    Print("Lab_2 UART console\r\nSYSCLK: ");
    PrintU32((uint32)CYDEV_BCLK__SYSCLK__MHZ);
    Print(" MHz\r\nSysTick: 1 ms\r\n");
    return 1u;
}

static uint8 CmdUptime(uint8 argc, char *argv[])
{
    (void)argc; (void)argv;
    PrintU32(uptimeMs);
    Print(" ms\r\n");
    return 1u;
}

static uint8 CmdReboot(uint8 argc, char *argv[])
{
    (void)argc; (void)argv;
    Print("rebooting...\r\n");
    CyDelay(20u);   /* let the text go out before the reset */
    CySoftwareReset();
    return 1u;
}

/* ---------- Commands added in step 6.3 ---------- */

/* echo <text...> : sends the received tokens back, separated by spaces */
static uint8 CmdEcho(uint8 argc, char *argv[])
{
    uint8 i;

    for (i = 1u; i < argc; i++)
    {
        if (i > 1u)
        {
            Print(" ");
        }
        Print(argv[i]);
    }
    Print("\r\n");
    return 1u;
}

/* led <r|g|b|all> <on|off|toggle> */
static uint8 CmdLed(uint8 argc, char *argv[])
{
    static const char usage[] = "led <r|g|b|all> <on|off|toggle>";
    uint8 index;
    uint8 i;
    uint8 first;
    uint8 last;

    if (argc != 3u)
    {
        return ErrorUsage(usage);
    }
    if (!ParseLedName(argv[1], 1u, &index))
    {
        return ErrorUsage(usage);
    }

    if (index == LED_ALL)
    {
        first = 0u;
        last = LED_COUNT - 1u;
    }
    else
    {
        first = index;
        last = index;
    }

    for (i = first; i <= last; i++)
    {
        if (strcmp(argv[2], "on") == 0)
        {
            LedSet(i, 1u);
        }
        else if (strcmp(argv[2], "off") == 0)
        {
            LedSet(i, 0u);
        }
        else if (strcmp(argv[2], "toggle") == 0)
        {
            LedSet(i, (uint8)(ledOn[i] ? 0u : 1u));
        }
        else
        {
            return ErrorUsage(usage);
        }
    }

    Print("OK\r\n");
    return 1u;
}

/* stats : prints the counters, one per line */
static uint8 CmdStats(uint8 argc, char *argv[])
{
    (void)argc; (void)argv;
    Print("rx bytes: ");      PrintU32(rxByteCount);        Print("\r\n");
    Print("cmd ok: ");        PrintU32(cmdOkCount);         Print("\r\n");
    Print("cmd err: ");       PrintU32(cmdErrCount);        Print("\r\n");
    Print("line overflow: "); PrintU32(lineOverflowCount);  Print("\r\n");
    return 1u;
}

/* blink <r|g|b> <ms|off> : non-blocking blinking with a half-period of 10..60000 ms */
static uint8 CmdBlink(uint8 argc, char *argv[])
{
    static const char usage[] = "blink <r|g|b> <10..60000 ms|off>";
    uint8  index;
    uint32 ms;

    if (argc != 3u)
    {
        return ErrorUsage(usage);
    }
    if (!ParseLedName(argv[1], 0u, &index))
    {
        return ErrorUsage(usage);
    }

    if (strcmp(argv[2], "off") == 0)
    {
        LedSet(index, 0u);          /* stop blinking and switch the LED off */
    }
    else
    {
        if (!ParseU32(argv[2], &ms) || (ms < BLINK_MIN_MS) || (ms > BLINK_MAX_MS))
        {
            return ErrorUsage(usage);
        }
        blinkIntervalMs[index] = ms;
        blinkNextMs[index] = uptimeMs + ms;
    }

    Print("OK\r\n");
    return 1u;
}

/* Called from the main loop: toggles every blinking LED when its deadline has passed. */
static void BlinkService(void)
{
    uint8 i;

    for (i = 0u; i < LED_COUNT; i++)
    {
        if (blinkIntervalMs[i] != 0u)
        {
            if ((int32)(uptimeMs - blinkNextMs[i]) >= 0)
            {
                ledOn[i] = (uint8)(ledOn[i] ? 0u : 1u);
                LedWriteHw(i, ledOn[i]);
                blinkNextMs[i] += blinkIntervalMs[i];
            }
        }
    }
}

/* ---------- Variant 5 command ---------- */

/* bin <n> : prints the number in binary without leading zeros, e.g. bin 10 -> 1010 */
static uint8 CmdBin(uint8 argc, char *argv[])
{
    static const char usage[] = "bin <n>";
    uint32 value;
    char   bits[33];
    uint8  pos = (uint8)sizeof(bits);

    if (argc != 2u)
    {
        return ErrorUsage(usage);
    }
    if (!ParseU32(argv[1], &value))
    {
        return ErrorUsage(usage);
    }

    bits[--pos] = '\0';
    do
    {
        bits[--pos] = (char)('0' + (value & 1u));
        value >>= 1u;
    } while (value != 0u);

    Print(&bits[pos]);
    Print("\r\n");
    return 1u;
}

static const Command commandTable[] =
{
    { "help",   "list commands",                       CmdHelp   },
    { "ping",   "reply pong",                          CmdPing   },
    { "info",   "clock and build info",                CmdInfo   },
    { "uptime", "time since reset, ms",                CmdUptime },
    { "reboot", "software reset",                      CmdReboot },
    { "echo",   "echo <text...>",                      CmdEcho   },
    { "led",    "led <r|g|b|all> <on|off|toggle>",     CmdLed    },
    { "stats",  "rx/cmd/overflow counters",            CmdStats  },
    { "blink",  "blink <r|g|b> <10..60000 ms|off>",    CmdBlink  },
    { "bin",    "bin <n> - number in binary",          CmdBin    },
};

#define COMMAND_COUNT (sizeof(commandTable) / sizeof(commandTable[0]))

static uint8 CmdHelp(uint8 argc, char *argv[])
{
    uint8 i;

    (void)argc; (void)argv;
    for (i = 0u; i < COMMAND_COUNT; i++)
    {
        Print("  ");
        Print(commandTable[i].name);
        Print(" - ");
        Print(commandTable[i].help);
        Print("\r\n");
    }
    return 1u;
}

/* ---------- Command line ---------- */

/* Splits lineBuf into tokens (spaces become '\0') and calls the handler. */
static void ExecuteLine(void)
{
    char  *argv[MAX_TOKENS];
    uint8  argc = 0u;
    uint8  inToken = 0u;
    uint8  i;
    uint8  ok = 0u;
    uint8  found = 0u;
    char  *p = lineBuf;

    for (; *p != '\0'; p++)
    {
        if (*p == ' ')
        {
            *p = '\0';
            inToken = 0u;
        }
        else if (!inToken)
        {
            if (argc == MAX_TOKENS)
            {
                Print("ERR too many arguments\r\n");
                cmdErrCount++;
                return;
            }
            argv[argc++] = p;
            inToken = 1u;
        }
    }

    if (argc == 0u)
    {
        return;
    }

    for (i = 0u; i < COMMAND_COUNT; i++)
    {
        if (strcmp(argv[0], commandTable[i].name) == 0)
        {
            found = 1u;
            ok = commandTable[i].handler(argc, argv);
            break;
        }
    }

    if (!found)
    {
        Print("ERR unknown command, type help\r\n");
    }

    if (found && ok)
    {
        cmdOkCount++;
    }
    else
    {
        cmdErrCount++;
    }
}

/* Handles one received byte: echo, Backspace, Enter, line overflow. */
static void ProcessChar(uint8 c)
{
    rxByteCount++;

    if ((c == '\n') && lastWasCr)
    {
        lastWasCr = 0u;   /* second half of CR+LF */
        return;
    }

    if ((c == '\r') || (c == '\n'))
    {
        lastWasCr = (uint8)(c == '\r');
        Print("\r\n");
        lineBuf[lineLen] = '\0';
        ExecuteLine();
        lineLen = 0u;
        Print("> ");
        return;
    }

    lastWasCr = 0u;

    if ((c == 0x08u) || (c == 0x7Fu))
    {
        if (lineLen > 0u)
        {
            lineLen--;
            Print("\b \b");
        }
    }
    else if ((c >= 0x20u) && (c < 0x7Fu))
    {
        if (lineLen < (LINE_MAX_LEN - 1u))
        {
            lineBuf[lineLen++] = (char)c;
            UART_1_UartPutChar(c);   /* echo from the board side */
        }
        else
        {
            lineOverflowCount++;
        }
    }
}

int main(void)
{
    uint8 i;

    CyGlobalIntEnable;

    CySysTickStart();
    (void)CySysTickSetCallback(0u, SysTickTick);

    for (i = 0u; i < LED_COUNT; i++)
    {
        ledOn[i] = 0u;
        blinkIntervalMs[i] = 0u;
        blinkNextMs[i] = 0u;
        LedWriteHw(i, 0u);
    }

    UART_1_Start();

    Print("\r\n=== Lab_2 UART console ===\r\nType help\r\n> ");

    for (;;)
    {
        while (UART_1_SpiUartGetRxBufferSize() > 0u)
        {
            ProcessChar((uint8)UART_1_SpiUartReadRxData());
        }

        BlinkService();
    }
}

/* [] END OF FILE */
