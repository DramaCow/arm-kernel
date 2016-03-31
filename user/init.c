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
  char* tok;

  while( 1 ) {
    read( STDIO, x, 16);
    tok = strtok(x, " ");

    if      (strncmp(tok, "run", 3) == 0) {
      int p = program_code( strtok(NULL, " ") );   
   
      if (p != -1) { // if successfully found program        
        int f = cfork();

        if (f == 0) {
          cexec( p );
        }
        else if (f == -1) {
          write( STDIO, "\nmemory fault\n", 14 );
          break; // May not be necessary?? - memory could become free between fork calls
        }
      }
    }
    else if (strncmp(tok, "kill", 4) == 0) {
      // should be 1 digit
      ckill( str2int( strtok(NULL, " "), 1, 10 ), SIGKILL );
    }
    else if (strncmp(tok, "wipe", 4) == 0) {
      disk_wipe();
    }
    else if (strncmp(tok, "quit", 4) == 0) {
      cexit(); return;
    }

/*    else if (strncmp(tok, "cd", 2) == 0) {*/

/*    }*/
/*    else if (strncmp(tok, "pwd", 3) == 0) {*/

/*    }*/
/*    else if (strncmp(tok, "ls", 2) == 0) {*/

/*    }*/
/*    else if (strncmp(tok, "mkdir", 5) == 0) {*/

/*    }*/
/*    else if (strncmp(tok, "mv", 2) == 0) {*/

/*    }*/
/*    else if (strncmp(tok, "cp", 2) == 0) {*/

/*    }*/
/*    else if (strncmp(tok, "rm", 2) == 0) {*/

/*    }*/

    yield(); // gets rid of reading delay
  }
}

void (*entry_init)() = &init;
