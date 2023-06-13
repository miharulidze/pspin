#pragma once

#include <stdint.h>

typedef uint32_t stcp_bitmap_word_t;

#define WORD_SIZE (sizeof(uint32_t)*8)
#define STCP_BITMAP_DEFINE(bits, name) (                    \
    typedef struct name##_s {                               \
        uint32_t num_words;                                 \
        stcp_bitmap_word_t words[((bits)/WORD_SIZE) + 1];   \
    } name)
#define STCP_BITMAP_SIZE(BITS) (BITS/8)

/* Generic bitmap */
typedef struct stcp_bitmap {
    uint32_t num_words;
    volatile stcp_bitmap_word_t words[1];
} stcp_bitmap_t;

void stcp_bitmap_init(stcp_bitmap_t *bitmap, uint32_t bits);
void stcp_bitmap_clear(stcp_bitmap_t *bitmap);
uint32_t stcp_bitmap_find_set(stcp_bitmap_t *bitmap, uint32_t start);
void stcp_bitmap_range_toggle(stcp_bitmap_t *bitmap, uint32_t start, uint32_t bits);
