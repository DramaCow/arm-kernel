#include "P1.h"

uint32_t gcd( uint32_t x, uint32_t y ) {
  if     ( x == y ) {
    return x;
  }
  else if( x >  y ) {
    return gcd( x - y, y );
  }
  else if( x <  y ) {
    return gcd( x, y - x );
  }
}

void P1() {
  char buf[12];

  while( 1 ) {
    // compute the gcd between pairs of x and y for 2^8 < x, y < 2^24

    for( uint32_t x = ( 1 << 8 ); x < ( 1 << 24 ); x++ ) {
      for( uint32_t y = ( 1 << 8 ); y < ( 1 << 24 ); y++ ) {
        uint32_t r = gcd( x, y );  

        // printf( "gcd( %d, %d ) = %d\n", x, y, r );
        write( 0, "gcd( ", 5 );
        int2str( x, buf, 10 );
        write( 0, buf, strlen( buf ) );
        write( 0, ", ", 2 );
        int2str( y, buf, 10 );
        write( 0, buf, strlen( buf ) );
        write( 0, " ) = ", 5 );
        int2str( r, buf, 10 );
        write( 0, buf, strlen( buf ) );
        write( 0, "\n", 1 );
      }
    }
  }

  cexit();

  return;
}

// TODO: remove when able to dynamically load programs
void (*entry_P1)() = &P1;
