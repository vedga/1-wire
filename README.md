# 1-wire
1-Wire bus driver for embedded applications

This is event-based driver for 1-wire bus for embedded applications (i.e. applications w/o operation system).
Current release support STM8L devices and dedicated to compile with IAR toolchain.

STM8L-Discovery board can be used as test board for this code.

Code can be compiled with different versions:
1. IRQ-only (DMA not used). Must be compiled with __DRV_ONEWIRE_DMA undefined.
2. DMA with 8-bit transfer. Must be compiled with __DRV_ONEWIRE_DMA=1
3. DMA with 16-bit transfer. Must be compiled with __DRV_ONEWIRE_DMA=1

Any above versions support Active-Pullup with overcurrent schematic protection. Active-Pullup support code compiled if defined symbol __DRV_ONEWIRE_ACTIVE_PULLUP.

Driver's hardware resources collected in the res_1wire.h file. Most significant defines:

ONEWIRE_PWM_IDLE_POLARITY - active polarity for output signal modulation (0 mean low level on device pin when high level on 1-wire bus; 1 mean high level on device pin when high level on 1-wire bus).

ONEWIRE_CAPTURE_POLARITY - active polarity for input signal (0 mean low level on 1-wire bus is active; 1 mean high level on 1-wire bus is active).

ONEWIRE_ACTIVE_PULLUP_POLARITY - Active-Pullup polarity (0 mean low level on device pin when Active-Pullup is ON; 1 mean high level on device pin when Active-Pullup is ON).

Also this project contain some high-level procedures, base on protothreads library:

PT_THREAD(ptOneWireProbeBus(struct pt * _pt, struct pt * _nested))
Probe the 1-wire bus, determine it capabilities and power requirements.

PT_THREAD(ptOneWireWalkROM(pt_onewire_search_context_t * _ctx))
Enumerate devices connected to the 1-Wire bus.
