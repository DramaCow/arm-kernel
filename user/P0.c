#include "P0.h"

int is_prime( uint32_t x ) {
  if ( !( x & 1 ) || ( x < 2 ) ) {
    return ( x == 2 );
  }

  for( uint32_t d = 3; ( d * d ) <= x ; d += 2 ) {
    if( !( x % d ) ) {
      return 0;
    }
  }

  return 1;
}

void P0() {
  int x = 0;
  char buf[12];

  while( 1 ) {
    // test whether each x for 2^8 < x < 2^24 is prime or not

    for( uint32_t x = ( 1 << 8 ); x < ( 1 << 24 ); x++ ) {
      int r = is_prime( x ); 

      // printf( "is_prime( %d ) = %d\n", x, r );   
      write( 0, "is_prime( ", 10 );
        write_int( 0, buf, x );
      write( 0, " ) = ", 5 );
        write_int( 0, buf, r );
      write( 0, "\n", 1 ); 
      
      //yield();  
    }
  }

  cexit();

  return;
}

// TODO: remove when able to dynamically load programs
void (*entry_P0)() = &P0;
