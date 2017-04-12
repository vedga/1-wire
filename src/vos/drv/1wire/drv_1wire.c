#include "drv_1wire.h"

#include <vos/kernel.h>
#include <res/stm8/res_1wire.h>

/* Forward module functions declaration */    
static void drvOneWireKernelOperationComplete(uint8_t _status);        

__INLINE void drvOneWireKernelOperationStart() {
    STATUS = ONEWIRE_STATUS_PROGRESS;
}

__INLINE int16_t drvOneWireDecodeBit(drv_onewire_duration_t _duration) {
#if 1
    if(OVERDRIVE) {
        if((_duration < OVERDRIVE_BIT1_MIN_DURATION) || 
           (_duration > OVERDRIVE_BIT0_MAX_DURATION)) {
            return -1;
        }
        
        return (_duration < OVERDRIVE_BIT1_MAX_DURATION) ? 0x80 : 0;
    } else {
        if((_duration < NORMAL_BIT1_MIN_DURATION) || 
           (_duration > NORMAL_BIT0_MAX_DURATION)) {
            return -1;
        }
        
        return (_duration < NORMAL_BIT1_MAX_DURATION) ? 0x80 : 0;
    }
#else    
    if(OVERDRIVE) {
        if(_duration < OVERDRIVE_BIT1_MIN_DURATION) {
            /* Duration less than min */
            return -1;
        } else if(_duration < OVERDRIVE_BIT1_MAX_DURATION) {
            /* Normal bit 1 duration detected */
            return 0x80;
        } else if(_duration < OVERDRIVE_BIT0_MIN_DURATION) {
            /* Invalid bit 0 duration */
            return -1;
        } else if(_duration < OVERDRIVE_BIT0_MAX_DURATION) {
            /* Good bit 0 duration detected */
            return 0;
        } else {
            /* Bit 0 duration too long */
            return -1;
        }
    } else {
        if(_duration < NORMAL_BIT1_MIN_DURATION) {
            /* Duration less than min */
            return -1;
        } else if(_duration < NORMAL_BIT1_MAX_DURATION) {
            /* Normal bit 1 duration detected */
            return 0x80;
        } else if(_duration < NORMAL_BIT0_MIN_DURATION) {
            /* Invalid bit 0 duration */
            return -1;
        } else if(_duration < NORMAL_BIT0_MAX_DURATION) {
            /* Good bit 0 duration detected */
            return 0;
        } else {
            /* Bit 0 duration too long */
            return -1;
        }
    }
#endif
}

#if !defined(__DRV_ONEWIRE_DMA)

__INLINE void drvOneWireKernelBitOperationStart() {
    /* Bit pulse duration */
    resOneWireTimerSetPwmLevel(BIT_DURATION(PARAM_VALUE & 0x01));
            
    /* Shift out sent bit */
    PARAM_VALUE >>= 1;
}

#endif /* !defined(__DRV_ONEWIRE_DMA) */

drv_onewire_context_t drv_onewire_context;

/*
 * Initialise 1-wire driver, attach required hardware resources
 *
 * Enter:
 * none
 *
 * Leave:
 * true if driver successfuly initialised else false
 *
 * This is synchronious operation.
 */
