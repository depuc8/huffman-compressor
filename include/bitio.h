#ifndef BITIO_H
#define BITIO_H

#include <stdio.h>
#include <stdint.h>

/* Accumulates bits into a byte buffer and writes complete bytes to a file.
   Bits are written MSB-first (bit 7 first). */
typedef struct {
    FILE    *file;
    uint8_t  buffer;     /* byte being assembled */
    int      bit_count;  /* bits written into buffer so far (0-7) */
    uint64_t total_bits; /* total bits written across all bytes */
} BitWriter;

/* Reads bytes from a file and exposes them one bit at a time.
   Stops after total_bits bits so padding is never returned. */
typedef struct {
    FILE    *file;
    uint8_t  buffer;     /* current byte being consumed */
    int      bits_left;  /* unread bits remaining in buffer */
    uint64_t total_bits; /* total payload bits (from the file header) */
    uint64_t bits_read;  /* how many bits have been consumed */
} BitReader;

/* Create a BitWriter for an open file. Returns NULL on failure. */
BitWriter *bitwriter_create(FILE *file);

/* Write a single bit (0 or 1). Returns 0 on success, -1 on error. */
int bitwriter_write_bit(BitWriter *bw, int bit);

/* Write every bit in a code string like "10110". Returns 0 on success. */
int bitwriter_write_code(BitWriter *bw, const char *code);

/* Flush any remaining bits (zero-padded) and free the writer.
   Must be called before closing the file. Returns 0 on success. */
int bitwriter_flush_and_free(BitWriter *bw);

/* Create a BitReader for an open file. total_bits is the exact number of
   payload bits to read (from the .huff header). Returns NULL on failure. */
BitReader *bitreader_create(FILE *file, uint64_t total_bits);

/* Read the next bit. Returns 0 or 1, or -1 when exhausted / on error. */
int bitreader_read_bit(BitReader *br);

/* Returns non-zero if all payload bits have been consumed. */
int bitreader_is_exhausted(const BitReader *br);

/* Free the BitReader (does not close the file). */
void bitreader_free(BitReader *br);

#endif /* BITIO_H */
