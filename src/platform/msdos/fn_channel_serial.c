/*
 * fn_channel_serial.c - MS-DOS COM-port byte channel.
 *
 * This file owns 8250-compatible UART access only. The common stream transport
 * owns SLIP framing and FujiBus request/response exchange.
 */

#include <conio.h>
#include <dos.h>

#include "fujinet-nio.h"
#include "fn_channel.h"
#include "fn_msdos.h"

#ifndef FN_MSDOS_COM
#define FN_MSDOS_COM 1
#endif

#ifndef FN_MSDOS_BAUD_DIVISOR
#define FN_MSDOS_BAUD_DIVISOR 1
#endif

enum {
    UART_RBR = 0,
    UART_THR = 0,
    UART_IER = 1,
    UART_FCR = 2,
    UART_LCR = 3,
    UART_MCR = 4,
    UART_LSR = 5,

    LSR_DR = 0x01,
    LSR_THRE = 0x20,

    LCR_DLAB = 0x80,
    LCR_8N1 = 0x03,

    MCR_DTR = 0x01,
    MCR_RTS = 0x02,
    MCR_OUT2 = 0x08
};

static uint16_t _uart_base;
static uint8_t _com_number = FN_MSDOS_COM;
static uint8_t _initialized;

static uint16_t com_base(uint8_t com_number)
{
    switch (com_number) {
    case 1:
        return 0x03F8;
    case 2:
        return 0x02F8;
    case 3:
        return 0x03E8;
    case 4:
        return 0x02E8;
    default:
        return 0;
    }
}

static unsigned long bios_ticks(void)
{
    volatile unsigned long far *ticks;

    ticks = (volatile unsigned long far *)MK_FP(0x40, 0x6C);
    return *ticks;
}

static unsigned long timeout_to_ticks(uint16_t timeout_ms)
{
    unsigned long ticks;

    ticks = ((unsigned long)timeout_ms * 182UL) / 10000UL;
    return ticks ? ticks : 1UL;
}

static void uart_init(uint16_t base, uint16_t divisor)
{
    _uart_base = base;

    outp(_uart_base + UART_IER, 0x00);
    outp(_uart_base + UART_LCR, LCR_DLAB);
    outp(_uart_base + 0, divisor & 0xFF);
    outp(_uart_base + 1, (divisor >> 8) & 0xFF);
    outp(_uart_base + UART_LCR, LCR_8N1);
    outp(_uart_base + UART_FCR, 0x07);
    outp(_uart_base + UART_MCR, MCR_DTR | MCR_RTS | MCR_OUT2);
}

uint8_t fn_channel_init(void)
{
    uint16_t base;

    if (_initialized) {
        return FN_OK;
    }

    base = com_base(_com_number);
    if (base == 0) {
        return FN_ERR_INVALID;
    }

    uart_init(base, FN_MSDOS_BAUD_DIVISOR);
    fn_channel_drain_rx();
    _initialized = 1;

    return FN_OK;
}

void fn_msdos_serial_set_com(uint8_t com_number)
{
    if (!_initialized) {
        _com_number = com_number;
    }
}

uint8_t fn_channel_ready(void)
{
    return _initialized;
}

uint8_t fn_channel_read_byte(uint8_t *value, uint16_t timeout_ms)
{
    unsigned long start;
    unsigned long limit;

    start = bios_ticks();
    limit = timeout_to_ticks(timeout_ms);

    while ((bios_ticks() - start) <= limit) {
        if (inp(_uart_base + UART_LSR) & LSR_DR) {
            *value = (uint8_t)inp(_uart_base + UART_RBR);
            return 1;
        }
    }

    return 0;
}

uint8_t fn_channel_write_byte(uint8_t value, uint16_t timeout_ms)
{
    unsigned long start;
    unsigned long limit;

    start = bios_ticks();
    limit = timeout_to_ticks(timeout_ms);

    while ((bios_ticks() - start) <= limit) {
        if (inp(_uart_base + UART_LSR) & LSR_THRE) {
            outp(_uart_base + UART_THR, value);
            return FN_OK;
        }
    }

    return FN_ERR_TIMEOUT;
}

void fn_channel_drain_rx(void)
{
    uint8_t ignored;

    while (fn_channel_read_byte(&ignored, 1)) {
    }
}

void fn_channel_close(void)
{
    _initialized = 0;
}

const char *fn_platform_name(void)
{
    return "msdos";
}
