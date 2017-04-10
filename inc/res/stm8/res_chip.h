#if !defined(__RES_CHIP_H)
#define __RES_CHIP_H

#include <bit/platform.h>

#define HSE_HZ                  16000000UL
#define LSE_HZ                  32768UL
#define HSI_HZ                  16000000UL
#define LSI_HZ                  38000UL

#define CLK_SOURCE_HSI          0x01
#define CLK_SOURCE_LSI          0x02
#define CLK_SOURCE_HSE          0x04
#define CLK_SOURCE_LSE          0x08

__EXTERN uint32_t getSysClk();
__EXTERN int8_t setSysClkSource(uint8_t _source, uint8_t _prescaler);


__INLINE void pwrTimer1On() {
  CLK_PCKENR2_PCKEN21 = 1;
}

__INLINE void pwrTimer1Off() {
  CLK_PCKENR2_PCKEN21 = 0;
}

__INLINE void pwrTimer2On() {
  CLK_PCKENR1_PCKEN10 = 1;
}

__INLINE void pwrTimer2Off() {
  CLK_PCKENR1_PCKEN10 = 0;
}

__INLINE void pwrTimer3On() {
  CLK_PCKENR1_PCKEN11 = 1;
}

__INLINE void pwrTimer3Off() {
  CLK_PCKENR1_PCKEN11 = 0;
}

#if 0
__INLINE void pwrTimer4On() {
  CLK_PCKENR1_PCKEN12 = 1;
}

__INLINE void pwrTimer4Off() {
  CLK_PCKENR1_PCKEN12 = 0;
}
#endif

#endif
