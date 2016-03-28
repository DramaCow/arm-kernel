#include "libc.h"

void yield() {
  asm volatile( "svc #0 \n" );
}

int cfork( uint32_t program ) {
  int f;
  asm volatile( "mov r0, %1 \n" 
                "svc #3     \n"
                "mov %0, r0 \n"
              : "=r" (f)        
              : "r" (program)
              : "r0"            );
  return f;
}

void cexit( void ) { // make this a raise sigkill call!
  asm volatile( "svc #4 \n"  );
}

void ckill( int pid, sig_t sig ) { // TODO: add more signals
  asm volatile( "mov r2, %0 \n"
                "mov r3, %1 \n"
                "svc #6     \n"
              :
              : "r" (pid), "r" (sig) );
}

void craise( sig_t sig ) {
  asm volatile( "mov r2, %0 \n"
                "svc #7     \n" 
                :
                : "r" (sig)     );
}

int mqinit( int mqd ) {
  int m;
  asm volatile( "mov r2, %0 \n"
                "svc #8     \n"
                "mov %0, r0 \n"
              : "=r" (m)
              : "r" (mqd)         );
  return m;
}

void msgsend( int mqd, void* buf ) {
  int m = 0;
  do {
    asm volatile( "mov r2, %1 \n"
                  "mov r3, %2 \n"
                  "svc #9     \n"
                  "mov %0, r0 \n"
                : "=r" (m)
                : "r" (mqd), "r" (buf) );

    if (m == -1) {
      yield();
      asm volatile( "mov  %0, r0 \n"
                    "mov  %1, r1 \n"
                    "mov  %2, r2 \n" 
                  : 
                  : "r" (m), "r" (mqd), "r" (buf) );  
    }
  } while (m == -1);
  return;
}

void msgreceive( int mqd, void* buf ) {
  int m = 0;
  do {
    asm volatile( "mov r2, %1 \n"
                  "mov r3, %2 \n"
                  "svc #10    \n"
                  "mov %0, r0 \n"
                : "=r" (m)
                : "r" (mqd), "r" (buf) );

    if (m == -1) {
      yield();
      asm volatile( "mov  %0, r0 \n"
                    "mov  %1, r1 \n"
                    "mov  %2, r2 \n" 
                  : 
                  : "r" (m), "r" (mqd), "r" (buf) );  
    }
  } while (m == -1);
  return;
}


int write( int fd, void* x, size_t n ) {
  int r;

  asm volatile( "mov r0, %1 \n"
                "mov r1, %2 \n"
                "mov r2, %3 \n"
                "svc #1     \n"
                "mov %0, r0 \n" 
              : "=r" (r) 
              : "r" (fd), "r" (x), "r" (n) 
              : "r0", "r1", "r2" );

  return r;
}

int read( int fd, void* x, size_t n ) {
  int r;

  asm volatile( "mov r0, %1 \n"
                "mov r1, %2 \n"
                "mov r2, %3 \n"
                "svc #2     \n"
                "mov %0, r0 \n" 
              : "=r" (r) 
              : "r" (fd), "r" (x), "r" (n) 
              : "r0", "r1", "r2" );

  return r;
}

void disk_wipe() {
  asm volatile( "svc #11 \n" );
}

int fopen( const char *path ) {
  int fd;

  asm volatile( "mov r0, %1 \n"
                "svc #12    \n"
                "mov %0, r0 \n" 
              : "=r" (fd) 
              : "r" (path) 
              : "r0"            );

  return fd;
}

// =========================
// === HELPFUL FUNCTIONS ===
// =========================

char* int2str( int value, char* str, int base ) {
  int c = 0;
  int negative = 0;

  if (value == 0) {
    str[c++] = '0';
  }
  else {
    if (value < 0) {
      negative = 1;
      value *= -1;
    }

    do {
      str[c++] = (value%base) + '0';
      value /= base;
    } while (value != 0);

    if (negative) str[c++] = '-';

    // Reverse string
    char t;
    for (int i = 0; i <= (c-1)/2; i++) {
      t = str[i];
      str[i] = str[c-i-1];
      str[c-i-1] = t;        
    }
  }

  str[c] = '\0';
  return str;
}

void write_int( int fd, int x ) {
  char buf[12];

  int2str( x, buf, 10 );

  int n;
  for (n = 0; n < 12; n++) {
    if (buf[ n ] == '\0') {
      write( fd, buf, n);
      return;
    }
  }
}
