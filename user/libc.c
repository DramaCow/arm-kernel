#include "libc.h"

void yield() {
  asm volatile( "svc #0 \n" );
}

int cfork() {
  int f;
  asm volatile( "svc #3         \n"
                "mov %0, r0     \n"
              : "=r" (f)        
              : 
              : "r0"            );
  return f;
}

void cexec( uint32_t program ) {
  asm volatile( "mov r0, %0 \n" 
                "svc #4     \n"
              :        
              : "r" (program)
              : "r0"            );
}

void cexit() { // TODO: make this a raise sigkill call!
  craise( SIGKILL );
}

void ckill( int pid, sig_t sig ) { 
  asm volatile( "mov r0, %0 \n"
                "mov r1, %1 \n"
                "svc #6     \n"
              :
              : "r" (pid), "r" (sig) 
              : "r0", "r1"           );
}

void craise( sig_t sig ) {
  asm volatile( "mov r0, %0 \n"
                "svc #7     \n" 
                :
                : "r" (sig)     
                : "r0"          );
}

int mqinit( int mqd ) {
  int m;

  asm volatile( "mov r0, %1 \n"
                "svc #8     \n"
                "mov %0, r0 \n"
              : "=r" (m)
              : "r" (mqd)   
              : "r0"            );

  return m;
}

void msgsend( int mqd, void* buf, size_t size ) {
  int m;

  asm volatile( "mov r0, %1 \n"
                "mov r1, %2 \n"
                "mov r2, %3 \n"
                "svc #9     \n"
                "mov %0, r0 \n"
              : "=r" (m)
              : "r" (mqd), "r" (buf), "r" (size) 
              : "r0", "r1"                       );

  // receiving will wake sender when ready
  craise( SIGWAIT );

  return;
}

void msgreceive( int mqd, void* buf, size_t size ) {
  int m;

  asm volatile( "mov r0, %1 \n"
                "mov r1, %2 \n"
                "mov r2, %3 \n"
                "svc #10    \n"
                "mov %0, r0 \n"
              : "=r" (m)
              : "r" (mqd), "r" (buf), "r" (size) 
              : "r0", "r1"                       );

  // If fails, try again later
  if (m == -1) {
    yield();
    msgreceive( mqd, buf, size );
  }

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

int fclose( const int fd ) {
  int r;

  asm volatile( "mov r0, %1 \n"
                "svc #13    \n"
                "mov %0, r0 \n" 
              : "=r" (r) 
              : "r" (fd) 
              : "r0"            );

  return r; 
}

int fseek( const int fd, uint32_t offset, const int whence ) {
  int r;

  asm volatile( "mov r0, %1 \n"
                "mov r1, %2 \n"
                "mov r2, %3 \n"
                "svc #5     \n"
                "mov %0, r0 \n" 
              : "=r" (r) 
              : "r" (fd), "r" (offset), "r" (whence) 
              : "r0"                                 );

  return r; 
}

// ===========================
// === DIRECTORY FUNCTIONS ===
// ===========================

void pwd() {
  asm volatile( "svc #14 \n" );
}

void ls() {
  asm volatile( "svc #15 \n" );
}

int mkdir( const char *name ) {
  int r;

  asm volatile( "mov r0, %1 \n"
                "svc #16    \n" 
                "mov %0, r0 \n"
              : "=r" (r) 
              : "r" (name)
              : "r0"            );

  return r;
}

int cd( const char *path ) {
  int r;

  asm volatile( "mov r0, %1 \n"
                "svc #17    \n" 
                "mov %0, r0 \n"
              : "=r" (r) 
              : "r" (path)
              : "r0"            );

  return r;
}

int rm( const char *name ) {
  return 0;
}

int mv( const char *src, const char *dest ) {
  return 0;
}

int cp( const char *src, const char *dest ) {
  return 0;
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

int str2int( char *str, int n, int base ) {
  int e = 1, value = 0;
  for (int i = n-1; i >= 0; i--) {
    if (str[ i ] == '-')
      return -value;

    value += e * (str[ i ] - '0');
    e *= base;
  } 
  return value;
}

void write_int( int fd, char *buf, int x ) {
  int2str( x, buf, 10 );

  int n;
  for (n = 0; n < 12; n++) {
    if (buf[ n ] == '\0') {
      write( fd, buf, n);
      return;
    }
  }
}
