#ifndef DICEROLL_UTF8_H
#define DICEROLL_UTF8_H

#include <stdint.h>

/* Decode the next UTF-8 code point and advance *text.
 * Returns 1 for a code point, 0 at the string terminator, and -1 for invalid
 * UTF-8. On invalid input, one byte is consumed so callers can recover. */
int diceroll_utf8_next( const char **text, uint32_t *codepoint );

#endif