bool_t drvOneWireAttach() {
  /* Source clock frequence */
  uint32_t sysclk = getSysClk();
  
  if(sysclk < TIMER_SOURCE_FREQUENCE) {
    return false;
  }

  uint8_t divider = (uint8_t)(sysclk / TIMER_SOURCE_FREQUENCE);
  
  uint8_t prescaler = 0;
  while(divider > 1) {
    divider >>= 1;
    prescaler++;
  }
  
  if(prescaler & ~ONEWIRE_TIMER_PRESCALER_MASK) {
    return false;
  }
  
  OVERDRIVE = 0;

#if defined(__DRV_ONEWIRE_ACTIVE_PULLUP)
  
  /* Active pullup (OFF) */
  ONEWIRE_TIMER_BKIN_ODR_BIT = ONEWIRE_ACTIVE_PULLUP_POLARITY ? 0 : 1;
  ONEWIRE_TIMER_BKIN_DDR_BIT = 1;
  ONEWIRE_TIMER_BKIN_CR1_BIT = 1;
  ONEWIRE_TIMER_BKIN_CR2_BIT = 1;
  
#endif /* defined(__DRV_ONEWIRE_ACTIVE_PULLUP) */
  
  /* CH1 output mode */
  ONEWIRE_TIMER_CH1_ODR_BIT = ONEWIRE_PWM_IDLE_POLARITY;
  ONEWIRE_TIMER_CH1_DDR_BIT = 1;
  
  /* CH2 input mode */
  ONEWIRE_TIMER_CH2_DDR_BIT = 0;
  
  /* Select push-pull or pseudo open drain for CH1 */
  ONEWIRE_TIMER_CH1_CR1_BIT = 1; /* push-pull*/
  ONEWIRE_TIMER_CH1_CR2_BIT = 1; /* fast mode */
  
  /* Select floating or pullup mode for CH2 */
  ONEWIRE_TIMER_CH2_CR1_BIT = 1; /* pullup ON */
  
  /* Turn on device */
  resOneWireTimerOn();
  
  /* One timer tick is 1us */
  resOneWireTimerPrescaler(prescaler);
  
  /* Setup PWM output channel. At this point we have CH1 out set to idle
  level (specified by TIMx_OISR_OIS1) while MOE == 0 and AOE == 1.
  I.e. on next UEV event MOE is automatic set to 1, output is enabled.
  */
  resOneWireTimerAttachPwm();
  
  /* Setup capture input channel */
  resOneWireTimerAttachCapture();
  
  return true;
}

/*
 * Get current operation status
 *
 * Enter:
 * none
 *
 * Leave:
 * current operation status:
 * ONEWIRE_STATUS_PROGRESS - current operation in progress
 * ONEWIRE_STATUS_COMPLETE - operation successfully complete
 * ONEWIRE_STATUS_ERROR - operation failed and complete
 *
 * This is synchronious operation.
 */
uint8_t drvOneWireStatus() {
    vosRaiseIrqLevel(VOS_CPU_RING_HARD);
    
    uint8_t status = drv_onewire_context.status;
    
    vosRaiseIrqLevel(VOS_CPU_RING_USER);
    
    return status;
}

/*
 * Get received bits in previous complete drvOneWireTxBits() operation
 *
 * Enter:
 * _count - bits count. Must be same as in drvOneWireTxBits() function call.
 *
 * Leave:
 * value < 0 if error (decode failed in DMA mode)
 * value >= 0 successfuly decoded bits
 *
 * This is synchronious operation.
 */
int16_t drvOneWireRxBits(uint8_t _count) {
#if defined(__DRV_ONEWIRE_DMA)
    int16_t value = 0;
    
    drv_onewire_duration_t * p = &PARAM_DURATIONS[_count];
    
    while(&PARAM_DURATIONS[0] != p) {
        /* Shift previous decoded bits */
        value >>= 1;
        
        drv_onewire_duration_t duration = * --p;
        
        value |= drvOneWireDecodeBit(duration);
        
        if(value < 0) {
            /* Bit decode error */
            return value;
        }
    }
    
    /* All bits decoded successfully */
    return value >> (8 - _count);
#else  /* defined(__DRV_ONEWIRE_DMA) */
    
    return PARAM_VALUE >> (8 - _count);
    
#endif /* defined(__DRV_ONEWIRE_DMA) */
}

/*
 * Initiate RESET pulse, monitor following PRESENCE pulse
 *
 * Enter:
 * _overdrive - 0 for normal operation mode, != 0 for overdrive speed
 *
 * Leave:
 * true if operation initiated successfuly (i.e. async operation started)
 * false if operation not initiated and now is unsuccessfully complete.
 *
 * This is asynchronious operation.
 */
