#include <vos/mod/mod_1wire.h>     
#include <vos/drv/onewire.h>

     
#define OP_SEARCH_ROM           0xF0
     
#define PRESENCE_DETECTED()                                             \
    drv_onewire_context.presence

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
        
#define PT_ONEWIRE_RESET() do {                                         \
    if(modOneWireReset()) {                                             \
        PT_WAIT_IO_COMPLETE(); } else {                                 \
        dummy = ONEWIRE_STATUS_ERROR; } } while(0)
        
        
#define CONTEXT                 _ctx
#define TASK_CONTEXT            (&(CONTEXT->pt))
#define LAST_DISCREPANCY        CONTEXT->lastDiscrepancy
#define LAST_DEVICE_FLAG        CONTEXT->lastDeviceFlag
#define LAST_FAMILY_DISCREPANCY CONTEXT->lastFamilyDiscrepancy

#define ID_BIT_NUMBER           CONTEXT->id_bit_number
#define LAST_ZERO               CONTEXT->last_zero
#define ROM_BYTE_NUMBER         CONTEXT->rom_byte_number
#define ROM_BYTE_MASK           CONTEXT->rom_byte_mask

#define ROMID                   CONTEXT->romid        
        
#define ROMID_BYTE_REF(_n)      (*((uint8_t *)&ROMID + (_n)))
        
#define RX_VALUE                dummy


/*
 * Execute device search procedure
 *
 * Enter:
 * _ctx - search context
 *
 * Leave:
 * Standard protothreads values:
 * PT_WAITING - procedure isn't complete, time slice released, call again later
 * PT_YIELDED - found next device, it's romid located at _ctx->romid array
 * PT_EXITED or PT_ENDED - no more devices at the bus
 *
 * This is
 */            
