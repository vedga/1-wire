#include <vos/mod/mod_1wire.h>     
#include <vos/drv/onewire.h>

#define OP_READ_POWER_SUPPLY    0xB4
#define OP_SKIP_ROM             0xCC

#define OVERDRIVE()                                                     \
    drv_onewire_context.overdrive
        
#define PRESENCE_DETECTED()                                             \
    drv_onewire_context.presence

#define PARASITE_POWER                                                  \
    drv_onewire_context.parasite
        
#define STATUS                                                          \
    drv_onewire_context.status
        
#define PT_WAIT_IO_COMPLETE()                                           \
    PT_WAIT_WHILE(TASK_CONTEXT, ONEWIRE_STATUS_PROGRESS == (dummy = drvOneWireStatus()))

#define IO_SUCCESS()                                                    \
    (ONEWIRE_STATUS_COMPLETE == dummy)

#define PT_TX_BITS(_v,_n) do {                                          \
    if(drvOneWireTxBits((_v),(_n))) {                                   \
        PT_WAIT_IO_COMPLETE(); } else {                                 \
        dummy = ONEWIRE_STATUS_ERROR; } } while(0)
        
#define PT_TX_BYTE(_v)                                                  \
    PT_TX_BITS((_v), 8)
        
#define PT_RX_BITS(_n)                                                  \
    PT_TX_BITS(~0,(_n))
        
#define PT_TX_BYTE_CONST(_v) do {                                       \
    PT_TX_BYTE((_v));                                                   \
    if(!IO_SUCCESS() || ((_v) != drvOneWireRxBits(8))) {                \
        STATUS = ONEWIRE_STATUS_ERROR; } } while(0)
        
#define TASK_CONTEXT            _pt

/*
 * Probe 1-Wire bus and determine it's capabilities
 *
 * First try RESET procedure at OVERDRIVE mode. If we got PRESENCE
 * pulse, than some bus devices support OVERDRIVE and we continue
 * operate at this mode.
 * If OVERDRIVE not supported then try RESET with normal timing. If we
 * got PRESENCE then some devices attached to the bus.
 * Then if PRESENCE pulse detected we select all devices, send
 * POWER_SUPPLY command and read result. If one or mode devices use
 * parasite power then we set appropriate flag. It can be used for
 * activate pullup on measurement command.
 */
PT_THREAD(ptOneWireProbeBus(struct pt * _pt, struct pt * _nested)) {
    uint8_t dummy;
    
    PT_BEGIN(TASK_CONTEXT);
    
    /* Parasite power not detected */
    PARASITE_POWER = 0;
    
    /* Try overdrive procedure first */
    if(drvOneWireReset(1)) {
        PT_WAIT_IO_COMPLETE();
        
        if(!IO_SUCCESS() || !PRESENCE_DETECTED()) {
            /* Overdrive RESET procedure failed */
            if(drvOneWireReset(0)) {
                PT_WAIT_IO_COMPLETE();
        
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
    
    PT_SPAWN(TASK_CONTEXT, _nested, ptOneWireTargetAll(_nested));

    if(ONEWIRE_STATUS_COMPLETE == STATUS) {
        PT_TX_BYTE_CONST(OP_READ_POWER_SUPPLY);

        if(IO_SUCCESS()) {
            /* Read one bit after command */
            PT_RX_BITS(1);
    
            if(IO_SUCCESS()) {
                /* Fetch bit value */
                int16_t value = drvOneWireRxBits(1);
            
                if(value < 0) {
                    /* Rx bit decode failed */
                    STATUS = ONEWIRE_STATUS_ERROR;
                } else {
                    /* If any device sent "0" then it used parasite power */
                    PARASITE_POWER = value ? 0 : 1;
                }
            }
        }
    }
    
    PT_END(TASK_CONTEXT);
}

/*
 * Send SKIP ROM command
 */
PT_THREAD(ptOneWireTargetAll(struct pt * _pt)) {
    uint8_t dummy;
    
    PT_BEGIN(TASK_CONTEXT);

    PT_TX_BYTE_CONST(OP_SKIP_ROM);
    
    PT_END(TASK_CONTEXT);
}

/*
 * Initiate RESET procedure with current OVERDRIVE settings
 */
bool_t modOneWireReset() {
    return drvOneWireReset(OVERDRIVE());
}

/*
 * Update CRC
 */
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
