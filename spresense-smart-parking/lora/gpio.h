#pragma once

#include "../common.h"
#include <arch/board/board.h>
#include <arch/chip/pin.h>
#include <debug.h>

EXTERN_C

#define INPUT 1
#define OUTPUT 0

#define HIGH 1
#define LOW 0
#define CHANGE 0x2
#define RISING 0x3
#define FALLING 0x4
#define PIN_NOT_ASSIGNED (0xFF)

#define digitalPinToInterrupt(p) (p)
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))

void digitalWrite(int pin, int level);
void pinMode(int pin, bool mode);

void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);

void yield(void); // __attribute__ ((weak, alias("__empty")));

void attachInterrupt(uint8_t interrupt, void (*isr)(void), int mode);
void detachInterrupt(uint8_t interrupt);

EXTERN_C_END
