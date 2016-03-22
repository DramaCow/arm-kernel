#include "P4.h"

void P4() {
  int m = mqinit(100);

  int message;

  msgreceive( m, &message );
  if (message == 1) write( 0, "success1", 8 );

  message = 2;
  msgsend( m, &message );

  msgreceive( m, &message );
  if (message == 3) write( 0, "success3", 8 );

  message = 4;
  msgsend( m, &message );

  cexit();
}

void (*entry_P4)() = &P4;
