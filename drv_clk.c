#include "board.h"

static uint8_t desiredPrescaler;

/**
 * Get current value of sysclk
 */
uint32_t getSysClk() {
  uint32_t source;
  
  switch(CLK_SCSR) {
  case CLK_SOURCE_HSI:
    /* HSI */
    source = HSI_HZ;
    
    break;
  case CLK_SOURCE_LSI:
    /* LSI */
    source = LSI_HZ;
    
    break;
  case CLK_SOURCE_HSE:
    /* HSE */
    source = HSE_HZ;
    
    break;
  case CLK_SOURCE_LSE:
    /* LSE */
    source = LSE_HZ;
    
    break;
  default:
    return ~0;
  }
  
  return source >> CLK_CKDIVR_CKM;
}

int8_t setSysClkSource(uint8_t _source, uint8_t _prescaler) {
  uint8_t complete;
  
  if(_prescaler & ~MASK_CLK_CKDIVR_CKM) {
    /* Invalid prescaler value */
    return -1;
  }
  
  __disable_interrupt();

  if(CLK_SCSR == _source) {
    /* Source don't changed, only prescaler */
    CLK_CKDIVR_CKM = _prescaler;
    
    complete = 1;
  } else {
    /* Desired prescaler value */
    desiredPrescaler = _prescaler;
  
    /* Start automatic clock switching procedure */
    CLK_SWCR_SWEN = 1;
  
    /* Generate IRQ on operation complete */
    CLK_SWCR_SWIEN = 1;
  
    /* Desired clock source */
    CLK_SWR = _source;
    
    complete = 0;
  }
  
  __enable_interrupt();
  
  return complete;
}

/*
 * System clock switch complete interrupt
 */
#pragma vector=CLK_SWITCH_vector
__interrupt void sysclk_switch_complete(void) {
  /* Clear interrupt request flag */
  CLK_SWCR_SWIF = 0;
  
  /* Set desired prescaler value */
  CLK_CKDIVR_CKM = desiredPrescaler;
  
  ledBlueOn();
  
  __no_operation();
}