bool_t drvOneWireReset(uint8_t _overdrive) {
    /* Operation initiated */
    drvOneWireKernelOperationStart();
    
    OVERDRIVE = _overdrive ? 1 : 0;

    /* Presence not detected yet */
    PRESENCE = 0;
  
    /* Assert idle mode */
    resOneWirePrepare();
  
    /* Procedure max duration */
    resOneWireTimerSetTop(RESET_PROCEDURE_DURATION());
  
    /* Reset pulse duration */
    resOneWireTimerSetPwmLevel(RESET_DURATION());

    /* Enable UEV interrupt */
    resOneWireEnableOverflowIrq();

    /* Start timer */
    resOneWireTimerStart();
  
    /* Update generation event */
    resOneWireFireOverflowEvent();
    
    return true;
}

/*
 * Initiate I/O bits operation over 1-wire bus
 *
 * Enter:
 * _value - sending binary value
 * _count - bit count for I/O operation (must be in range from 1 to 8)
 *
 * Leave:
 * true if operation initiated successfuly (i.e. async operation started)
 * false if operation not initiated and now is unsuccessfully complete.
 *
 * This is asynchronious operation.
 */
bool_t drvOneWireTxBits(uint8_t _value, uint8_t _count) {
    /* Operation initiated */
    drvOneWireKernelOperationStart();
    
    /* Assert idle mode */
    resOneWirePrepare();
  
    /* Bit I/O procedure duration */
    resOneWireTimerSetTop(IO_BIT_PROCEDURE_DURATION());
    
    /* Rising edge capture polarity */
    resOneWireSetCapturePolarity(ONEWIRE_CAPTURE_POLARITY);

#if defined(__DRV_ONEWIRE_DMA)
    /* Turn on DMA module */
    if(!resOneWireDmaAttach()) {
        /* DMA resource BUSY, can't continue operation */
        drvOneWireKernelOperationComplete(ONEWIRE_STATUS_ERROR);
        
        return false;
    }

    /* First bit pulse duration */
    resOneWireTimerSetPwmLevel(BIT_DURATION(_value & 0x01));
    
    /* Initialize capture DMA */
    resOneWirePrepareCaptureDma(_count);
    
    if(--_count) {
        /* Sending more than one bit, initialise PWM dma */
        resOneWirePreparePwmDma(_count);
        
        while(_count--) {
            /* Shift out processed bit */
            _value >>= 1;
            
            /* Calculate next bit duration */
            PARAM_DURATIONS[_count] = BIT_DURATION(_value & 0x01);
        }
    } else {
        /* PWM DMA not used because only one bit to I/O operation */
        resOneWireEnableOverflowIrq();
    }
    
    /* Enable DMA operations */
    resOneWireDmaOn();
    
#else  /* defined(__DRV_ONEWIRE_DMA) */
    
    PARAM_VALUE = _value;
    
    PARAM_COUNT = _count;
    
    /* Prepare first bit I/O operation */
    drvOneWireKernelBitOperationStart();
    
    /* Enable UEV interrupt */
    resOneWireEnableOverflowIrq();
  
#endif /* defined(__DRV_ONEWIRE_DMA) */
    
    /* Start timer */
    resOneWireTimerStart();
  
    /* Update generation event */
    resOneWireFireOverflowEvent();
    
    return true;
}

#if defined(__DRV_ONEWIRE_ACTIVE_PULLUP)

/*
 * Turn ON active pullup
 *
 * Enter:
 * none
 *
 * Leave:
 * none
 *
 * Active-pullup start to produce current for devices attached to 1-wire bus.
 * In this mode any bits I/O operations or RESET operation are disabled
 * (and protected by hardware circuit).
 *
 * This is asynchronious operation w/o status changing.
 */
void drvOneWireActivePullupOn() {
    /* Activate active pullup */
    ONEWIRE_TIMER_BKIN_ODR_BIT = ONEWIRE_ACTIVE_PULLUP_POLARITY;
}

/*
 * Turn OFF active pullup
 *
 * Enter:
 * none
 *
 * Leave:
 * none
 *
 * Active-pullup finish to produce current for devices attached to 1-wire bus.
 * In this mode any bits I/O operations or RESET operation are enabled.
 *
 * This is asynchronious operation w/o status changing.
 */
void drvOneWireActivePullupOff() {
    /* Deactivate active pullup */
    ONEWIRE_TIMER_BKIN_ODR_BIT = ONEWIRE_ACTIVE_PULLUP_POLARITY ? 0 : 1;
}

