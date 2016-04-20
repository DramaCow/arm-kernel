#include "pong.h"

void pong() {
  int m = mqinit(100);

  char c;
  const char *send = "<<<<<<<<<<<<<<< P O N G\n\n"; // 16+9=25  

  while (1) {
    for (int i = 0; i < 25; i++) {
      msgreceive( m, &c, 1);
      for (int j = 0; j < 4000000; j++) {}
      write( STDIO, &c, 1 );
    }

    for (int i = 0; i < 25; i++) 
      msgsend( m, &send[ i ], 1 );
  }

  cexit();
}

void (*entry_pong)() = &pong;
