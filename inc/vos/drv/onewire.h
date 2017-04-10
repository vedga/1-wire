#if !defined(__ONEWIRE_H)
#define __ONEWIRE_H

#include <bit/platform.h>

#if defined(__DRV_ONEWIRE_DMA)
#if (__DRV_ONEWIRE_DMA == 1)
typedef uint8_t drv_onewire_duration_t;
#elif (__DRV_ONEWIRE_DMA == 2)
typedef uint16_t drv_onewire_duration_t;
#else /* (__DRV_ONEWIRE_DMA == 2) */
#error "OneWire DMA size not supported yet"
#endif /* (__DRV_ONEWIRE_DMA == 1) */
#else /* defined(__DRV_ONEWIRE_DMA) */
typedef uint16_t drv_onewire_duration_t;
#endif /* defined(__DRV_ONEWIRE_DMA) */


typedef struct {
    union {
        struct {
            /* Timer's counter value, when presence pulse is started */
            uint16_t presencePulseMark;
        } reset;
        struct {
#if defined(__DRV_ONEWIRE_DMA)
            /* Bits duration values */
            drv_onewire_duration_t durations[8];
#else  /* defined(__DRV_ONEWIRE_DMA) */
            /* I/O value */
            uint8_t value;
            /* Pending bits count */
            uint8_t count;
#endif /* defined(__DRV_ONEWIRE_DMA) */
        } io;
    } op;
    /* Overdrive mode */
    uint8_t overdrive:1;
    /* Presence pulse detected */
    volatile uint8_t presence:1;
    /* Operation status */
    volatile uint8_t status:2;
} drv_onewire_context_t;

#define ONEWIRE_STATUS_PROGRESS         0
#define ONEWIRE_STATUS_COMPLETE         1
#define ONEWIRE_STATUS_ERROR            2

__EXTERN drv_onewire_context_t drv_onewire_context;
__EXTERN bool_t drvOneWireAttach();    
__EXTERN uint8_t drvOneWireStatus();
__EXTERN int16_t drvOneWireRxBits(uint8_t _count);
__EXTERN bool_t drvOneWireReset(uint8_t _overdrive);
__EXTERN bool_t drvOneWireTxBits(uint8_t _value, uint8_t _count);

#if defined(__DRV_ONEWIRE_ACTIVE_PULLUP)
__EXTERN void drvOneWireActivePullupOn();
__EXTERN void drvOneWireActivePullupOff();
#else /* defined(__DRV_ONEWIRE_ACTIVE_PULLUP) */
#define drvOneWireActivePullupOn()
#define drvOneWireActivePullupOff()
#endif /* defined(__DRV_ONEWIRE_ACTIVE_PULLUP) */

#endif
