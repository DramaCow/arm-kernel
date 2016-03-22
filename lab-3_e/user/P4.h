#ifndef __P4_H
#define __P4_H

#include <stddef.h>
#include <stdint.h>

#include "libc.h"

// define symbols for P1 entry point and top of stack
extern void (*entry_P4)(); 
extern uint32_t tos_P4;

#endif
