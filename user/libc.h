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

// filesystem functions
int fopen( const char *path );

// =========================
// === HELPFUL FUNCTIONS ===
// =========================

char* int2str( int value, char* str, int base );
void write_int( int fd, int x );

#endif
