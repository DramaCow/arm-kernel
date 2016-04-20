#include "MESSAGE.h"

void MESSAGE() {
  int FILE = fopen( "myfile", O_CREAT );
  
  if (FILE != -1 ) {
    for (int i = 0; i < 100; i++) {
      write( FILE, "MESSAGE\n", 8 );
    }
  }
  
  fclose( FILE );

  cexit();
}

void (*entry_MESSAGE)() = &MESSAGE;
