#include "blanks.h"

void blanks() {
  int FILE = fopen( "grid", O_CREAT );
  
  if (FILE != -1 ) {
    for (int i = 0; i < 32; i++) {
      write( FILE, "________________________________\n", 33 );
    }
  }
  
  fclose( FILE );

  cexit();
}

void (*entry_blanks)() = &blanks;
