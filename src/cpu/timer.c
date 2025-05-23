#include "port.h"
#include "terminal.h"
#include "idt.h"

extern void timer_handler_stub();

static int tick_count = 0;

#define IDT_FLAG_PRESENT 0x80
#define IDT_FLAG_RING0   0x00
#define IDT_FLAG_INT_GATE 0x0E

void timer_handler() {
    /* tick */
    tick_count++;
    if (tick_count >= 9) {
        /* for terminal cursor */
        terminal_toggle_cursor();
        tick_count = 0;
    }

    /* end of interrupt */
    outb(0x20, 0x20);
}

void timer_init() {
    uint16_t divisor = 65536 / 18;

    /* timer entry */
    idt_set_entry(32, (uint64_t)timer_handler_stub, 0x08, IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE);

    outb(0x43, 0x36);
    outb(0x40, divisor & 0xFF);
    outb(0x40, divisor >> 8);
}
