#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "huffman.h"
#include "fileio.h"

static void print_usage(const char *prog_name);
static void print_banner(void);
static int  get_file_size(const char *path, uint64_t *size);
static void print_compress_stats(const char *output_path, uint64_t original_size);

int main(int argc, char *argv[])
{
    print_banner();

    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *flag = argv[1];

    if (strcmp(flag, "-h") == 0 || strcmp(flag, "--help") == 0) {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    if (argc != 4) {
        fprintf(stderr, "Error: expected exactly 2 file arguments.\n\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *input_path  = argv[2];
    const char *output_path = argv[3];

    if (strcmp(flag, "-c") == 0) {

        printf("Compressing '%s' -> '%s'\n\n", input_path, output_path);

        uint64_t frequencies[ALPHABET_SIZE];
        uint64_t original_size = 0;

        printf("[1/4] Counting symbol frequencies...\n");
        if (fileio_count_frequencies(input_path, frequencies, &original_size) != 0)
            return EXIT_FAILURE;

        printf("      Input size   : %llu bytes\n", (unsigned long long)original_size);
        printf("      Distinct syms: %d\n", huffman_count_symbols(frequencies));

        if (original_size == 0) {
            fprintf(stderr, "Error: input file is empty.\n");
            return EXIT_FAILURE;
        }

        printf("\n[2/4] Building Huffman tree...\n");
        HuffmanNode *root = huffman_build_tree(frequencies);
        if (!root) {
            fprintf(stderr, "Error: failed to build Huffman tree.\n");
            return EXIT_FAILURE;
        }

        printf("\n[3/4] Generating Huffman codes...\n");
        HuffmanCode table[ALPHABET_SIZE];
        huffman_generate_codes(root, table);
        huffman_print_codes(table);

        printf("[4/4] Writing compressed file...\n");
        int rc = fileio_compress(input_path, output_path, table, frequencies, original_size);
        huffman_free_tree(root);

        if (rc != 0) {
            fprintf(stderr, "Error: compression failed.\n");
            return EXIT_FAILURE;
        }

        print_compress_stats(output_path, original_size);
        return EXIT_SUCCESS;

    } else if (strcmp(flag, "-d") == 0) {

        printf("Decompressing '%s' -> '%s'\n\n", input_path, output_path);

        if (fileio_decompress(input_path, output_path) != 0) {
            fprintf(stderr, "Error: decompression failed.\n");
            return EXIT_FAILURE;
        }

        uint64_t out_size = 0;
        get_file_size(output_path, &out_size);
        printf("Done. Output: %s (%llu bytes)\n\n",
               output_path, (unsigned long long)out_size);
        return EXIT_SUCCESS;

    } else {
        fprintf(stderr, "Error: unknown flag '%s'.\n\n", flag);
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
}

static void print_banner(void)
{
    printf("Huffman File Compressor v1.0\n\n");
}

static void print_usage(const char *prog_name)
{
    printf("Usage:\n");
    printf("  %s -c <input> <output>   compress a file\n", prog_name);
    printf("  %s -d <input> <output>   decompress a file\n", prog_name);
    printf("  %s -h                    show this help\n\n", prog_name);
    printf("Examples:\n");
    printf("  %s -c test/sample.txt output.huff\n", prog_name);
    printf("  %s -d output.huff test/restored.txt\n\n", prog_name);
}

static int get_file_size(const char *path, uint64_t *size)
{
    FILE *f = fopen(path, "rb");
    if (!f) { *size = 0; return -1; }
    fseek(f, 0, SEEK_END);
    long s = ftell(f);
    fclose(f);
    *size = (s < 0) ? 0 : (uint64_t)s;
    return 0;
}

static void print_compress_stats(const char *output_path, uint64_t original_size)
{
    uint64_t compressed_size = 0;
    get_file_size(output_path, &compressed_size);

    double ratio      = (original_size > 0)
                        ? (100.0 * (double)compressed_size / (double)original_size)
                        : 0.0;
    double space_saved = 100.0 - ratio;

    printf("\nDone.\n");
    printf("  Original   : %llu bytes\n", (unsigned long long)original_size);
    printf("  Compressed : %llu bytes\n", (unsigned long long)compressed_size);
    printf("  Ratio      : %.1f%%  (%.1f%% saved)\n\n", ratio, space_saved);
}
