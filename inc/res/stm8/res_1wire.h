#if !defined(__RES_1WIRE_H)
#define __RES_1WIRE_H

#include <res/stm8/res_chip.h>

#if defined(__DRV_ONEWIRE_DMA)
#include <res/stm8/res_dma.h>
#endif /* defined(__DRV_ONEWIRE_DMA) */

/*
* Hardware selection guide:
* 1. Can use any timers with (minimum) one PWM channel and one Capture channel
* 2. If use Active-Pullup, then it must be connected to the pin with BREAK
*    signal on selected timer. This provide hardware prevent to set LOW output
*    level on the 1-wire bus while active-pullup is ON.
*
*
* This programm use following schematic connections:
* 
* TIM3_CH1 - PB1 (PWM)
* TIM3_CH2 - PD0 (capture)
* TIM3_ETR - PD1
* TIM3_BKIN - PA5 (active pullup)
*
* Hardware connections:
* - timer's channel 1 used as PWM to generate output signals;
* - timer's channel 2 used as CAPTURE to measure input signals;
*
* TIM3 U (Overflow) DMA channel 0
* TIM3 CH1 (PWM) DMA channel 1
* TIM3 CH2 (Capture) DMA channel 2
*/

/* Idle polarity for PWM output pin (low level) */
#define ONEWIRE_PWM_IDLE_POLARITY       0

/* Low to high level edge act as capture trigger */
#define ONEWIRE_CAPTURE_POLARITY        0

/* Active pullup active level high */
#define ONEWIRE_ACTIVE_PULLUP_POLARITY  1

#define ONEWIRE_TIMER_OVERFLOW_IRQ      TIM3_OVR_UIF_vector
#define ONEWIRE_TIMER_CAPTURE_IRQ       TIM3_CAPCOM_BIF_vector    

#if defined(__DRV_ONEWIRE_DMA)
#define ONEWIRE_TIMER_PWM_DMA_IRQ       DMA1_CH1_TC_vector
#define ONEWIRE_TIMER_CAPTURE_DMA_IRQ   DMA1_CH2_TC_vector
#endif /* defined(__DRV_ONEWIRE_DMA) */

#define ONEWIRE_TIMER_CH1_DDR_BIT       PB_DDR_DDR1
#define ONEWIRE_TIMER_CH1_ODR_BIT       PB_ODR_ODR1
#define ONEWIRE_TIMER_CH1_CR1_BIT       PB_CR1_C11
#define ONEWIRE_TIMER_CH1_CR2_BIT       PB_CR2_C21

#define ONEWIRE_TIMER_CH2_DDR_BIT       PD_DDR_DDR0
#define ONEWIRE_TIMER_CH2_CR1_BIT       PD_CR1_C10
#define ONEWIRE_TIMER_CH2_CR2_BIT       PD_CR2_C20

#define ONEWIRE_TIMER_BKIN_DDR_BIT      PA_DDR_DDR5
#define ONEWIRE_TIMER_BKIN_ODR_BIT      PA_ODR_ODR5
#define ONEWIRE_TIMER_BKIN_IDR_BIT      PA_IDR_IDR5
#define ONEWIRE_TIMER_BKIN_CR1_BIT      PA_CR1_C15
#define ONEWIRE_TIMER_BKIN_CR2_BIT      PA_CR2_C25


#define ONEWIRE_TIMER_PRESCALER_MASK                                    \
    MASK_TIM3_PSCR_PSC
    

__INLINE bool_t resOneWireOverflowIrq() {
    if(TIM3_SR1_UIF) {
        /* Clear IRQ request */
        TIM3_SR1_UIF = 0;
        
        return 1;
    }
    
    return 0;
}

__INLINE bool_t resOneWirePwmIrq() {
    if(TIM3_SR1_CC1IF) {
        /* Clear IRQ request */
        TIM3_SR1_CC1IF = 0;
        
        return 1;
    }
    
    return 0;
}

__INLINE bool_t resOneWireCaptureIrq() {
    if(TIM3_SR1_CC2IF) {
        /* Clear IRQ request */
        TIM3_SR1_CC2IF = 0;
        
        return 1;
    }
    
    return 0;
}

#if defined(__DRV_ONEWIRE_ACTIVE_PULLUP)

