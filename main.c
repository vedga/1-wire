#include <string.h>

#include "board.h"
#include <vos/drv/onewire.h>

#include <vos/kernel.h>

int c;
    
int main( void )
{
    initBoard();
  
//  setSysClkSource(CLK_SOURCE_LSI, 0); //OK
    setSysClkSource(CLK_SOURCE_HSI, 0); //OK
//  setSysClkSource(CLK_SOURCE_LSE, 0); //OK
//  setSysClkSource(CLK_SOURCE_HSE, 0); //FAIL

    while(CLK_SWCR_SWBSY) ;
    
    drvOneWireAttach();
  
    ledGreenOn();

#if 1
    drvOneWireReset(1);
  
    while(ONEWIRE_STATUS_PROGRESS == (c = drvOneWireStatus())) {
        vosIdle();
    }
    
    drvOneWireReset(0);

    while(ONEWIRE_STATUS_PROGRESS == (c = drvOneWireStatus())) {
        vosIdle();
    }
#else
    drv_onewire_context.presence = 1;
    drv_onewire_context.overdrive = 0;
#endif    
    
    drvOneWireActivePullupOn();
//    drvOneWireActivePullupOff();
//    drvOneWireActivePullupOn();
//    drvOneWireActivePullupOff();

    drvOneWireTxBits(0xFE, 2);
//    drvOneWireTxBits(0xF1, 2);
//    drvOneWireTxBits(0xAA, 8); // == 170
//    drvOneWireTxBits(0xF0, 8); 
//    drvOneWireTxBits(0x0F, 8); 
//    drvOneWireTxBits(0xFF, 1); 
//    drvOneWireTxBits(0x00, 1); 
    
    while(ONEWIRE_STATUS_PROGRESS == (c = drvOneWireStatus())) {
        vosIdle();
    }

    // c must be ONEWIRE_STATUS_ERROR if compiled with Active-pullup,
    // else c must be ONEWIRE_STATUS_COMPLETE.
    
    drvOneWireActivePullupOff();

    drvOneWireTxBits(0xFE, 2);
    
    while(ONEWIRE_STATUS_PROGRESS == (c = drvOneWireStatus())) {
        vosIdle();
    }

    // c must be ONEWIRE_STATUS_COMPLETE.
    
    c = drvOneWireRxBits(2);
    
    // c must be (0xFE & 0x03) == 0x03
    
//    c = drvOneWireRxBits(8);
//    c = drvOneWireRxBits(1);

    drvOneWireTxBits(0xAA, 8);

    while(ONEWIRE_STATUS_PROGRESS == (c = drvOneWireStatus())) {
        vosIdle();
    }

    // c must be ONEWIRE_STATUS_COMPLETE.
    
    c = drvOneWireRxBits(8);

    // c must be 0xAA
    
    drvOneWireTxBits(0x55, 8);

    while(ONEWIRE_STATUS_PROGRESS == (c = drvOneWireStatus())) {
        vosIdle();
    }

    // c must be ONEWIRE_STATUS_COMPLETE.
    
    c = drvOneWireRxBits(8);
  
    // c must be 0x55
    
    return 0;
}
