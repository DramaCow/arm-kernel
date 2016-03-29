#include "init.h"

int program_code( char* id ) {
  if      (strncmp(id, "P0", 2) == 0) return 0;
  else if (strncmp(id, "P1", 2) == 0) return 1;
  else if (strncmp(id, "P2", 2) == 0) return 2;
  else if (strncmp(id, "P3", 2) == 0) return 3;
  else if (strncmp(id, "P4", 2) == 0) return 4;
  else return -1;
}

void init() {

/*  int f1 = cfork();*/
/*  if (f1 == 0) cexec(0);*/

/*  int f2 = cfork();*/
/*  if (f2 == 0) cexec(0);*/

  char x[32];

  while( 1 ) {
    read( 0, x, 16);
    char* tok = strtok(x, " ");

    if (strncmp(tok, "run", 3) == 0) {
      int p = program_code( strtok(NULL, " ") );   
   
      if (p != -1) { // if successfully found program        
        int f = cfork();

        if (f == 0) {
          cexec( p );
        }
        else if (f == -1) {
          write( 0, "\nmemory fault\n", 14 );
          break; // May not be necessary?? - memory could become free between fork calls
        }
      }
    }
    else if (strncmp(tok, "kill", 4) == 0) {
      // kill( stoi( strtok(NULL, " ") ) );
    }
    else if (strncmp(tok, "wipe", 4) == 0) {
      disk_wipe();
    }
    else if (strncmp(tok, "quit", 4) == 0) {
      cexit();
      return;
    }

    yield(); // gets rid of reading delay
  }
}

void (*entry_init)() = &init;
