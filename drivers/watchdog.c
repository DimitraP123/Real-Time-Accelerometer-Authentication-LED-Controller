#include "watchdog.h"
#include <rp2350/watchdog.h>
#include <rp2350/psm.h>
#include <stdint.h>

static uint32_t reload_value;

void configure_watchdog(uint32_t reload_ms)
{
    reload_value = reload_ms;
    PSM_WDSEL = 0xFFFFFFFF;   
    WATCHDOG_LOAD = reload_value;
    WATCHDOG_CTRL = WATCHDOG_CTRL_ENABLE(1);
}

void feed_the_watchdog(void)
{
    WATCHDOG_LOAD = reload_value;
}
