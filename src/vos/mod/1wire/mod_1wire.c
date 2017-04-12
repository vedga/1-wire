#include <vos/mod/mod_1wire.h>     
#include <vos/drv/onewire.h>

#define OP_READ_POWER_SUPPLY    0xB4

#define OVERDRIVE()                                                     \
    drv_onewire_context.overdrive
        
#define PRESENCE_DETECTED()                                             \
    drv_onewire_context.presence

#define PARASITE_POWER                                                  \
    drv_onewire_context.parasite
        
#define TXBYTE(_v)                                                      \
    drvOneWireTxBits((_v), 8)

#define RXBITS(_n)                                                      \
    drvOneWireTxBits(~0,(_n))
        
#define TASK_CONTEXT            _pt

#define WAIT_IO_COMPLETE()                                              \
    PT_WAIT_WHILE(TASK_CONTEXT, ONEWIRE_STATUS_PROGRESS == (wireState = drvOneWireStatus()))

#define IO_SUCCESS()                                                    \
    (ONEWIRE_STATUS_COMPLETE == wireState)
        
PT_THREAD(ptOneWireProbeBus(struct pt * _pt)) {
    uint8_t wireState;
    
    PT_BEGIN(TASK_CONTEXT);
    
    /* Parasite power not detected */
    PARASITE_POWER = 0;
    
    /* Try overdrive procedure first */
    if(drvOneWireReset(1)) {
        WAIT_IO_COMPLETE();
        
        if(!IO_SUCCESS() || !PRESENCE_DETECTED()) {
            /* Overdrive RESET procedure failed */
            if(drvOneWireReset(0)) {
                WAIT_IO_COMPLETE();
        
                if(!IO_SUCCESS() || !PRESENCE_DETECTED()) {
                    /* No devices on the bus */
                    PT_EXIT(TASK_CONTEXT);
                }
            } else {
                /* Hardware BUSY */
                PT_EXIT(TASK_CONTEXT);
            }
        }
    } else {
        /* Hardware BUSY */
        PT_EXIT(TASK_CONTEXT);
    }
    
    if(TXBYTE(OP_READ_POWER_SUPPLY)) {
        WAIT_IO_COMPLETE();
    } else {
        wireState = ONEWIRE_STATUS_ERROR;
    }
    
    if(IO_SUCCESS()) {
        /* Read one bit after command */
        if(RXBITS(1)) {
            WAIT_IO_COMPLETE();
        } else {
            wireState = ONEWIRE_STATUS_ERROR;
        }
    
        if(IO_SUCCESS()) {
            /* Fetch bit value */
            int16_t value = drvOneWireRxBits(1);
            
            if(value < 0) {
                /* Rx bit decode failed */
            } else {
                /* If any device sent "0" then it used parasite power */
                PARASITE_POWER = value ? 0 : 1;
            }
        }
    }
    
    PT_END(TASK_CONTEXT);
}

uint8_t modOneWireUpdateCRC(uint8_t _crc, uint8_t _data) {
    uint8_t i;

    _crc = _crc ^ _data;
    
    for(i = 0; i < 8; i++) {
        if(_crc & 0x01) {
           _crc = (_crc >> 1) ^ 0x8C;
        }
        else {
           _crc >>= 1;
        }
    }

    return _crc;
}
