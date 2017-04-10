#if !defined(__RES_DMA_H)
#define __RES_DMA_H

#include <bit/platform.h>

__INLINE bool_t resDmaAttach() {
    if(CLK_PCKENR2_PCKEN24) {
        /* DMA already active, i.e. attached to other drivers */
        return 0;
    }
    
    /* Activate DMA clock (i.e. DMA power ON) */
    CLK_PCKENR2_PCKEN24 = 1;

    /* Global DMA disable, no timeout */
    DMA1_GCSR = 0;
    
    /* Disable DMA channel 0 */
    DMA1_C0CR = 0;
    
    /* Disable DMA channel 1 */
    DMA1_C1CR = 0;
    
    /* Disable DMA channel 2 */
    DMA1_C2CR = 0;
    
    /* Disable DMA channel 3 */
    DMA1_C3CR = 0;
    
    return 1;
}

__INLINE void resDmaDetach() {
    /* Deactivate DMA clock (i.e. DMA power OFF) */
    CLK_PCKENR2_PCKEN24 = 0;
}

#endif
