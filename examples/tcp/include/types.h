#pragma once
#include <stdint.h>

#define __LITTLE_ENDIAN_BITFIELD

#define __u8 uint8_t
#define __u16 uint16_t
#define __u32 uint32_t
#define __u64 uint64_t
#define __be16 uint16_t
#define __be32 uint32_t
#define __sum16 uint16_t

#define __constant_cpu_to_be32(X) X

#define U32_H16(i) ((i) >> 16)
#define U32_L16(i) ((i)&0xFFFF)
#define HTONS(n) (((((unsigned short)(n)&0xFF)) << 8) | (((unsigned short)(n)&0xFF00) >> 8))
#define NTOHS(n) (((((unsigned short)(n)&0xFF)) << 8) | (((unsigned short)(n)&0xFF00) >> 8))

#define HTONL(n) (((((unsigned long)(n) & 0xFF)) << 24) | \
                  ((((unsigned long)(n) & 0xFF00)) << 8) | \
                  ((((unsigned long)(n) & 0xFF0000)) >> 8) | \
                  ((((unsigned long)(n) & 0xFF000000)) >> 24))

#define NTOHL(n) (((((unsigned long)(n) & 0xFF)) << 24) | \
                  ((((unsigned long)(n) & 0xFF00)) << 8) | \
                  ((((unsigned long)(n) & 0xFF0000)) >> 8) | \
                  ((((unsigned long)(n) & 0xFF000000)) >> 24))
                  
