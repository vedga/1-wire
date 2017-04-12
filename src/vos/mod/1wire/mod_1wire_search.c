/*
 * Пример обнаружения всех устройств на шине 1-wire
 * 
 * pt_1wire_search_context_t ctxSearch;
 * 
 * ...
 * 
 * ptOneWireInitWalkROM(&ctxSearch);
 * uint8_t c;
 * do {
 *     PT_WAIT_UNTIL(&(VOS_CURRENT_TASK()->pt), 
 *                   ((c = ptOneWireWalkROM(&ctxSearch)) > PT_WAITING));
 * } while(c == PT_YIELDED);
 */
     
#include <vos/mod/mod_1wire.h>     
#include <vos/drv/onewire.h>

     
#define OP_SEARCH_ROM           0xF0
     
#define OVERDRIVE()                                                     \
    drv_onewire_context.overdrive
        
#define PRESENCE_DETECTED()                                             \
    drv_onewire_context.presence

        
#define TXBYTE(_v)                                                      \
    drvOneWireTxBits((_v), 8)
        
#define RXBITS(_n)                                                      \
    drvOneWireTxBits(~0,(_n))
        
        
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
        
#define RX_VALUE                wireState        
        
#define WAIT_IO_COMPLETE()                                              \
    PT_WAIT_WHILE(TASK_CONTEXT, ONEWIRE_STATUS_PROGRESS == (wireState = drvOneWireStatus()))

#define IO_SUCCESS()                                                    \
    (ONEWIRE_STATUS_COMPLETE == wireState)



