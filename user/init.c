#include "init.h"

void init() {
  char x[64];
  char* tok;

  while( 1 ) {
    write( STDIO, "$ ", 2 );
    read( STDIO, x, 64);
    tok = strtok(x, " ");

    if      (strncmp(tok, "run", 4) == 0) {       
      tok = strtok(NULL, " \n\r");

      int f = cfork();
      if (f == 0) {
        cexec( tok ); break;
      }
      else if (f == -1) {
        write( STDIO, "memory fault\n", 14 );
      }
    }
    else if (strncmp(tok, "kill", 4) == 0) {
      ckill( str2int( strtok(NULL, " \n\r"), 1, 10 ), SIGKILL );
    }
    else if (strncmp(tok, "wipe", 4) == 0) {
      disk_wipe();
    }
    else if (strncmp(tok, "quit", 4) == 0) {
      break;
    }
    else if (strncmp(tok, "pwd", 3) == 0) {
      pwd();
    }
    else if (strncmp(tok, "ls", 2) == 0) {
      ls();
    }
    else if (strncmp(tok, "mkdir", 5) == 0) {
      mkdir( strtok( NULL, " \n\r" ) );
    }
    else if (strncmp(tok, "cd", 2) == 0) {
      cd( strtok( NULL, " \n\r" ) );
    }
    else if (strncmp(tok, "rm", 2) == 0) {
      rm( strtok( NULL, " \n\r" ) );
    }
    else if (strncmp(tok, "mv", 2) == 0) {
      mv( strtok( NULL, " \n\r" ), strtok( NULL, " \n\r" ) );
    }
    else if (strncmp(tok, "cp", 2) == 0) {
      cp( strtok( NULL, " \n\r" ), strtok( NULL, " \n\r" ) );
    }
    else if (strncmp(tok, "cat", 3) == 0) {
      tok = strtok( NULL, " \n\r" );
      int FILE = fopen( tok, O_EXIST );
      if (FILE != -1) {
        fseek( FILE, 0, SEEK_END );
        int length = ftell( FILE );
        fseek( FILE, 0, SEEK_SET );

        char buf[ 512 ]; int n;
        for (int i = 0; i < length; i+=512) {
          n = length - i > 512 ? 512 : length - i;
          read( FILE, &buf, n );
          write( STDIO, &buf, n );
        }

        fclose( FILE );
      } 
    }

    craise( SIGPRI0 );
    yield(); // gets rid of reading delay
  }

  cexit();
}

void (*entry_init)() = &init;
