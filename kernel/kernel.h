#ifndef __KERNEL_H
#define __KERNEL_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// hardware
#include   "GIC.h"
#include "PL011.h"
#include "SP804.h"
#include "disk.h"

// kernel "modules"
#include "interrupt.h"
#include "terms.h"
#include "mqueue.h"
#include "fs.h"

// static user progs
#include "init.h"
#include "P0.h"
#include "P1.h"
#include "P2.h"
#include "het.h"
#include "ping.h"
#include "pong.h"
#include "MESSAGE.h"
#include "CABB.h"

#define PROCESS_LIMIT 8 // limit on number of processes running at once

typedef int pid_t;

typedef struct {
  uint32_t cpsr, pc, gpr[ 13 ], sp, lr;
} ctx_t;

typedef enum { 
  TERMINATED,
  CREATED,
  READY,
  EXECUTING,
  WAITING
} pst_t; // process state

typedef struct {
  pid_t pid;
  pid_t prt; // parent pid (UNUSED)
  ctx_t ctx;

  // process state
  pst_t pst; 

  // priority
  uint32_t defp; // default priority
  uint32_t prio; // minor priority

  // file descriptor table (holds file descriptions)
  ofile_t *fd[ FDT_LIMIT ];
} pcb_t;

// === PROCESS + SIGNAL FUNCTIONS ===
uint32_t fork( ctx_t* ctx );
int getProgramEntry( char *path );
int exec( ctx_t* ctx, char *path );
void kill( pid_t pid, sig_t sig );

// === SCHEDULER + READY QUEUE FUNCTIONS ===
int cmp_pcb( const void* p1, const void* p2 );
void rq_rotate();
void rq_add( pid_t pid );
void rq_rm( pid_t pid );
void scheduler( ctx_t* ctx );

#endif