__INLINE bool_t resOneWirePullupConflictIrq() {
    if(TIM3_SR1_BIF) {
        TIM3_SR1_BIF = 0;
        
        return 1;
    }

    return 0;
}

#endif /* defined(__DRV_ONEWIRE_ACTIVE_PULLUP) */

__INLINE void resOneWireEnablePwmIrq() {
    TIM3_IER_CC1IE = 1;
}

__INLINE void resOneWireDisablePwmIrq() {
    TIM3_IER_CC1IE = 0;
}

__INLINE bool_t resOneWireCaptureIrqEnabled() {
    return TIM3_IER_CC2IE ? 1 : 0;
}

__INLINE void resOneWireEnableCaptureIrq() {
    TIM3_IER_CC2IE = 1;
}

__INLINE void resOneWireDisableCaptureIrq() {
    TIM3_IER_CC2IE = 0;
}

__INLINE void resOneWireEnableOverflowIrq() {
    TIM3_SR1_UIF = 0;
    TIM3_IER_UIE = 1;
}

__INLINE void resOneWireDisableOverflowIrq() {
    TIM3_IER_UIE = 0;
}

__INLINE void resOneWireFireOverflowEvent() {
    TIM3_EGR_UG = 1;
}

__INLINE void resOneWirePrepare() {
#if defined(__DRV_ONEWIRE_ACTIVE_PULLUP)
    /* Clear possible active-pullup conflict flag */
    resOneWirePullupConflictIrq();
#endif /* defined(__DRV_ONEWIRE_ACTIVE_PULLUP) */
    
    /* Timer channels in idle mode, start on UEV event */
    TIM3_BKR_AOE = 1;
    TIM3_BKR_MOE = 0;
}
 
__INLINE void resOneWireHalt() {
    /* Timer channels in idle mode, start only by software */
    TIM3_BKR_AOE = 0;
    TIM3_BKR_MOE = 0;
}

__INLINE void resOneWireTimerOn() {
    pwrTimer3On();
  
    /* Disable counter */
    TIM3_CR1_CEN = 0;
  
    /* ARR register is not buffered */
    TIM3_CR1_ARPE = 0;
  
    /* Edge-aligned mode */
    TIM3_CR1_CMS = 0;
  
    /* Up-counter direction */
    TIM3_CR1_DIR = 0;
  
    /* Continuous mode */
    TIM3_CR1_OPM = 0;
  
    /* UDEV is generated */
    TIM3_CR1_UDIS = 0;
  
#if defined(__DRV_ONEWIRE_ACTIVE_PULLUP)
    TIM3_BKR = /* MASK_TIM3_BKR_AOE  | */
             MASK_TIM3_BKR_OSSI | 
             MASK_TIM3_BKR_BKE  | 
             (ONEWIRE_ACTIVE_PULLUP_POLARITY ? MASK_TIM3_BKR_BKP : 0);
#else /* defined(__DRV_ONEWIRE_ACTIVE_PULLUP) */
    TIM3_BKR = /* MASK_TIM3_BKR_AOE  | */
             MASK_TIM3_BKR_OSSI;
#endif /* defined(__DRV_ONEWIRE_ACTIVE_PULLUP) */
}

__INLINE void resOneWireTimerOff() {
    /* Disable counter */
    TIM3_CR1_CEN = 0;
  
    pwrTimer3Off();
}

__INLINE void resOneWireTimerStart() {
    /* Force capture inerrupt requests are cleared before start timer */
    TIM3_SR1_CC1IF = 0;
    TIM3_SR1_CC2IF = 0;
    
    TIM3_CR1_CEN = 1;
}

__INLINE void resOneWireTimerStop() {
    TIM3_CR1_CEN = 0;
}

__INLINE void resOneWireTimerPrescaler(uint8_t _prescaler) {
    /* Use internal clock */
    TIM3_SMCR_SMS = 0;
    TIM3_ETR_ECE = 0;
  
    TIM3_PSCR_PSC = _prescaler & ONEWIRE_TIMER_PRESCALER_MASK;
}

__INLINE uint16_t resOneWireTimerGetTop() {
    uint16_t value = (uint16_t)TIM3_ARRH << 8;
  
    return value | TIM3_ARRL;
}

__INLINE void resOneWireTimerSetTop(uint16_t _value) {
    TIM3_ARRH = (uint8_t)(_value >> 8);
  
    TIM3_ARRL = (uint8_t)(_value & 0xFF);
}

