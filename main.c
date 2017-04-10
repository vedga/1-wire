#include <string.h>

#include "board.h"
#include <vos/drv/onewire.h>

#include <vos/kernel.h>

static void indicateError();
static int waitComplete();
    
int main( void )
{
    int c;
    
    initBoard();
  
//  setSysClkSource(CLK_SOURCE_LSI, 0); //OK
    setSysClkSource(CLK_SOURCE_HSI, 0); //OK
//  setSysClkSource(CLK_SOURCE_LSE, 0); //OK
//  setSysClkSource(CLK_SOURCE_HSE, 0); //FAIL

    while(CLK_SWCR_SWBSY) ;
    
    drvOneWireAttach();
  
    ledGreenOn();

    /* Overdrive presence detection */
    drvOneWireReset(1);
    if((c = waitComplete()) == ONEWIRE_STATUS_ERROR) {
        /* No overdrive-compatible devices, check normal presence pulse */
        drvOneWireReset(0);
        if((c = waitComplete()) == ONEWIRE_STATUS_ERROR) {
            /* No presence pulse detected */
            indicateError();
        }
    }
  
    drvOneWireTxBits(0x00, 1);
    if((c = waitComplete()) == ONEWIRE_STATUS_ERROR) {
        indicateError();
    }
    if(((c = drvOneWireRxBits(1)) < 0) || (c != 0)) {
        indicateError();
    }

    drvOneWireTxBits(0x01, 1);
    if((c = waitComplete()) == ONEWIRE_STATUS_ERROR) {
        indicateError();
    }
    if(((c = drvOneWireRxBits(1)) < 0) || (c != 1)) {
        indicateError();
    }
    
    drvOneWireTxBits(0x02, 2);
    if((c = waitComplete()) == ONEWIRE_STATUS_ERROR) {
        indicateError();
    }
    if(((c = drvOneWireRxBits(2)) < 0) || (c != 0x02)) {
        indicateError();
    }

    drvOneWireTxBits(0x01, 2);
    if((c = waitComplete()) == ONEWIRE_STATUS_ERROR) {
        indicateError();
    }
    if(((c = drvOneWireRxBits(2)) < 0) || (c != 0x01)) {
        indicateError();
    }

    drvOneWireTxBits(0xAA, 8);
    if((c = waitComplete()) == ONEWIRE_STATUS_ERROR) {
        indicateError();
    }
    if(((c = drvOneWireRxBits(8)) < 0) || (c != 0xAA)) {
        indicateError();
    }

    drvOneWireTxBits(0x55, 8);
    if((c = waitComplete()) == ONEWIRE_STATUS_ERROR) {
        indicateError();
    }
    if(((c = drvOneWireRxBits(8)) < 0) || (c != 0x55)) {
        indicateError();
    }

    drvOneWireTxBits(0xF0, 8);
    if((c = waitComplete()) == ONEWIRE_STATUS_ERROR) {
        indicateError();
    }
    if(((c = drvOneWireRxBits(8)) < 0) || (c != 0xF0)) {
        indicateError();
    }
    
    drvOneWireTxBits(0x0F, 8);
    if((c = waitComplete()) == ONEWIRE_STATUS_ERROR) {
        indicateError();
    }
    if(((c = drvOneWireRxBits(8)) < 0) || (c != 0x0F)) {
        indicateError();
    }

    drvOneWireTxBits(0x00, 8);
    if((c = waitComplete()) == ONEWIRE_STATUS_ERROR) {
        indicateError();
    }
    if(((c = drvOneWireRxBits(8)) < 0) || (c != 0x00)) {
        indicateError();
    }

    drvOneWireTxBits(0xFF, 8);
    if((c = waitComplete()) == ONEWIRE_STATUS_ERROR) {
        indicateError();
    }
    if(((c = drvOneWireRxBits(8)) < 0) || (c != 0xFF)) {
        indicateError();
    }
    
#if defined(__DRV_ONEWIRE_ACTIVE_PULLUP)
    /* Active-pullup protection test */
    drvOneWireActivePullupOn();
    
    /* Overdrive reset */
    drvOneWireReset(1);
    if((c = waitComplete()) != ONEWIRE_STATUS_ERROR) {
        indicateError();
    }
    
    /* Normal reset */
    drvOneWireReset(0);
    if((c = waitComplete()) != ONEWIRE_STATUS_ERROR) {
        indicateError();
    }
    
    /* Simulate PRESENCE detection */
    drv_onewire_context.presence = 1;

    drvOneWireTxBits(0x00, 1);
    if((c = waitComplete()) != ONEWIRE_STATUS_ERROR) {
        indicateError();
    }

    drvOneWireTxBits(0x01, 1);
    if((c = waitComplete()) != ONEWIRE_STATUS_ERROR) {
        indicateError();
    }
    
    drvOneWireTxBits(0x02, 2);
    if((c = waitComplete()) != ONEWIRE_STATUS_ERROR) {
        indicateError();
    }

    drvOneWireTxBits(0x01, 2);
    if((c = waitComplete()) != ONEWIRE_STATUS_ERROR) {
        indicateError();
    }
    
    drvOneWireActivePullupOff();
    
    /* Overdrive presence detection */
    drvOneWireReset(1);
    if((c = waitComplete()) == ONEWIRE_STATUS_ERROR) {
        /* No overdrive-compatible devices, check normal presence pulse */
        drvOneWireReset(0);
        if((c = waitComplete()) == ONEWIRE_STATUS_ERROR) {
            /* No presence pulse detected */
            indicateError();
        }
    }
#endif /* defined(__DRV_ONEWIRE_ACTIVE_PULLUP) */    
    
    /* Green and blue leds ON - all tests successfuly complete */
    ledBlueOn();

    while(1) {
        waitComplete();
    }
    
    return 0;
}

static void indicateError() {
    /* Assert pullup OFF */
    drvOneWireActivePullupOff();
    
    /* Green led OFF, blue led ON - indicate error */
    ledGreenOff();
    ledBlueOn();
    
    while(1) {
        waitComplete();
    }
}

static int waitComplete() {
    int c;
    
    while(ONEWIRE_STATUS_PROGRESS == (c = drvOneWireStatus())) {
        vosIdle();
    }
    
    return c;
}