PT_THREAD(ptOneWireWalkROM(pt_onewire_search_context_t * _ctx)) {
    PT_BEGIN(TASK_CONTEXT);
    
    while(!LAST_DEVICE_FLAG) {
        int16_t wireState;
        
        // initialize for search
        ID_BIT_NUMBER = 1;
        LAST_ZERO = 0;
        ROM_BYTE_NUMBER = 0;
        ROM_BYTE_MASK = 1;
        
        // 1-Wire reset (в зависимости от используемого режима)
        if(drvOneWireReset(OVERDRIVE())) {
            /* Ожидаем завершение операции сброса */
            WAIT_IO_COMPLETE();
        } else {
            wireState = ONEWIRE_STATUS_ERROR;
        }
        
        if(!IO_SUCCESS() || !PRESENCE_DETECTED()) {
            // reset the search
            LAST_DISCREPANCY = 0;
            LAST_DEVICE_FLAG = 0;
            LAST_FAMILY_DISCREPANCY = 0;

            /* Если presence не обнаружен, то на шине вообще нет устройств! */
            PT_EXIT(TASK_CONTEXT);
        }

        // issue the search command 
        if(TXBYTE(OP_SEARCH_ROM)) {
            WAIT_IO_COMPLETE();
            
            if(!IO_SUCCESS()) {
                /* Send command error, repeat procedure from RESET point */
                continue;
            }
            
            if(OP_SEARCH_ROM != drvOneWireRxBits(8)) {
                /* Data corrupted */
                continue;
            }
        } else {
            /* Command not initiated, repeat procedure from RESET point */
            continue;
        }
        
        // loop to do the search
        do {
            // read a bit and its complement
            if(RXBITS(2)) {
                WAIT_IO_COMPLETE();
            } else {
                wireState = ONEWIRE_STATUS_ERROR;
            }

            if(!IO_SUCCESS()) {
                /* Ошибка приема 2-х битов после команды поиска.
                 * Имеем ctx_id_bit_number меньше 65, поэтому процедура
                 * будет повторена с точки, соответствующей входу в
                 * эту задачу.
                 */
                break;
            }

            if((RX_VALUE = drvOneWireRxBits(2)) < 0) {
                __no_operation();
                
                break;
            }
            
                __no_operation();
            
            uint8_t id_bit = (RX_VALUE & 0x01) ? 1 : 0;
            uint8_t cmp_id_bit = (RX_VALUE & 0x02) ? 1 : 0;

            uint8_t search_direction;
            
            // check for no devices on 1-wire
            if ((id_bit == 1) && (cmp_id_bit == 1)) {
                /* Биты не инверсные, соответствуют отсутствию сигналов на шине */
                break;
            } else {
                // all devices coupled have 0 or 1
                if (id_bit != cmp_id_bit) {
                    search_direction = id_bit;  // bit write value for search
                } else {
                    // if this discrepancy if before the Last Discrepancy
                    // on a previous next then pick the same as last time
                    if (ID_BIT_NUMBER < LAST_DISCREPANCY) {
                        search_direction = (ROMID_BYTE_REF(ROM_BYTE_NUMBER) & ROM_BYTE_MASK) ? 1 : 0;
                    } else {
                        // if equal to last pick 1, if not then pick 0
                        search_direction = (ID_BIT_NUMBER == LAST_DISCREPANCY) ? 1 : 0;
                    }

                    // if 0 was picked then record its position in LastZero
                    if (search_direction == 0) {
                        LAST_ZERO = ID_BIT_NUMBER;
                    }

                    // check for Last discrepancy in family
                    if (LAST_ZERO < 9) {
                        LAST_FAMILY_DISCREPANCY = LAST_ZERO;
                    }
                    
                }
            }

            // set or clear the bit in the ROM byte rom_byte_number
            // with mask rom_byte_mask
            if (search_direction == 1) {
                ROMID_BYTE_REF(ROM_BYTE_NUMBER) |= ROM_BYTE_MASK;
            } else {
                ROMID_BYTE_REF(ROM_BYTE_NUMBER) &= ~ROM_BYTE_MASK;
            }

            // serial number search direction write bit
            if(drvOneWireTxBits(search_direction, 1)) {
                WAIT_IO_COMPLETE();
            } else {
                wireState = ONEWIRE_STATUS_ERROR;
            }
            
            /* search_direction not stored, therefore we can't check echo */

            if(!IO_SUCCESS()) {
                /* Ошибка передачи бита direction.
                 * Имеем ctx_id_bit_number меньше 65, поэтому процедура
                 * будет повторена с точки, соответствующей входу в
                 * эту задачу.
                 */
                break;
            }

            // increment the byte counter id_bit_number
            // and shift the mask rom_byte_mask
            ID_BIT_NUMBER++;
            ROM_BYTE_MASK <<= 1;

            // if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
            if (ROM_BYTE_MASK == 0) {
                ROM_BYTE_NUMBER++;
                ROM_BYTE_MASK = 1;
            }
        } while(ROM_BYTE_NUMBER < 8);  // loop until through all ROM bytes 0-7

        // if the search was successful then
        if(!(ID_BIT_NUMBER < 65)) {
            uint8_t i;
            
            /* Выполняем проверку CRC */
            uint8_t crc = 0;
            for(i = 0;i < sizeof(ROMID);i++) {
                crc = modOneWireUpdateCRC(crc, ROMID_BYTE_REF(i));
            }
            
            if(crc) {
                /* Ошибка CRC.
                 * Повторим процедуру с той точки, с которой мы ее начинали 
                 */
                continue;
            }
            
            // search successful so set ctx_last_discrepancy,ctx_last_device_flag,search_result
            LAST_DISCREPANCY = LAST_ZERO;

            // check for last device
            if (LAST_DISCREPANCY == 0) {
                LAST_DEVICE_FLAG = 1;
            }

            /* Есть очередная порция результатов */
        } else {
            /* Ошибка обмена по интерфейсу.
             * Повторим процедуру с той точки, с которой мы ее начинали 
             */
            continue;
        }
        
        if(!ROMID.id.familyCode) {
            /* familyCode не должен быть равен 0! */
            break;
        }
        
        /* Возвращаем очередную порцию результатов */
        PT_YIELD(TASK_CONTEXT);
    }

    /* Подготовка к следующему начальному запуску операции сканирования */
    ptOneWireInitWalkROM(CONTEXT);
    
    PT_END(TASK_CONTEXT);
}

void ptOneWireInitWalkROM(pt_onewire_search_context_t * _ctx) {
    /* Подготовка к начальному запуску операции сканирования */
    LAST_DISCREPANCY = 0;
    LAST_DEVICE_FLAG = 0;
    LAST_FAMILY_DISCREPANCY = 0;

    /* Инициализируем структуру для protothreads */
    PT_INIT(TASK_CONTEXT);
}
     
     
#if 0

