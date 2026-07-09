#ifndef FILEIO_H
#define FILEIO_H

#include <stdint.h>
#include "huffman.h"

/*
 * .huff file header layout:
 *   [8 bytes]  magic           "HUFF\x01\x00\x00\x00"
 *   [4 bytes]  num_symbols     uint32_t
 *   [8 bytes]  original_size   uint64_t (bytes)
 *   [8 bytes]  total_bits      uint64_t (payload bits in the bit stream)
 *   then for each symbol:
 *     [1 byte]   symbol value
 *     [8 bytes]  frequency (uint64_t)
 *   followed by the raw bit stream.
 */

#define HUFF_MAGIC     "HUFF\x01\x00\x00\x00"
#define HUFF_MAGIC_LEN 8

/* Count how many times each byte value appears in a file.
   Also writes the total file size to *file_size.
   Returns 0 on success, -1 on error. */
int fileio_count_frequencies(const char *filepath,
                             uint64_t frequencies[ALPHABET_SIZE],
                             uint64_t *file_size);

/* Compress input_path into output_path using the given code table.
   Returns 0 on success, -1 on error. */
int fileio_compress(const char *input_path,
                    const char *output_path,
                    HuffmanCode table[ALPHABET_SIZE],
                    uint64_t frequencies[ALPHABET_SIZE],
                    uint64_t original_size);

/* Decompress a .huff file back to its original contents.
   Returns 0 on success, -1 on error. */
int fileio_decompress(const char *input_path,
                      const char *output_path);

#endif /* FILEIO_H */
