#include "P3.h"

void P3() {
/*  const int FILE = fopen( "/file.txt" );*/
/*  char buf[12];*/
/*  write_int( 0, buf, FILE ); // success!*/

  int m = mqinit(100);

  int message = 15061996;
  msgsend( m, &message, sizeof( int ) );

  msgreceive( m, &message, sizeof( int ) );
  if (message == 2) write( 0, "success2", 8 );

  message = 3;
  msgsend( m, &message, sizeof( int ) );

  msgreceive( m, &message, sizeof( int ) );
  if (message == 4) write( 0, "success4", 8 );

  cexit();
}

void (*entry_P3)() = &P3;
