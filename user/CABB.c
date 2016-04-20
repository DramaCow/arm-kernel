#include "CABB.h"

void CABB() {
  int FILE = fopen( "myfile", O_CREAT );
  
  if (FILE != -1 ) {
    for (int i = 0; i < 90; i++) {
      write( FILE, "CABB", 4 );
      fseek( FILE, 4, SEEK_CUR );
    }
  }

  fclose( FILE );
  
  cexit();
}

void (*entry_CABB)() = &CABB;
