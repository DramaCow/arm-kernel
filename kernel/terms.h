#ifndef __TERMS_H
#define __TERMS_H

#define STDIO 0x7FFFFFFF

typedef enum { // extreme subset of signals
  SIGKILL,
  SIGWAIT,
  SIGCONT
} sig_t; //signal

typedef enum {
  SEEK_SET,
  SEEK_CUR,
  SEEK_END
} whence_t; // seek

#endif
