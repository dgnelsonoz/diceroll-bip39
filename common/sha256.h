#ifndef SHA256_H
#define SHA256_H

#include <stddef.h>
#include <stdint.h>

enum { SHA256_DIGEST_SIZE = 32 };

void sha256( const uint8_t *data, size_t length, uint8_t digest[ SHA256_DIGEST_SIZE ] );

#endif