__INLINE uint16_t resOneWireTimerGetCounter() {
    uint16_t value = (uint16_t)TIM3_CNTRH << 8;
  
    return value | TIM3_CNTRL;
}

__INLINE void resOneWireTimerSetCounter(uint16_t _value) {
    TIM3_CNTRH = (uint8_t)(_value >> 8);
  
    TIM3_CNTRL = (uint8_t)(_value & 0xFF);
}

__INLINE void resOneWireTimerAttachPwm() {
    /* CH1 off */
    TIM3_CCER1_CC1E = 0;
  
    /* CH1 output */
    TIM3_CCMR1_CC1S = 0;
  
    /* ob110 - PWM mode 1, 0b111 - PWM mode 2 */
    TIM3_CCMR1_OC1M = 6;
  
    /* CCR1 preload mode ON */
    TIM3_CCMR1_OC1PE = 1;
  
    /* Normal mode */
    TIM3_CCMR1_OC1FE = 0;
  
    /* CH1 polarity. 0 is active high, 1 is active low. */
    TIM3_CCER1_CC1P = ONEWIRE_PWM_IDLE_POLARITY;

    /* As TIM3_BKR_OSSI == 1, this is idle level for CH1. */
    /* Really activated when TIM3_CCER1_CC1E is set to 1. */
    TIM3_OISR_OIS1 = ONEWIRE_PWM_IDLE_POLARITY;
  
    /* CH1 connected to external pin */
    TIM3_CCER1_CC1E = 1;
}

__INLINE void resOneWireTimerAttachCapture() {
    /* CH2 off */
    TIM3_CCER1_CC2E = 0;

    /* CH2 input, IC2 mapped to TI2FP2 */
    //TIM3_CCMR2_CC2S = 1;
  
    /* CH2 input, IC2 mapped to TI2FP2, no capture filters and prescaler */
    TIM3_CCMR2 = 1 /*| (0x01 << 4)*/;
  
    /* Capture filter: Fsampling = Fsysclk, N = 8 */
//  TIM3_CCMR2 = (TIM3_CCMR2 & 0x0F) | (0x03 << 4);
  
    /* Capture polarity */
    TIM3_CCER1_CC2P = ONEWIRE_CAPTURE_POLARITY;
  
    /* CH2 connected to external pin (enable capture) */
    TIM3_CCER1_CC2E = 1;
}

__INLINE void resOneWireTimerSetPwmLevel(uint16_t _value) {
    TIM3_CCR1H = (uint8_t)(_value >> 8);
  
    TIM3_CCR1L = (uint8_t)(_value & 0xFF);
}

__INLINE uint16_t resOneWireTimerGetPwmLevel() {
    uint16_t value = (uint16_t)TIM3_CCR1H << 8;
  
    return value | TIM3_CCR1L;
}

__INLINE void resOneWireTimerSetCapture(uint16_t _value) {
    TIM3_CCR2H = (uint8_t)(_value >> 8);
  
    TIM3_CCR2L = (uint8_t)(_value & 0xFF);
}

__INLINE uint16_t resOneWireTimerGetPwmCapture() {
    uint16_t value = (uint16_t)TIM3_CCR2H << 8;
  
    return value | TIM3_CCR2L;
}

__INLINE uint8_t resOneWireGetCapturePolarity() {
    return TIM3_CCER1_CC2P;
}

__INLINE void resOneWireSetCapturePolarity(uint8_t _polarity) {
    TIM3_CCER1_CC2E = 0;
    TIM3_CCER1_CC2P = _polarity ? 1 : 0;
    TIM3_CCER1_CC2E = 1;
    
    /* Clear possible changed capture IRQ request flag */
    resOneWireCaptureIrq();
}

#if defined(__DRV_ONEWIRE_DMA)

#if (__DRV_ONEWIRE_DMA == 1)
#define ONEWIRE_DMA_BLOCK_SIZE          0
#define ONEWIRE_DMA_PWM_TARGET          ((uint16_t)&TIM3_CCR1L)
#define ONEWIRE_DMA_CAPTURE_TARGET      ((uint16_t)&TIM3_CCR2L)
#elif (__DRV_ONEWIRE_DMA == 2)
#define ONEWIRE_DMA_BLOCK_SIZE          MASK_DMA1_C1SPR_TSIZE
#define ONEWIRE_DMA_PWM_TARGET          ((uint16_t)&TIM3_CCR1H)
#define ONEWIRE_DMA_CAPTURE_TARGET      ((uint16_t)&TIM3_CCR2H)
#else /* (__DRV_ONEWIRE_DMA == 2) */
#error "OneWire DMA size not supported yet"
#endif /* (__DRV_ONEWIRE_DMA == 1) */


