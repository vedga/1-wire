#if !defined(__MOD_1WIRE_H)
#define __MOD_1WIRE_H

#include <bit/platform.h>
#include <pt/pt.h>

/* 1-wire unique id */
typedef struct {
    /* Device type */
    uint8_t familyCode;
    /* S/N */
    uint8_t sn[6];
    /* CRC (calculated by first 7 bytes) */
    /* uint8_t crc; */
} ds_romid_t;

/* 1-wire bus search task context */
typedef struct {
    /* prototheads task context */
    struct pt pt;
    
    // global search state
    struct {
        /* Device identifier w/o CRC */
        ds_romid_t id;
        /* @TODO: CRC can be calculated, not stored (optimize memory usage) */
        uint8_t crc;
    } romid;
    uint8_t lastDiscrepancy; // size same as last_zero
    uint8_t lastFamilyDiscrepancy; // size same as last_zero
    uint8_t lastDeviceFlag; // size 1 bit
    
    // private task variables
    uint8_t id_bit_number; // max value 65 (size 7 bits)
    uint8_t last_zero; // same id_bit_number, max value 65? (size 7 bits)
    uint8_t rom_byte_number; // max value 8 (size 4 bits)
    uint8_t rom_byte_mask; // size 8 bit
} pt_onewire_search_context_t;

__EXTERN PT_THREAD(ptOneWireProbeBus(struct pt * _pt, struct pt * _nested));
__EXTERN PT_THREAD(ptOneWireTargetAll(struct pt * _pt));
__EXTERN bool_t modOneWireReset();
__EXTERN uint8_t modOneWireUpdateCRC(uint8_t _crc, uint8_t _data);


__EXTERN void ptOneWireInitWalkROM(pt_onewire_search_context_t * _ctx);
__EXTERN PT_THREAD(ptOneWireWalkROM(pt_onewire_search_context_t * _ctx));

#endif
