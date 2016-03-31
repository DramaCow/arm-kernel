#include "P3.h"

void P3() {
  const int FILE = fopen( "/file.txt" );

  write( FILE, "Hello, world!", 13 );

  fclose( FILE );

/*  int m = mqinit(100);*/

/*  int message = 15061996;*/
/*  msgsend( m, &message, sizeof( int ) );*/

/*  msgreceive( m, &message, sizeof( int ) );*/
/*  if (message == 2) write( STDIO, "success2", 8 );*/

/*  message = 3;*/
/*  msgsend( m, &message, sizeof( int ) );*/

/*  msgreceive( m, &message, sizeof( int ) );*/
/*  if (message == 4) write( STDIO, "success4", 8 );*/

  cexit();
}

void (*entry_P3)() = &P3;