__INLINE void resOneWireDmaOn() {
    /* Globally enable DMA */
    DMA1_GCSR_GEN = 1;
}

__INLINE void resOneWireDmaOff() {
    /* Globally disable DMA */
    DMA1_GCSR_GEN = 0;
    
    /* Disable DMA request from timer's channel 1 (PWM) */
    TIM3_DER_CC1DE = 0;
    
    /* Disable DMA request from timer's channel 2 (Capture) */
    TIM3_DER_CC2DE = 0;
}

__INLINE bool_t resOneWirePwmDmaIrq() {
    if(DMA1_C1SPR_TCIF) {
        DMA1_C1SPR_TCIF = 0;
        
        return 1;
    }
    
    return 0;
}

__INLINE bool_t resOneWireCaptureDmaIrq() {
    if(DMA1_C2SPR_TCIF) {
        DMA1_C2SPR_TCIF = 0;
        
        return 1;
    }
    
    return 0;
}

__INLINE bool_t resOneWireDmaAttach() {
    return resDmaAttach();
}

__INLINE void resOneWireDmaDetach() {
    resOneWireDmaOff();
    
    resDmaDetach();
}

__INLINE void resOneWirePreparePwmDma(uint8_t _count) {
    /* Non-circular, from mem to peripheral, interrupt on complete */
    DMA1_C1CR = MASK_DMA1_C1CR_DIR | MASK_DMA1_C1CR_TCIE;
    
    /* Low priority, n-bit size */
    DMA1_C1SPR = ONEWIRE_DMA_BLOCK_SIZE;

    /* Count */
    DMA1_C1NDTR = _count;

    /* Target hardware address: TIM3 CCR1 (PWM) register (MSB, even) */
    DMA1_C1PARH = (uint8_t)(ONEWIRE_DMA_PWM_TARGET >> 8);
    DMA1_C1PARL = (uint8_t)(ONEWIRE_DMA_PWM_TARGET & 0xFF);
    
    /* Source memory address (last byte as used post-decrement) */
    DMA1_C1M0ARH = (uint8_t)((uint16_t)&drv_onewire_context.op.io.durations[_count - 1] >> 8);
    DMA1_C1M0ARL = (uint8_t)((uint16_t)&drv_onewire_context.op.io.durations[_count - 1] & 0xFF); 
  
    /* Enable DMA channel 1 */
    DMA1_C1CR_EN = 1;
    
    /* Enable DMA request from timer's channel 1 (PWM) */
    TIM3_DER_CC1DE = 1;
}

__INLINE void resOneWirePrepareCaptureDma(uint8_t _count) {
    /* Non-circular, from peripheral to mem, interrupt on complete */
    DMA1_C2CR = MASK_DMA1_C2CR_TCIE;
    
    /* Low priority, n-bit size */
    DMA1_C2SPR = ONEWIRE_DMA_BLOCK_SIZE;
    
    /* Count */
    DMA1_C2NDTR = _count;

    /* Source hardware address: TIM3 CCR2 (Capture) register (MSB, even) */
    DMA1_C2PARH = (uint8_t)(ONEWIRE_DMA_CAPTURE_TARGET >> 8);
    DMA1_C2PARL = (uint8_t)(ONEWIRE_DMA_CAPTURE_TARGET & 0xFF);
    
    /* Target memory address (last byte as used post-decrement) */
    DMA1_C2M0ARH = (uint8_t)((uint16_t)&drv_onewire_context.op.io.durations[_count - 1] >> 8);
    DMA1_C2M0ARL = (uint8_t)((uint16_t)&drv_onewire_context.op.io.durations[_count - 1] & 0xFF); 
  
    /* Enable DMA channel 1 */
    DMA1_C2CR_EN = 1;
    
    /* Enable DMA request from timer's channel 2 (Capture) */
    TIM3_DER_CC2DE = 1;
}

#endif /* defined(__DRV_ONEWIRE_DMA) */

#endif /* !defined(__RES_1WIRE_H) */
