
#include <vos/drv/onewire.h>
#include "timing.h"

#if defined(__DRV_ONEWIRE_DMA)
#if (__DRV_ONEWIRE_DMA == 1)
/* Desired timer's resolution, ns */
#define TIMER_RESOLUTION_NS     500UL
#elif (__DRV_ONEWIRE_DMA == 2)
/* Desired timer's resolution, ns */
#define TIMER_RESOLUTION_NS     250UL
#else /* (__DRV_ONEWIRE_DMA == 2) */
#error "OneWire DMA size not supported yet"
#endif /* (__DRV_ONEWIRE_DMA == 1) */
#else
/* Desired timer's resolution, ns */
#define TIMER_RESOLUTION_NS     250UL
#endif /* defined(__DRV_ONEWIRE_DMA) */

/* Timer ticks per 1us */
#define TIMER_TICKS_PER_US      (1000UL / TIMER_RESOLUTION_NS)

/* Desired timer source frequence */
#define TIMER_SOURCE_FREQUENCE  (TIMER_TICKS_PER_US * 1000000UL)

#define NORMAL_RESET_DURATION                                           \
    (NORMAL_RESET * TIMER_TICKS_PER_US)        
#define NORMAL_RESET_PROCEDURE_DURATION                                 \
    ((NORMAL_RESET + NORMAL_PRESENCE_START_MAX + NORMAL_PRESENCE_MAX) * TIMER_TICKS_PER_US)        
#define OVERDRIVE_RESET_DURATION                                        \
    (OVERDRIVE_RESET * TIMER_TICKS_PER_US)        
#define OVERDRIVE_RESET_PROCEDURE_DURATION                              \
    ((OVERDRIVE_RESET + OVERDRIVE_PRESENCE_START_MAX + OVERDRIVE_PRESENCE_MAX) * TIMER_TICKS_PER_US)

#define NORMAL_BIT1_DURATION                                            \
    (NORMAL_BIT1 * TIMER_TICKS_PER_US)
#define NORMAL_BIT0_DURATION                                            \
    (NORMAL_BIT0 * TIMER_TICKS_PER_US)
#define OVERDRIVE_BIT1_DURATION                                         \
    (OVERDRIVE_BIT1 * TIMER_TICKS_PER_US)
#define OVERDRIVE_BIT0_DURATION                                         \
    (OVERDRIVE_BIT0 * TIMER_TICKS_PER_US)

#define NORMAL_PRESENCE_MIN_DELAY                                       \
    (NORMAL_PRESENCE_START_MIN * TIMER_TICKS_PER_US)
#define NORMAL_PRESENCE_MAX_DELAY                                       \
    (NORMAL_PRESENCE_START_MAX * TIMER_TICKS_PER_US)
#define NORMAL_PRESENCE_MIN_DURATION                                    \
    (NORMAL_PRESENCE_MIN * TIMER_TICKS_PER_US)
#define NORMAL_PRESENCE_MAX_DURATION                                    \
    (NORMAL_PRESENCE_MAX * TIMER_TICKS_PER_US)

#define OVERDRIVE_PRESENCE_MIN_DELAY                                    \
    (OVERDRIVE_PRESENCE_START_MIN * TIMER_TICKS_PER_US)
#define OVERDRIVE_PRESENCE_MAX_DELAY                                    \
    (OVERDRIVE_PRESENCE_START_MAX * TIMER_TICKS_PER_US)
#define OVERDRIVE_PRESENCE_MIN_DURATION                                 \
    (OVERDRIVE_PRESENCE_MIN * TIMER_TICKS_PER_US)
#define OVERDRIVE_PRESENCE_MAX_DURATION                                 \
    (OVERDRIVE_PRESENCE_MAX * TIMER_TICKS_PER_US)

#define NORMAL_BIT1_MIN_DURATION                                        \
    (NORMAL_BIT1_MIN * TIMER_TICKS_PER_US)
#define NORMAL_BIT1_MAX_DURATION                                        \
    (NORMAL_BIT1_MAX * TIMER_TICKS_PER_US)
#define NORMAL_BIT0_MIN_DURATION                                        \
    (NORMAL_BIT0_MIN * TIMER_TICKS_PER_US)
#define NORMAL_BIT0_MAX_DURATION                                        \
    (NORMAL_BIT0_MAX * TIMER_TICKS_PER_US)

#define OVERDRIVE_BIT1_MIN_DURATION                                     \
    (OVERDRIVE_BIT1_MIN * TIMER_TICKS_PER_US)
#define OVERDRIVE_BIT1_MAX_DURATION                                     \
    (OVERDRIVE_BIT1_MAX * TIMER_TICKS_PER_US)
#define OVERDRIVE_BIT0_MIN_DURATION                                     \
    (OVERDRIVE_BIT0_MIN * TIMER_TICKS_PER_US)
#define OVERDRIVE_BIT0_MAX_DURATION                                     \
    (OVERDRIVE_BIT0_MAX * TIMER_TICKS_PER_US)
        
#define PARAM_PRESENCE_PULSE_MARK                                       \
    drv_onewire_context.op.reset.presencePulseMark

#if defined(__DRV_ONEWIRE_DMA)

#define PARAM_DURATIONS                                                 \
    drv_onewire_context.op.io.durations
        
#else  /* defined(__DRV_ONEWIRE_DMA) */

#define PARAM_VALUE                                                     \
    drv_onewire_context.op.io.value

#define PARAM_COUNT                                                     \
    drv_onewire_context.op.io.count

#endif /* defined(__DRV_ONEWIRE_DMA) */

#define OVERDRIVE                                                       \
    drv_onewire_context.overdrive

#define STATUS                                                          \
    drv_onewire_context.status
        
#define PRESENCE                                                        \
    drv_onewire_context.presence
        
#define RESET_DURATION()                                                \
    (OVERDRIVE ? OVERDRIVE_RESET_DURATION : NORMAL_RESET_DURATION)

#define RESET_PROCEDURE_DURATION()                                      \
    (OVERDRIVE ? OVERDRIVE_RESET_PROCEDURE_DURATION : NORMAL_RESET_PROCEDURE_DURATION)
        
#define PRESENCE_MIN_DELAY()                                            \
    (OVERDRIVE ? OVERDRIVE_PRESENCE_MIN_DELAY : NORMAL_PRESENCE_MIN_DELAY)
            
#define PRESENCE_MAX_DELAY()                                            \
    (OVERDRIVE ? OVERDRIVE_PRESENCE_MAX_DELAY : NORMAL_PRESENCE_MAX_DELAY)
        
#define PRESENCE_MIN_DURATION()                                         \
    (OVERDRIVE ? OVERDRIVE_PRESENCE_MIN_DURATION : NORMAL_PRESENCE_MIN_DURATION)

#define PRESENCE_MAX_DURATION()                                         \
    (OVERDRIVE ? OVERDRIVE_PRESENCE_MAX_DURATION : NORMAL_PRESENCE_MAX_DURATION)


#define IO_BIT_PROCEDURE_DURATION()                                     \
    (OVERDRIVE ? ((OVERDRIVE_TSLOT_MAX + OVERDRIVE_TREC_MIN) * TIMER_TICKS_PER_US) : ((NORMAL_TSLOT_MAX + NORMAL_TREC_MIN) * TIMER_TICKS_PER_US))
        
#define BIT_DURATION(_bit)                                              \
    ((_bit) ? (OVERDRIVE ? OVERDRIVE_BIT1_DURATION : NORMAL_BIT1_DURATION) : (OVERDRIVE ? OVERDRIVE_BIT0_DURATION : NORMAL_BIT0_DURATION))
