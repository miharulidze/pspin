#include "stcp_bitmap.h"
#include "stcp_utils.h"
#include "handler.h"

#ifndef HOST
#define ALL_ONE (~0UL)
#else
#define ALL_ONE (~0U)
#endif


#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 


static int32_t first_one(uint32_t word){
    uint32_t res;
    if (word==0) return 32;
    asm volatile("p.fl1 %0, %1" : "=r"(res) : "r"(word));
    return 31 - res;
}
/*
void print_bitmap(stcp_bitmap_t *bitmap, uint32_t bits){
    printf("bitmap: ");
    for (int i=0; i<bitmap->num_words; i++){
        printf("%u ", bitmap->words[i]);
    }
    printf("\n");

    uint8_t* bitmap_bytes = ((uint8_t*)bitmap->words);
    for (int i=0; i<STCP_BITMAP_SIZE(bits); i += 4){
        printf(BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(bitmap_bytes[i+3]));
        printf(BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(bitmap_bytes[i+2]));
        printf(BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(bitmap_bytes[i+1]));
        printf(BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(bitmap_bytes[i+0]));
    }
    printf("\n");
}
*/

void stcp_bitmap_range_toggle(stcp_bitmap_t *bitmap, uint32_t start, uint32_t bits)
{
    uint32_t bits_left = bits;
    uint32_t unaligned_offset = FAST_MOD(start, WORD_SIZE);
    uint32_t bitmap_offset = start / WORD_SIZE;

    if (unaligned_offset != 0)
    {
        uint32_t unaligned_size = MIN(WORD_SIZE - unaligned_offset, bits);   
        uint32_t unaligned_word = (ALL_ONE >> unaligned_offset);

        if (unaligned_size < WORD_SIZE) 
        {
            unaligned_word &= (ALL_ONE << (WORD_SIZE - (unaligned_offset + unaligned_size)));
        }
    
        //STCP_DPRINTF("OOO: bitmap_offset: %lu; first_bitmap_word: %lu; unaligned_offset: %lu; unaligned_size: %lu\n", bitmap_offset, unaligned_word, unaligned_offset, unaligned_size);

        amo_xor(&(bitmap->words[bitmap_offset]), unaligned_word);

        bits_left -= unaligned_size;
        start += unaligned_size;
        bitmap_offset++;    
    }

    uint32_t num_full_words = bits_left / WORD_SIZE;    
    //STCP_DPRINTF("OOO: setting bits: [%lu; %lu] from bitmap[%lu]; num_full_words: %lu;\n", start, start+bits-1, bitmap_offset, num_full_words);

    for (uint32_t i=0; i<num_full_words; i++)
    {
        //amo_xor(&(bitmap->words[FAST_MOD(bitmap_offset, bitmap->num_words)]), ALL_ONE);
        bitmap->words[FAST_MOD(bitmap_offset, bitmap->num_words)] ^= ALL_ONE;
        bits_left -= WORD_SIZE;
        start += WORD_SIZE;
        bitmap_offset++;
    }

    if (bits_left>0)
    {
        uint32_t unaligned_size = FAST_MOD((start + bits_left), WORD_SIZE);  
        uint32_t unaligned_word = ALL_ONE << (WORD_SIZE - unaligned_size);
        //STCP_DPRINTF("OOO: bitmap_offset: %lu; last_bitmap_word: %lu; unaligned word: %lu; unaligned_size: %lu\n", bitmap_offset, unaligned_word, unaligned_word, unaligned_size);
        //bitmap->words[FAST_MOD(bitmap_offset, bitmap->num_words)] ^= unaligned_word;
        amo_xor(&(bitmap->words[FAST_MOD(bitmap_offset, bitmap->num_words)]), unaligned_word);
    }
}

uint32_t stcp_bitmap_find_set(stcp_bitmap_t *bitmap, uint32_t start){
    uint32_t bitmap_offset = start / WORD_SIZE;
    uint32_t unaligned_offset = start % WORD_SIZE;
    uint32_t count = 0;

    //first word
    uint32_t first_word = bitmap->words[bitmap_offset++];

    first_word = ~(first_word | ~(ALL_ONE >> unaligned_offset));
    int32_t ff1 = first_one(first_word);

    //printf("FIND: bitmap_offset: %lu; unaligned_offset: %lu; first_word: %lu; ffs: %i\n", bitmap_offset, unaligned_offset, first_word, ff1);

    //first_one will return -1 if first_word is 0 (i.e., no bits set), unaligned_offset if that is the only bit set, or >unalinged_offset otherwise
    if (ff1 == (int32_t) unaligned_offset) return 0;


    if ((uint32_t) ff1<WORD_SIZE) {
        //there is a 0 in the first word already
        return ff1 - unaligned_offset;
    }

    count += WORD_SIZE - unaligned_offset;  
    do
    {
        uint32_t word = bitmap->words[FAST_MOD((bitmap_offset++), bitmap->num_words)];
        ff1 = first_one(~word);
        count += ff1;
    } while (ff1==WORD_SIZE);
    
    return count;
}

void stcp_bitmap_clear(stcp_bitmap_t *bitmap){
    for (uint32_t i=0; i<bitmap->num_words; i++){
        bitmap->words[i] = 0;
    }
}

void stcp_bitmap_init(stcp_bitmap_t *bitmap, uint32_t bits){

    uint32_t words = (bits + 8*sizeof(uint32_t) - 1) / (8*sizeof(uint32_t));
    bitmap->num_words=words;

    stcp_bitmap_clear(bitmap);
    //there is an additional 0 word at the end
    bitmap->words[words] = 0;
}
