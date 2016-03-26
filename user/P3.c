#include "P3.h"

void P3() {
  //int data = 0x7FFFFFFF; // 2147483647;
  int *ptr;
  *ptr = 5;


  const int FILE = fopen( "/file.txt" );  

  write_int( 0, FILE ); // success!

  /*

  int m = mqinit(100);

  int message = 1;
  msgsend( m, &message );
  
  msgreceive( m, &message );
  if (message == 2) write( 0, "success2", 8 );

  message = 3;
  msgsend( m, &message );

  msgreceive( m, &message );
  if (message == 4) write( 0, "success4", 8 );

  */

  cexit();
}

void (*entry_P3)() = &P3;