#endif /* defined(__DRV_ONEWIRE_ACTIVE_PULLUP) */

/*
 * Timer overflow interrupt
 * Generated on UEV event
 */
#pragma vector=ONEWIRE_TIMER_OVERFLOW_IRQ
__interrupt void isrOneWireTimerOverflow(void) {
#if defined(__DRV_ONEWIRE_ACTIVE_PULLUP)
    /* Check active-pullup protection circuit */
    if(resOneWirePullupConflictIrq()) {
        /* Active-pullup is ON, no I/O or RESET operations possible */
        drvOneWireKernelOperationComplete(ONEWIRE_STATUS_ERROR);
        
        /* Other interrups can be ignored because operation is complete */
        return;
    }
#endif /* defined(__DRV_ONEWIRE_ACTIVE_PULLUP) */
    
    if(resOneWireOverflowIrq()) {
        if(PRESENCE) {
            /* Presence detected: Tx/Rx operation */
#if defined(__DRV_ONEWIRE_DMA)
            if(resOneWireTimerGetPwmLevel()) {
                /* Prevent pulse generation after last bit sent */
                resOneWireTimerSetPwmLevel(0);
                
                /* Next overflow IRQ caused rx timeout */
            } else {
                /* Overflow IRQ in DMA mode indicate rx timeout */
                drvOneWireKernelOperationComplete(ONEWIRE_STATUS_ERROR);
            }
#else  /* defined(__DRV_ONEWIRE_DMA) */            
            if(PARAM_COUNT) {
                /* Pending bits count */
                if(--PARAM_COUNT) {
                    /* Started I/O for non-last bit */
                    resOneWireEnablePwmIrq();
                } else {
                    /* Started I/O for last bit */
                    resOneWireDisablePwmIrq();
                }
            
                /* Capture IRQ must be enabled */
                resOneWireEnableCaptureIrq();
            } else {
                /* No more bits to sending but overflow, i.e. last rx bit not captured */
                drvOneWireKernelOperationComplete(ONEWIRE_STATUS_ERROR);
            }
#endif /* defined(__DRV_ONEWIRE_DMA) */
        } else {
            /* Presence not detected: reset procedure in progress */
            if(resOneWireCaptureIrqEnabled()) {
                /* Compare interrupt already enabled: operation complete */

                /* Driver operation complete */
                drvOneWireKernelOperationComplete(ONEWIRE_STATUS_ERROR);
            } else {
                /* Compare interrupt disabled: reset operation now started */
            
                /* Wait RESET pulse rising edge */
                resOneWireEnablePwmIrq();
            }
        }
    }
}

