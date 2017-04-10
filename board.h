#ifndef __BOARD_H
#define __BOARD_H

#include <bit/platform.h>
#include <res/stm8/res_chip.h>

#define LED_GREEN_IDR_BIT       PE_IDR_IDR7
#define LED_GREEN_DDR_BIT       PE_DDR_DDR7
#define LED_GREEN_ODR_BIT       PE_ODR_ODR7
#define LED_GREEN_CR1_BIT       PE_CR1_C17
#define LED_GREEN_CR2_BIT       PE_CR2_C27

#define LED_BLUE_IDR_BIT        PC_IDR_IDR7
#define LED_BLUE_DDR_BIT        PC_DDR_DDR7
#define LED_BLUE_ODR_BIT        PC_ODR_ODR7
#define LED_BLUE_CR1_BIT        PC_CR1_C17
#define LED_BLUE_CR2_BIT        PC_CR2_C27

#define BUTTON_IDR_BIT          PC_IDR_IDR1
#define BUTTON_DDR_BIT          PC_DDR_DDR1
#define BUTTON_ODR_BIT          PC_ODR_ODR1
#define BUTTON_CR1_BIT          PC_CR1_C11
#define BUTTON_CR2_BIT          PC_CR2_C21
#define BUTTON_IRQ_CR1_BITS     EXTI_CR1_P1IS
#define BUTTON_IRQ_SR1_BIT      EXTI_SR1_P1F

#define IDD_WAKEUP_IDR_BIT      PE_IDR_IDR6
#define IDD_WAKEUP_DDR_BIT      PE_DDR_DDR6
#define IDD_WAKEUP_ODR_BIT      PE_ODR_ODR6
#define IDD_WAKEUP_CR1_BIT      PE_CR1_C16
#define IDD_WAKEUP_CR2_BIT      PE_CR2_C26
#define IDD_WAKEUP_IRQ_CR2_BITS EXTI_CR2_P6IS
#define IDD_WAKEUP_IRQ_SR1_BIT  EXTI_SR1_P6F

#define IDD_CNT_EN_IDR_BIT      PC_IDR_IDR4
#define IDD_CNT_EN_DDR_BIT      PC_DDR_DDR4
#define IDD_CNT_EN_ODR_BIT      PC_ODR_ODR4
#define IDD_CNT_EN_CR1_BIT      PC_CR1_C14
#define IDD_CNT_EN_CR2_BIT      PC_CR2_C24

__INLINE void initBoard(void) {
  /* Boot ROM not need now, disable it clock */
  CLK_PCKENR2_PCKEN27 = 0;
  
  /* Output push/pull fast mode, high level */
  IDD_CNT_EN_ODR_BIT = 1;
  IDD_CNT_EN_DDR_BIT = 1;
  IDD_CNT_EN_CR1_BIT = 1;
  IDD_CNT_EN_CR2_BIT = 1;

  /* Floating input w/o pullup and IRQ */
  IDD_WAKEUP_ODR_BIT = 0;
  IDD_WAKEUP_DDR_BIT = 0;
  IDD_WAKEUP_CR1_BIT = 0;
  IDD_WAKEUP_CR2_BIT = 1;
  
  /* Rising edge interrupt */
  IDD_WAKEUP_IRQ_CR2_BITS = 1;
  
  /* Output push/pull fast mode, low level */
  LED_GREEN_ODR_BIT = 0;
  LED_GREEN_DDR_BIT = 1;
  LED_GREEN_CR1_BIT = 1;
  LED_GREEN_CR2_BIT = 1;

  /* Output push/pull fast mode, low level */
  LED_BLUE_ODR_BIT = 0;
  LED_BLUE_DDR_BIT = 1;
  LED_BLUE_CR1_BIT = 1;
  LED_BLUE_CR2_BIT = 1;

  /* Floating input with interrupt */
  BUTTON_ODR_BIT = 0;
  BUTTON_DDR_BIT = 0;
  BUTTON_CR1_BIT = 0;
  BUTTON_CR2_BIT = 1;
  
  /* Push and pull IRQs */
  BUTTON_IRQ_CR1_BITS = 3;
}

__INLINE void ledGreenOn() {
  LED_GREEN_ODR_BIT = 1;
}

__INLINE void ledGreenOff() {
  LED_GREEN_ODR_BIT = 0;
}

__INLINE void ledGreenToggle() {
  LED_GREEN_ODR_BIT = !LED_GREEN_ODR_BIT;
}

__INLINE void ledBlueOn() {
  LED_BLUE_ODR_BIT = 1;
}

__INLINE void ledBlueOff() {
  LED_BLUE_ODR_BIT = 0;
}

__INLINE void ledBlueToggle() {
  LED_BLUE_ODR_BIT = !LED_BLUE_ODR_BIT;
}

#endif
