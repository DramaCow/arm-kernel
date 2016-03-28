#include "init.h"

int program_code( char* id ) {
  if      (strncmp(id, "P0", 2) == 0) return 0;
  else if (strncmp(id, "P1", 2) == 0) return 1;
  else if (strncmp(id, "P2", 2) == 0) return 2;
  else if (strncmp(id, "P3", 2) == 0) return 3;
  else if (strncmp(id, "P4", 2) == 0) return 4;
  else return -1;
}

int stoi( char* str ) {
  if ('0' <= str[0] && str[0] <= '8')
    return str[0] - 48; // '48' is 0 in ascii 

  if (strncmp(str, "all", 3) == 0)
    return 9;
  
  return -1; // error
}

void init() {
  char x[32];
  char* tok;
  int p = 0;

  while( 1 ) {
    read( 0, x, 16);

    tok = strtok(x, " ");
    if      (strncmp(x, "run", 3) == 0) {

      int p = program_code( strtok(NULL, " ") );      
      if (p != -1) { // if successfully found program  
        int n = strtol( strtok(NULL, " "), NULL, 10 );
            n = n > 0 ? n : 1; // no quantity param => 1 program

        for (int i = 0; i < n; i++) {
          if (cfork(p) == -1) {
            write( 0, "\nmemory fault\n", 14 );
            break; // May not be necessary?? - memory could become free between fork calls
          }
        }
      }

    }
    else if (strncmp(x, "kill", 4) == 0) {
      kill( stoi( strtok(NULL, " ") ) );
    }
    else if (strncmp(x, "wipe", 4) == 0) {
      disk_wipe();
    }
    else if (strncmp(x, "quit", 4) == 0) {
      break;
    }

    yield(); // gets rid of reading delay
  }

  cexit();

  return;
}

void (*entry_init)() = &init;
