#ifndef __INIT_H
#define __INIT_H

#include <stddef.h>
#include <stdint.h>

#include <string.h> // for command interpretation
#include <stdlib.h> // string to in

#include "libc.h"

// define symbols for usr entry point and top of stack
extern void (*entry_init)(); 
extern uint32_t tos_init;

#endif
