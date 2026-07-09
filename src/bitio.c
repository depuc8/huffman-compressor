#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bitio.h"

/* --- BitWriter --- */

BitWriter *bitwriter_create(FILE *file)
{
    if (!file) return NULL;

    BitWriter *bw = calloc(1, sizeof(BitWriter));
    if (!bw) {
        perror("[bitwriter] calloc");
        return NULL;
    }
    bw->file = file;
    return bw;
}

int bitwriter_write_bit(BitWriter *bw, int bit)
{
    if (!bw) return -1;

    /* Set the appropriate bit in the buffer, MSB first. */
    if (bit)
        bw->buffer |= (uint8_t)(1u << (7 - bw->bit_count));

    bw->bit_count++;
    bw->total_bits++;

    /* Flush the full byte to the file. */
    if (bw->bit_count == 8) {
        if (fwrite(&bw->buffer, 1, 1, bw->file) != 1) {
            perror("[bitwriter] fwrite");
            return -1;
        }
        bw->buffer    = 0;
        bw->bit_count = 0;
    }

    return 0;
}

int bitwriter_write_code(BitWriter *bw, const char *code)
{
    if (!bw || !code) return -1;

    for (int i = 0; code[i] != '\0'; i++) {
        if (bitwriter_write_bit(bw, code[i] == '1') != 0)
            return -1;
    }
    return 0;
}

int bitwriter_flush_and_free(BitWriter *bw)
{
    if (!bw) return 0;

    int rc = 0;

    /* Write the last partial byte (zero-padded). */
    if (bw->bit_count > 0) {
        if (fwrite(&bw->buffer, 1, 1, bw->file) != 1) {
            perror("[bitwriter] fwrite flush");
            rc = -1;
        }
    }

    free(bw);
    return rc;
}

/* --- BitReader --- */

BitReader *bitreader_create(FILE *file, uint64_t total_bits)
{
    if (!file) return NULL;

    BitReader *br = calloc(1, sizeof(BitReader));
    if (!br) {
        perror("[bitreader] calloc");
        return NULL;
    }
    br->file       = file;
    br->total_bits = total_bits;
    return br;
}

int bitreader_read_bit(BitReader *br)
{
    if (!br) return -1;

    if (br->bits_read >= br->total_bits)
        return -1;

    /* Refill the buffer when it runs out. */
    if (br->bits_left == 0) {
        if (fread(&br->buffer, 1, 1, br->file) != 1) {
            if (feof(br->file))
                fprintf(stderr, "[bitreader] unexpected end of file\n");
            else
                perror("[bitreader] fread");
            return -1;
        }
        br->bits_left = 8;
    }

    /* Extract the next bit, MSB first. */
    int bit = (br->buffer >> (br->bits_left - 1)) & 1;
    br->bits_left--;
    br->bits_read++;
    return bit;
}

int bitreader_is_exhausted(const BitReader *br)
{
    return !br || (br->bits_read >= br->total_bits);
}

void bitreader_free(BitReader *br)
{
    free(br);
}
