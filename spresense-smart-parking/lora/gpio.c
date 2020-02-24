#include "gpio.h"
#include <arch/board/board.h>
#include <arch/chip/pin.h>
#include <debug.h>
#include <nuttx/arch.h>
#include <nuttx/config.h>
#include <sdk/config.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define DELAY_CORRECTION (700)
#define DELAY_INTERVAL (50)

void inline digitalWrite(int pin, int level) {
  printf("%s Setting pin %d to %d\n", __func__, pin, level);
  board_gpio_write(pin, level);
}

void inline pinMode(int pin, bool mode) {

  printf("%s Setting pin %d to %d\n", __func__, pin, mode);
  board_gpio_write(pin, -1);
  board_gpio_config(pin, 0, mode, true, PIN_PULLUP);
}

uint64_t micros(void) {
  struct timespec tp;

  /* Wait until RTC is available */
  while (g_rtc_enabled == false)
    ;

  if (clock_gettime(CLOCK_MONOTONIC, &tp)) {
    return 0;
  }

  return (((uint64_t)tp.tv_sec) * 1000000 + tp.tv_nsec / 1000);
}

void delayMicroseconds(unsigned int us) { usleep(us); }

void delay(unsigned long ms) { usleep(ms * 1000); }

static void attach_interrupt(uint8_t pin, void (*isr)(void), int mode) {
  int _mode;
  bool filter = true; // always enable noise filter

  switch (mode) {
  case LOW:
    _mode = INT_LOW_LEVEL;
    break;
  case HIGH:
    _mode = INT_HIGH_LEVEL;
    break;
  case CHANGE:
    _mode = INT_BOTH_EDGE;
    break;
  case RISING:
    _mode = INT_RISING_EDGE;
    break;
  case FALLING:
    _mode = INT_FALLING_EDGE;
    break;
  default:
    printf("ERROR: unknown interrupt mode [%d]\n", mode);
    return;
  }

  int irq = board_gpio_intconfig(pin, _mode, filter, (xcpt_t)isr);
  if (irq < 0) {
    printf("ERROR: Out of interrupt resources\n");
    return;
  }

  /* wait RTC few cycles beforparking_data_state_tostre the interrupt is enabled for noise filter. */
  delay(1);
  board_gpio_int(pin, true);
}

void attachInterrupt(uint8_t interrupt, void (*isr)(void), int mode) {
  // uint8_t _pin = pin_convert(interrupt);
  uint8_t _pin = (interrupt);
  if (_pin == PIN_NOT_ASSIGNED)
    return;
  attach_interrupt(_pin, isr, mode);
}

void detachInterrupt(uint8_t interrupt) {}

static void __empty() {
  // Empty
}

void yield(void) __attribute__((weak, alias("__empty")));