#include <drv_1wire.h>

#define OP_SEARCH_ROM   0xF0

#define ctx_romid                       _ctx->romid
#define ctx_last_discrepancy            _ctx->lastDiscrepancy
#define ctx_last_family_discrepancy     _ctx->lastFamilyDiscrepancy
#define ctx_last_device_flag            _ctx->lastDeviceFlag

#define ctx_romid_byte(_n)              (*((uint8_t *)&(ctx_romid) + (_n)))

#define ctx_id_bit_number               _ctx->id_bit_number
#define ctx_last_zero                   _ctx->last_zero
#define ctx_rom_byte_number             _ctx->rom_byte_number
#define ctx_rom_byte_mask               _ctx->rom_byte_mask

#define rx_value                        ctx1WireDriver.param.io.value

#define WAIT_RESET_COMPLETE()                                               \
    PT_WAIT_UNTIL(&_ctx->pt, (wireState = drv1WireState()) & DRV_1WIRE_STATE_RESET_COMPLETE)

#define WAIT_IO_COMPLETE()                                                  \
    PT_WAIT_UNTIL(&_ctx->pt, (wireState = drv1WireState()) & DRV_1WIRE_STATE_IO_COMPLETE)

/**
 * Реализация операции поиска устройств на шине 1-wire
 * 
 * @param _ctx
 * @return - статус задачи (см. ниже)
 * 
 * Исходная реализация:
 * https://www.maximintegrated.com/en/app-notes/index.mvp/id/187
 * 
 * Если возвращаемое значение равно PT_YIELDED, то на шине обнаружено очередное
 * устройство, идентификатор которого находится в массиве _ctx->romid.
 * Если возвращаемое значение равно PT_EXITED (presence не обнаружен) или PT_ENDED,
 * то на шине больше нет устройств.
 * Если возвращаемое значение равно PT_WAITING, то задача в процессе выполнения
 * и должна быть вызвана в очередной раз при возникновении significant event.
 */
