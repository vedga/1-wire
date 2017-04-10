#if !defined(__KERNEL_H)
#define __KERNEL_H

#include <bit/platform.h>

__EXTERN void vosIdle();

__INLINE void vosKernelFireSignificantEvent() {
    CPU_CFG_GCR_AL = 0;
}

/*
* Clear significant event state.
* Return true if previous state is fired or false if not fired.
*/
__INLINE bool_t vosKernelSignificantEvent() {
    bool_t value = CPU_CFG_GCR_AL ? 0 : 1;
    
    CPU_CFG_GCR_AL = 1;
    
    return value;
}

#define VOS_CPU_RING_USER       (MASK_CPU_CCR_I1)
#define VOS_CPU_RING_KERNEL     (MASK_CPU_CCR_I0)
#define VOS_CPU_RING_SOFT       (0)
#define VOS_CPU_RING_HARD       (MASK_CPU_CCR_I1|MASK_CPU_CCR_I0)

__INLINE void vosRaiseIrqLevel(uint8_t _level) {
    __set_interrupt_state(_level);
}

#endif
