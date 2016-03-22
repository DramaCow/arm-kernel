#include "P3.h"

void P3() {
  int data = 0x7FFFFFFF; // 2147483647;

  write( 0, "wipe started", 12 ); 

  dwrite( 0x0, &data, 4 );
  
  /*
  int data2;
  dread( 0x0, &data2, 4 );

  char buf[12];
  int2str( data2, buf, 10 );
  write( 0, buf, strlen(buf) );
  */

  write( 0, "disk wiped", 10 );

  /*

  int m = mqinit(100);

  int message = 1;
  msgsend( m, &message );
  
  msgreceive( m, &message );
  if (message == 2) write( 0, "success2", 8 );

  message = 3;
  msgsend( m, &message );

  msgreceive( m, &message );
  if (message == 4) write( 0, "success4", 8 );

  */

  cexit();
}

void (*entry_P3)() = &P3;
