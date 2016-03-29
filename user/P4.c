#include "P4.h"

void P4() {
  int m = mqinit(100);

  int message;

  msgreceive( m, &message, sizeof( int ) );
  if (message == 15061996) write( 0, "success1", 8 );

  message = 2;
  msgsend( m, &message, sizeof( int ) );

  msgreceive( m, &message, sizeof( int ) );
  if (message == 3) write( 0, "success3", 8 );

  message = 4;
  msgsend( m, &message, sizeof( int ) );

  cexit();
}

void (*entry_P4)() = &P4;
