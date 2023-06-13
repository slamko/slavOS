#ifndef PIT_H
#define PIT_H

#include "drivers/int.h"

int pit_init(unsigned long freq);

void sleep_ms(unsigned long delay);

void pit_handler(struct isr_handler_args);
#endif