PT_THREAD(ptOneWireWalkROM(pt_1wire_search_context_t * _ctx)) {
    PT_BEGIN(&_ctx->pt);

    while(!ctx_last_device_flag) {
        // initialize for search
        ctx_id_bit_number = 1;
        ctx_last_zero = 0;
        ctx_rom_byte_number = 0;
        ctx_rom_byte_mask = 1;
        
        uint8_t wireState;
        
        // 1-Wire reset (в зависимости от используемого режима)
        DRV_1WIRE_RESET();
        
        /* Ожидаем завершение операции сброса */
        WAIT_RESET_COMPLETE();
        
        if(!(wireState & DRV_1WIRE_STATE_PRESENCE_DETECTED)) {
            // reset the search
            ctx_last_discrepancy = 0;
            ctx_last_device_flag = 0;
            ctx_last_family_discrepancy = 0;

            /* Если presence не обнаружен, то на шине вообще нет устройств! */
            PT_EXIT(&_ctx->pt);
        }

        // issue the search command 
        DRV_1WIRE_TXBYTE(OP_SEARCH_ROM, 0);
        WAIT_IO_COMPLETE();

        if(wireState & DRV_1WIRE_STATE_IO_ERROR) {
            /* Ошибка передачи команды - повторим процедуру со сброса */
            continue;
        }
        
        // loop to do the search
        do {
            // read a bit and its complement
            DRV_1WIRE_RXBITS(2);
            WAIT_IO_COMPLETE();

            if(wireState & DRV_1WIRE_STATE_IO_ERROR) {
                /* Ошибка приема 2-х битов после команды поиска.
                 * Имеем ctx_id_bit_number меньше 65, поэтому процедура
                 * будет повторена с точки, соответствующей входу в
                 * эту задачу.
                 */
                break;
            }
            
            uint8_t id_bit = (rx_value & 0b01000000) ? 1 : 0;
            uint8_t cmp_id_bit = (rx_value & 0b10000000) ? 1 : 0;

            uint8_t search_direction;
            
            // check for no devices on 1-wire
            if ((id_bit == 1) && (cmp_id_bit == 1)) {
                /* Биты не инверсные, соответствуют отсутствию сигналов на шине */
                break;
            } else {
                // all devices coupled have 0 or 1
                if (id_bit != cmp_id_bit) {
                    search_direction = id_bit;  // bit write value for search
                } else {
                    // if this discrepancy if before the Last Discrepancy
                    // on a previous next then pick the same as last time
                    if (ctx_id_bit_number < ctx_last_discrepancy) {
                        search_direction = (ctx_romid_byte(ctx_rom_byte_number) & ctx_rom_byte_mask) ? 1 : 0;
                    } else {
                        // if equal to last pick 1, if not then pick 0
                        search_direction = (ctx_id_bit_number == ctx_last_discrepancy) ? 1 : 0;
                    }

                    // if 0 was picked then record its position in LastZero
                    if (search_direction == 0) {
                        ctx_last_zero = ctx_id_bit_number;
                    }

                    // check for Last discrepancy in family
                    if (ctx_last_zero < 9) {
                        ctx_last_family_discrepancy = ctx_last_zero;
                    }
                    
                }
            }

            // set or clear the bit in the ROM byte rom_byte_number
            // with mask rom_byte_mask
            if (search_direction == 1) {
                ctx_romid_byte(ctx_rom_byte_number) |= ctx_rom_byte_mask;
            } else {
                ctx_romid_byte(ctx_rom_byte_number) &= ~ctx_rom_byte_mask;
            }

            // serial number search direction write bit
            DRV_1WIRE_TXBITS(search_direction, 1, 0);
            WAIT_IO_COMPLETE();

            if(wireState & DRV_1WIRE_STATE_IO_ERROR) {
                /* Ошибка передачи бита direction.
                 * Имеем ctx_id_bit_number меньше 65, поэтому процедура
                 * будет повторена с точки, соответствующей входу в
                 * эту задачу.
                 */
                break;
            }

            // increment the byte counter id_bit_number
            // and shift the mask rom_byte_mask
            ctx_id_bit_number++;
            ctx_rom_byte_mask <<= 1;

            // if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
            if (ctx_rom_byte_mask == 0) {
                ctx_rom_byte_number++;
                ctx_rom_byte_mask = 1;
            }
        } while(ctx_rom_byte_number < 8);  // loop until through all ROM bytes 0-7

        // if the search was successful then
        if(!(ctx_id_bit_number < 65)) {
            uint8_t i;
            
            /* Выполняем проверку CRC */
            uint8_t crc = 0;
            for(i = 0;i < sizeof(ctx_romid);i++) {
                crc = mod1WireUpdateCRC(crc, ctx_romid_byte(i));
            }
            
            if(crc) {
                /* Ошибка CRC.
                 * Повторим процедуру с той точки, с которой мы ее начинали 
                 */
                continue;
            }
            
            // search successful so set ctx_last_discrepancy,ctx_last_device_flag,search_result
            ctx_last_discrepancy = ctx_last_zero;

            // check for last device
            if (ctx_last_discrepancy == 0) {
                ctx_last_device_flag = 1;
            }

            /* Есть очередная порция результатов */
        } else {
            /* Ошибка обмена по интерфейсу.
             * Повторим процедуру с той точки, с которой мы ее начинали 
             */
            continue;
        }
        
        if(!ctx_romid.id.familyCode) {
            /* familyCode не должен быть равен 0! */
            break;
        }
        
        /* Возвращаем очередную порцию результатов */
        PT_YIELD(&_ctx->pt);
    }

    /* Подготовка к следующему начальному запуску операции сканирования */
    ptOneWireInitWalkROM(_ctx);
    
    PT_END(&_ctx->pt);
}

/**
 * Подготовка к первому запуску задачи поиска устройств на шине 1-wire
 * 
 * @param _ctx
 */
void ptOneWireInitWalkROM(pt_1wire_search_context_t * _ctx) {
    /* Подготовка к начальному запуску операции сканирования */
    ctx_last_discrepancy = 0;
    ctx_last_device_flag = 0;
    ctx_last_family_discrepancy = 0;

    /* Инициализируем структуру для protothreads */
    PT_INIT(&_ctx->pt);
}

#endif
