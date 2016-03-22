#ifndef __LIBC_H
#define __LIBC_H

#include <stddef.h>
#include <stdint.h>

#include "signal.h"

// cooperatively yield control of processor, i.e., invoke the scheduler
void yield();

// POSIX fork : page 882
int cfork();

// exit
void cexit();

// POSIX exec : page 772?
void cexec( int p );

// POSIX kill
void ckill( int pid, sig_t sig );

// POSIX raise
void raise( sig_t sig );

// POSIXish
int mqinit( int name ); // need to add mqd to processes list of open mqueues
void msgsend( int mqd, void* buf );
void msgreceive( int mqd, void* buf );

// write n bytes from x to the file descriptor fd
int write( int fd, void* x, size_t n );
int read( int fd, void* x, size_t n );

// writes/reads n bytes from x to address a
int dwrite( int a, void* x, size_t n);
int dread( int a, void* x, size_t n);

// =========================
// === HELPFUL FUNCTIONS ===
// =========================

char* int2str( int value, char* str, int base );

#endif
