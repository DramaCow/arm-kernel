#ifndef __TERMS_H
#define __TERMS_H

#define STDIO 0x7FFFFFFF

typedef enum { // extreme subset of signals
  SIGKILL,
  SIGWAIT,
  SIGCONT,
  SIGPRI0
} sig_t; //signal

typedef enum {
  SEEK_SET,
  SEEK_CUR,
  SEEK_END
} whence_t; // seek

typedef enum {
  O_CREAT,
  O_EXIST
} oflag_t; 

#endif
