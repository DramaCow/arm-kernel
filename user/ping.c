#include "ping.h"

void ping() {
  int m, k = 100;

  char c;
  const char *send = "P I N G >>>>>>>>>>>>>>>\n\n"; // 25 

  while (1) {
    m = mqinit(k);

    for (int i = 0; i < 25; i++) 
      msgsend( m, &send[ i ], 1 );

    for (int i = 0; i < 25; i++) {
      msgreceive( m, &c, 1);
      for (int j = 0; j < 4000000; j++) {}
      write( STDIO, &c, 1 );
    }

    mqunlink(m); k++;
  }

  cexit();
}

void (*entry_ping)() = &ping;
