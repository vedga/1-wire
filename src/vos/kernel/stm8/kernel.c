#include <vos/kernel.h>

void vosIdle() {
    __disable_interrupt();
    
    if(vosKernelSignificantEvent()) {
        __enable_interrupt();
    } else {
        __wait_for_interrupt();
    }
}
