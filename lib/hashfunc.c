#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include <func.h>

#define hashsize(n) ((unsigned int)1<<(n))
#define hashmask(n) (hashsize(n)-1)

/* basically the same as is used by reiserfs */
static unsigned int r5_hash (const char *s, int len, unsigned int a )
{
  while( len-- ) {
    a += *s << 4;
    a += *s >> 4;
    a *= 11;
    s++;
  }

  return a;
}

/* Another hash function, don't know where I got this from *
static unsigned int c_hash (char *str, int len, unsigned int init )
{
  unsigned int value;   * Used to compute the hash value.  *
  int index;            * Used to cycle through random values. *

  * Set the initial value from key. *
  value = 0x238F13AF * len;
  for (index = 0; index < len; index++)
    value = (value + (str[index] << (index*5 % 24))) & 0x7FFFFFFF;

  value = (1103515243 * value + 12345) & 0x7FFFFFFF;

  * Return the value. *
  return( value );
}
*/

unsigned int lhash( unsigned char *s, unsigned int len, unsigned int init )
{
  return r5_hash( s, len, init ) ;
}