#pragma vector=ONEWIRE_TIMER_CAPTURE_IRQ
__interrupt void isrOneWirePwmCapture(void) {
    if(resOneWirePwmIrq()) {
        /* PWM IRQ */
        if(PRESENCE) {
            /* Presence detected: Tx/Rx operation */

#if defined(__DRV_ONEWIRE_DMA)
            
#else  /* defined(__DRV_ONEWIRE_DMA) */            
            /* Prepare next bit operation */
            drvOneWireKernelBitOperationStart();
#endif /* defined(__DRV_ONEWIRE_DMA) */
        } else {
            /* Presence not detected: RESET operation */
            
            /* Prevent second RESET pulse generation */
            resOneWireTimerSetPwmLevel(0);
            
            /* Capture start presence pulse time point */
            resOneWireSetCapturePolarity(!ONEWIRE_CAPTURE_POLARITY);
      
            /* Enable capture interrupt */
            resOneWireEnableCaptureIrq();
            
            /* Disable PWM interrupt */
            resOneWireDisablePwmIrq();
        }
    }
    
    if(resOneWireCaptureIrq()) {
        /* Capture IRQ */
        if(PRESENCE) {
            /* Presence detected: Tx/Rx operation */
#if defined(__DRV_ONEWIRE_DMA)
            
#else  /* defined(__DRV_ONEWIRE_DMA) */            
            
            /* Captured value */
            uint16_t value = resOneWireTimerGetPwmCapture();
            
            int16_t decoded = drvOneWireDecodeBit(value);
            
            if(decoded < 0) {
                /* Invalid signal */
                drvOneWireKernelOperationComplete(ONEWIRE_STATUS_ERROR);
            } else {
                /* Bit decoded successfully */
                PARAM_VALUE |= (uint8_t)(decoded & 0xFF);
            }
            
            /* Rx bit decode complete */
            if(!PARAM_COUNT) {
                /* Last bit decode complete */
                if(ONEWIRE_STATUS_PROGRESS == STATUS) {
                    /* All bits received successfully */
                    drvOneWireKernelOperationComplete(ONEWIRE_STATUS_COMPLETE);
                }
            }
#endif /* defined(__DRV_ONEWIRE_DMA) */
        } else {
            /* Presence not detected: RESET operation */
            uint16_t value = resOneWireTimerGetPwmCapture();
      
            if(resOneWireGetCapturePolarity() == ONEWIRE_CAPTURE_POLARITY) {
                /* Presence pulse end time point */
        
                /* Presence detection complete */

                /* PRESENCE pulse duration */
                uint16_t duration = value - PARAM_PRESENCE_PULSE_MARK;
        
                /* Check PRESENCE duration value */
                if((duration < PRESENCE_MIN_DURATION()) || 
                   (duration > PRESENCE_MAX_DURATION())) {
                    /* Invalid presence pulse duration */
                    drvOneWireKernelOperationComplete(ONEWIRE_STATUS_ERROR);
                } else {
                    /* Presence detected */
                    PRESENCE = 1;
            
                    /* Reset operation complete */
                    drvOneWireKernelOperationComplete(ONEWIRE_STATUS_COMPLETE);
                }
            } else {
                /* Presence pulse start time point */
                resOneWireSetCapturePolarity(ONEWIRE_CAPTURE_POLARITY);

                /* Delay after RESET and start PRESENCE pulse */
                uint16_t delay = value - RESET_DURATION();
                
                /* Check presence pulse front delay */
                if((delay < PRESENCE_MIN_DELAY()) || 
                   (delay > PRESENCE_MAX_DELAY())) {
                    /* Invalid delay between end of RESET and start of PRESENCE */
                    drvOneWireKernelOperationComplete(ONEWIRE_STATUS_ERROR);
                } else {
                    /* Presence pulse start timepoint */
                    PARAM_PRESENCE_PULSE_MARK = value;
                }
            }
        }
    }
}

#if defined(__DRV_ONEWIRE_DMA)

#pragma vector=ONEWIRE_TIMER_PWM_DMA_IRQ
__interrupt void isrOneWirePwmDma(void) {
    if(resOneWirePwmDmaIrq()) {
        /* If last bit is '0' then possible unexpected pulse if sysclk < 8Mhz.
           Also unexpected pulse may occured at overdrive mode w/o speed
           optimization.
        */
        
        /* On next UEV interrupt we activate last bit watchdog */
        resOneWireEnableOverflowIrq();
    }
}

#pragma vector=ONEWIRE_TIMER_CAPTURE_DMA_IRQ
__interrupt void isrOneWireCaptureDma(void) {
    if(resOneWireCaptureDmaIrq()) {
        /* Capture DMA complete interrupt: I/O operation complete */
        drvOneWireKernelOperationComplete(ONEWIRE_STATUS_COMPLETE);
    }
}

#endif /* defined(__DRV_ONEWIRE_DMA) */

static void drvOneWireKernelOperationComplete(uint8_t _status) {
#if defined(__DRV_ONEWIRE_DMA)    
    /* Turn off DMA module */
    resOneWireDmaOff();
    
    /* Detach DMA module */
    resOneWireDmaDetach();
#endif /* defined(__DRV_ONEWIRE_DMA) */
    
    resOneWireDisableOverflowIrq();
    
    resOneWireDisablePwmIrq();
    
    resOneWireDisableCaptureIrq();
    
    /* PWM control finished, force PWM output at IDLE state */
    resOneWireHalt();
      
    /* Stop timer operation */
    resOneWireTimerStop();
    
    STATUS = _status;
    
    vosKernelFireSignificantEvent();
}
