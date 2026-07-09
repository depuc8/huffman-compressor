#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "fileio.h"
#include "huffman.h"
#include "bitio.h"

/* Write a uint32_t in little-endian byte order. */
static int write_u32(FILE *f, uint32_t value)
{
    uint8_t buf[4];
    buf[0] = (uint8_t)(value & 0xFF);
    buf[1] = (uint8_t)((value >> 8)  & 0xFF);
    buf[2] = (uint8_t)((value >> 16) & 0xFF);
    buf[3] = (uint8_t)((value >> 24) & 0xFF);
    return (fwrite(buf, 1, 4, f) == 4) ? 0 : -1;
}

/* Write a uint64_t in little-endian byte order. */
static int write_u64(FILE *f, uint64_t value)
{
    uint8_t buf[8];
    for (int i = 0; i < 8; i++)
        buf[i] = (uint8_t)((value >> (i * 8)) & 0xFF);
    return (fwrite(buf, 1, 8, f) == 8) ? 0 : -1;
}

/* Read a uint32_t in little-endian byte order. */
static int read_u32(FILE *f, uint32_t *out)
{
    uint8_t buf[4];
    if (fread(buf, 1, 4, f) != 4) return -1;
    *out = (uint32_t)buf[0]
         | ((uint32_t)buf[1] << 8)
         | ((uint32_t)buf[2] << 16)
         | ((uint32_t)buf[3] << 24);
    return 0;
}

/* Read a uint64_t in little-endian byte order. */
static int read_u64(FILE *f, uint64_t *out)
{
    uint8_t buf[8];
    if (fread(buf, 1, 8, f) != 8) return -1;
    *out = 0;
    for (int i = 0; i < 8; i++)
        *out |= ((uint64_t)buf[i] << (i * 8));
    return 0;
}

int fileio_count_frequencies(const char *filepath,
                             uint64_t frequencies[ALPHABET_SIZE],
                             uint64_t *file_size)
{
    memset(frequencies, 0, sizeof(uint64_t) * ALPHABET_SIZE);
    *file_size = 0;

    FILE *f = fopen(filepath, "rb");
    if (!f) { perror(filepath); return -1; }

    int c;
    while ((c = fgetc(f)) != EOF) {
        frequencies[(uint8_t)c]++;
        (*file_size)++;
    }

    fclose(f);
    return 0;
}

int fileio_compress(const char *input_path,
                    const char *output_path,
                    HuffmanCode table[ALPHABET_SIZE],
                    uint64_t    frequencies[ALPHABET_SIZE],
                    uint64_t    original_size)
{
    uint32_t num_symbols = 0;
    for (int i = 0; i < ALPHABET_SIZE; i++)
        if (frequencies[i] > 0) num_symbols++;

    FILE *in = fopen(input_path, "rb");
    if (!in) { perror(input_path); return -1; }

    FILE *out = fopen(output_path, "wb");
    if (!out) { perror(output_path); fclose(in); return -1; }

    /* Write the header: magic, num_symbols, original_size, then a placeholder
       for total_bits that we'll patch once the bit stream is written. */
    if (fwrite(HUFF_MAGIC, 1, HUFF_MAGIC_LEN, out) != HUFF_MAGIC_LEN) goto err;
    if (write_u32(out, num_symbols)  != 0) goto err;
    if (write_u64(out, original_size) != 0) goto err;

    long total_bits_offset = ftell(out);
    if (total_bits_offset < 0) goto err;

    uint64_t placeholder = 0;
    if (write_u64(out, placeholder) != 0) goto err;

    /* Write the symbol frequency table. */
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (frequencies[i] == 0) continue;
        uint8_t sym = (uint8_t)i;
        if (fwrite(&sym, 1, 1, out) != 1)          goto err;
        if (write_u64(out, frequencies[i]) != 0)   goto err;
    }

    /* Write the bit stream. */
    BitWriter *bw = bitwriter_create(out);
    if (!bw) goto err;

    int c;
    while ((c = fgetc(in)) != EOF) {
        uint8_t sym = (uint8_t)c;
        if (!table[sym].valid) {
            fprintf(stderr, "[fileio] no code for symbol 0x%02X\n", sym);
            bitwriter_flush_and_free(bw);
            goto err;
        }
        if (bitwriter_write_code(bw, table[sym].bits) != 0) {
            bitwriter_flush_and_free(bw);
            goto err;
        }
    }

    uint64_t total_bits = bw->total_bits;
    if (bitwriter_flush_and_free(bw) != 0) goto err;

    /* Seek back and write the real total_bits value into the header. */
    if (fseek(out, total_bits_offset, SEEK_SET) != 0) goto err;
    if (write_u64(out, total_bits) != 0) goto err;

    fclose(in);
    fclose(out);
    return 0;

