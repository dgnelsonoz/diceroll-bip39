#include "utf8.h"

#include <stddef.h>

int diceroll_utf8_next( const char **text, uint32_t *codepoint )
{
    const unsigned char *bytes;
    uint32_t value;
    uint32_t minimum;
    uint8_t length;

    if( text == NULL || *text == NULL || codepoint == NULL )
        return -1;
    if( **text == '\0' )
        return 0;

    bytes = ( const unsigned char * )*text;
    if( bytes[ 0 ] < 0x80U )
    {
        *codepoint = bytes[ 0 ];
        *text += 1;
        return 1;
    }
    if( bytes[ 0 ] >= 0xc2U && bytes[ 0 ] <= 0xdfU )
    {
        length = 2U;
        value = bytes[ 0 ] & 0x1fU;
        minimum = 0x80U;
    }
    else if( bytes[ 0 ] >= 0xe0U && bytes[ 0 ] <= 0xefU )
    {
        length = 3U;
        value = bytes[ 0 ] & 0x0fU;
        minimum = 0x800U;
    }
    else if( bytes[ 0 ] >= 0xf0U && bytes[ 0 ] <= 0xf4U )
    {
        length = 4U;
        value = bytes[ 0 ] & 0x07U;
        minimum = 0x10000U;
    }
    else
    {
        *text += 1;
        return -1;
    }

    for( uint8_t index = 1U; index < length; ++index )
    {
        if( ( bytes[ index ] & 0xc0U ) != 0x80U )
        {
            *text += 1;
            return -1;
        }
        value = ( value << 6 ) | ( bytes[ index ] & 0x3fU );
    }
    if( value < minimum || ( value >= 0xd800U && value <= 0xdfffU ) || value > 0x10ffffU )
    {
        *text += 1;
        return -1;
    }

    *codepoint = value;
    *text += length;
    return 1;
}
