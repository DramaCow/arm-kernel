#include "hashs.h"

void hashs() {
  int FILE = fopen( "grid", O_CREAT );
  
  if (FILE != -1 ) {
    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 4; j++) {
        write( FILE, "####", 4 ); fseek( FILE, 4, SEEK_CUR );
        write( FILE, "####", 4 ); fseek( FILE, 4, SEEK_CUR );
        write( FILE, "####", 4 ); fseek( FILE, 4, SEEK_CUR );
        write( FILE, "####", 4 ); fseek( FILE, 5, SEEK_CUR );
      }
      if (i % 2 == 0) fseek( FILE,  4, SEEK_CUR );
      else            fseek( FILE, -4, SEEK_CUR );
    }
  }

  fclose( FILE );
  
  cexit();
}

void (*entry_hashs)() = &hashs;