err:
    fclose(in);
    fclose(out);
    return -1;
}

int fileio_decompress(const char *input_path, const char *output_path)
{
    FILE *in = fopen(input_path, "rb");
    if (!in) { perror(input_path); return -1; }

    /* Validate magic. */
    char magic[HUFF_MAGIC_LEN];
    if (fread(magic, 1, HUFF_MAGIC_LEN, in) != HUFF_MAGIC_LEN) {
        fprintf(stderr, "[fileio] file too short\n");
        fclose(in); return -1;
    }
    if (memcmp(magic, HUFF_MAGIC, HUFF_MAGIC_LEN) != 0) {
        fprintf(stderr, "[fileio] not a valid .huff file\n");
        fclose(in); return -1;
    }

    /* Read header fields. */
    uint32_t num_symbols   = 0;
    uint64_t original_size = 0;
    uint64_t total_bits    = 0;

    if (read_u32(in, &num_symbols)   != 0 ||
        read_u64(in, &original_size) != 0 ||
        read_u64(in, &total_bits)    != 0) {
        fprintf(stderr, "[fileio] failed to read header\n");
        fclose(in); return -1;
    }

    if (num_symbols == 0 || num_symbols > ALPHABET_SIZE) {
        fprintf(stderr, "[fileio] invalid num_symbols (%u)\n", num_symbols);
        fclose(in); return -1;
    }

    /* Read the symbol frequency table. */
    uint64_t frequencies[ALPHABET_SIZE];
    memset(frequencies, 0, sizeof(frequencies));

    for (uint32_t i = 0; i < num_symbols; i++) {
        uint8_t  sym  = 0;
        uint64_t freq = 0;
        if (fread(&sym, 1, 1, in) != 1 || read_u64(in, &freq) != 0) {
            fprintf(stderr, "[fileio] failed to read symbol table entry %u\n", i);
            fclose(in); return -1;
        }
        frequencies[sym] = freq;
    }

    /* Rebuild the Huffman tree from the stored frequencies. */
    HuffmanNode *root = huffman_build_tree(frequencies);
    if (!root) {
        fprintf(stderr, "[fileio] failed to rebuild Huffman tree\n");
        fclose(in); return -1;
    }

    FILE *out = fopen(output_path, "wb");
    if (!out) {
        perror(output_path);
        huffman_free_tree(root);
        fclose(in);
        return -1;
    }

    /* Decode the bit stream: walk left on 0, right on 1.
       Emit a symbol each time a leaf is reached, then restart from root. */
    BitReader   *br      = bitreader_create(in, total_bits);
    uint64_t     decoded = 0;
    HuffmanNode *current = root;

    while (decoded < original_size && !bitreader_is_exhausted(br)) {
        int bit = bitreader_read_bit(br);
        if (bit < 0) break;

        current = (bit == 0) ? current->left : current->right;

        if (!current) {
            fprintf(stderr, "[fileio] tree traversal error — corrupt file?\n");
            break;
        }

        if (!current->left && !current->right) {
            if (fwrite(&current->symbol, 1, 1, out) != 1) {
                perror("[fileio] fwrite");
                break;
            }
            decoded++;
            current = root;
        }
    }

    bitreader_free(br);
    huffman_free_tree(root);
    fclose(in);
    fclose(out);

    if (decoded != original_size) {
        fprintf(stderr, "[fileio] decoded %llu bytes but expected %llu\n",
                (unsigned long long)decoded,
                (unsigned long long)original_size);
        return -1;
    }

    return 0;
}