PT_THREAD(ptOneWireWalkROM(pt_onewire_search_context_t * _ctx)) {
    PT_BEGIN(TASK_CONTEXT);
    
    while(!LAST_DEVICE_FLAG) {
        int16_t dummy;
        
        /* initialize for search */
        ID_BIT_NUMBER = 1;
        LAST_ZERO = 0;
        ROM_BYTE_NUMBER = 0;
        ROM_BYTE_MASK = 1;
        
        /* 1-Wire reset (dependent on OVERDRIVE flag) */
        PT_ONEWIRE_RESET();
        
        if(!IO_SUCCESS() || !PRESENCE_DETECTED()) {
            // reset the search
            LAST_DISCREPANCY = 0;
            LAST_DEVICE_FLAG = 0;
            LAST_FAMILY_DISCREPANCY = 0;

            /* If presence not detected then no devices on the bus */
            PT_EXIT(TASK_CONTEXT);
        }

        /* issue the search command */
        PT_TX_BYTE(OP_SEARCH_ROM);
        
        if(!IO_SUCCESS() || (OP_SEARCH_ROM != drvOneWireRxBits(8))) {
            /* Send command error, repeat procedure from RESET point */
            
            /* Other solution is abort search procedure */
            
            continue;
        }
        
        // loop to do the search
        do {
            // read a bit and its complement
            PT_RX_BITS(2);
            
            if(!IO_SUCCESS()) {
                /* Error while receiving 2 bits.
                 * As ID_BIT_NUMBER less than 65 search procedure
                 * resumed from state such as original task entry.
                 */
                break;
            }

            if((RX_VALUE = drvOneWireRxBits(2)) < 0) {
                __no_operation();
                
                break;
            }
            
            uint8_t id_bit = (RX_VALUE & 0x01) ? 1 : 0;
            uint8_t cmp_id_bit = (RX_VALUE & 0x02) ? 1 : 0;

            uint8_t search_direction;
            
            /* check for no devices on 1-wire */
            if ((id_bit == 1) && (cmp_id_bit == 1)) {
                /* Same bit values equ "1" indicate no devices on the bus */
                break;
            } else {
                /* all devices coupled have 0 or 1 */
                if (id_bit != cmp_id_bit) {
                    search_direction = id_bit;  /* bit write value for search */
                } else {
                    /* if this discrepancy if before the LAST_DISCREPANCY
                     on a previous next then pick the same as last time */
                    if (ID_BIT_NUMBER < LAST_DISCREPANCY) {
                        search_direction = (ROMID_BYTE_REF(ROM_BYTE_NUMBER) & ROM_BYTE_MASK) ? 1 : 0;
                    } else {
                        /* if equal to last pick 1, if not then pick 0 */
                        search_direction = (ID_BIT_NUMBER == LAST_DISCREPANCY) ? 1 : 0;
                    }

                    /* if 0 was picked then record its position in LAST_ZERO */
                    if (search_direction == 0) {
                        LAST_ZERO = ID_BIT_NUMBER;
                    }

                    /* check for LAST_FAMILY_DISCREPANCY in family */
                    if (LAST_ZERO < 9) {
                        LAST_FAMILY_DISCREPANCY = LAST_ZERO;
                    }
                    
                }
            }

            /* set or clear the bit in the ROM byte ROM_BYTE_NUMBER
               with mask rom_byte_mask */
            if (search_direction == 1) {
                ROMID_BYTE_REF(ROM_BYTE_NUMBER) |= ROM_BYTE_MASK;
            } else {
                ROMID_BYTE_REF(ROM_BYTE_NUMBER) &= ~ROM_BYTE_MASK;
            }

            /* serial number search direction write bit */
            PT_TX_BITS(search_direction, 1);
            
            /* search_direction not stored, therefore we can't check echo */

            if(!IO_SUCCESS()) {
                /* Sending direction failed.
                 * As ID_BIT_NUMBER less than 65 search procedure
                 * resumed from state such as original task entry.
                 */
                break;
            }

            /* increment the byte counter ID_BIT_NUMBER
               and shift the mask rom_byte_mask */
            ID_BIT_NUMBER++;
            ROM_BYTE_MASK <<= 1;

            /* if the mask is 0 then go to new SerialNum byte ROM_BYTE_NUMBER and reset mask */
            if (ROM_BYTE_MASK == 0) {
                ROM_BYTE_NUMBER++;
                ROM_BYTE_MASK = 1;
            }
        } while(ROM_BYTE_NUMBER < 8);  /* loop until through all ROM bytes 0-7 */

        /* if the search was successful then */
        if(!(ID_BIT_NUMBER < 65)) {
            uint8_t i;
            
            /* Calculate CRC */
            uint8_t crc = 0;
            for(i = 0;i < sizeof(ROMID);i++) {
                crc = modOneWireUpdateCRC(crc, ROMID_BYTE_REF(i));
            }
            
            if(crc) {
                /* CRC error.
                 * Repeat procedure from original point
                 */
                continue;
            }
            
            /* search successful so set LAST_DISCREPANCY and LAST_DEVICE_FLAG */
            LAST_DISCREPANCY = LAST_ZERO;

            // check for last device
            if (LAST_DISCREPANCY == 0) {
                LAST_DEVICE_FLAG = 1;
            }

            /* Next device detection complete */
        } else {
            /* I/O error.
             * Retry procedure from original point
             */
            continue;
        }
        
        if(!ROMID.id.familyCode) {
            /* familyCode не должен быть равен 0! */
            break;
        }
        
        /* Return detected device S/N */
        PT_YIELD(TASK_CONTEXT);
    }

    /* Reset state for next scan loop (if need) */
    ptOneWireInitWalkROM(CONTEXT);
    
    PT_END(TASK_CONTEXT);
}

/*
 * Initialize device search procedure
 */
void ptOneWireInitWalkROM(pt_onewire_search_context_t * _ctx) {
    /* Prepare ptOneWireWalkROM() for first call  */
    LAST_DISCREPANCY = 0;
    LAST_DEVICE_FLAG = 0;
    LAST_FAMILY_DISCREPANCY = 0;

    /* Initialize protothreads data */
    PT_INIT(TASK_CONTEXT);
}
