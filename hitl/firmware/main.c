#include "control.h"
#include "peripheral.h"

#include <nrf_soc.h>
#include <simpleembed/nrf52/softdevice/common/nrf_sdh.h>
#include <simpleembed/support/delay.h>

// The SDK enables this interrupt in polling mode. Dispatch events in the main loop.
// nrf_soc.h maps SD_EVT_IRQHandler to the SWI2 vector name. Without it this empty handler is never linked and the
// first SoftDevice event lands in Default_Handler.
void SD_EVT_IRQHandler(void) {}

int main(void) {
    control_init();
    peripheral_init();

    for (;;) {
        nrf_sdh_evts_poll();
        control_poll();
        peripheral_poll();
        nrf_delay_ms(1);
    }
}
