/*
 * bench.c — Huffman Compressor Benchmark Driver
 *
 * Measures:
 *   - Compression throughput (MB/s)
 *   - Decompression throughput (MB/s)
 *   - Compression ratio (%)
 *   - Space saved (%)
 *   - Round-trip correctness verification
 *
 * Build via the benchmark/Makefile, then run:
 *   ./bench
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>

/* ------------------------------------------------------------------ */
/* Portable monotonic clock (POSIX clock_gettime)                       */
/* ------------------------------------------------------------------ */
static double now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/* ------------------------------------------------------------------ */
/* File helpers                                                         */
/* ------------------------------------------------------------------ */
static long file_size(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    return (long)st.st_size;
}

/*
 * Compare two files byte-for-byte.
 * Returns 0 if identical, 1 if different, -1 on error.
 */
static int files_equal(const char *a, const char *b)
{
    FILE *fa = fopen(a, "rb");
    FILE *fb = fopen(b, "rb");
    if (!fa || !fb) {
        if (fa) fclose(fa);
        if (fb) fclose(fb);
        return -1;
    }

    int result = 0;
    int ca, cb;
    do {
        ca = fgetc(fa);
        cb = fgetc(fb);
        if (ca != cb) { result = 1; break; }
    } while (ca != EOF);

    fclose(fa);
    fclose(fb);
    return result;
}

/* ------------------------------------------------------------------ */
/* Invoke the compress binary as a subprocess                          */
/* ------------------------------------------------------------------ */
#define COMPRESS_BIN "../compress"

/*
 * Run one compress or decompress pass and return elapsed wall-clock
 * seconds.  Returns -1.0 on failure.
 */
static double run_pass(const char *flag,
                       const char *input,
                       const char *output)
{
    /* Build the shell command (redirect stdout/stderr to /dev/null so
       the benchmark output stays clean).                               */
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "%s %s \"%s\" \"%s\" > /dev/null 2>&1",
             COMPRESS_BIN, flag, input, output);

    double t0 = now_sec();
    int rc = system(cmd);
    double t1 = now_sec();

    if (rc != 0) return -1.0;
    return t1 - t0;
}

/* ------------------------------------------------------------------ */
/* Pretty-print helpers                                                 */
/* ------------------------------------------------------------------ */
static void print_separator(void)
{
    printf("+-%-20s-+-%-10s-+-%-10s-+-%-12s-+-%-12s-+-%-10s-+\n",
           "--------------------",
           "----------",
           "----------",
           "------------",
           "------------",
           "----------");
}

static void print_header(void)
{
    print_separator();
    printf("| %-20s | %-10s | %-10s | %-12s | %-12s | %-10s |\n",
           "File", "Orig (KB)", "Comp (KB)", "Comp MB/s", "Decomp MB/s", "Ratio %");
    print_separator();
}

/* ------------------------------------------------------------------ */
/* Single benchmark case                                               */
/* ------------------------------------------------------------------ */
typedef struct {
    const char *label;      /* human-readable name shown in the table */
    const char *input;      /* path to input file (relative to benchmark/) */
} BenchCase;

static void run_case(const BenchCase *bc, int *pass_count, int *fail_count)
{
    /* Temporary output paths (inside benchmark/ so we stay self-contained). */
    char compressed[512];
    char restored[512];
    snprintf(compressed, sizeof(compressed), "tmp_%s.huff", bc->label);
    snprintf(restored,   sizeof(restored),   "tmp_%s.restored", bc->label);

    long orig_bytes = file_size(bc->input);
    if (orig_bytes <= 0) {
        printf("| %-20s | %-10s | %-10s | %-12s | %-12s | %-10s |\n",
               bc->label, "MISSING", "-", "-", "-", "-");
        (*fail_count)++;
        return;
    }

    /* --- Compress -------------------------------------------------- */
    double comp_sec = run_pass("-c", bc->input, compressed);
    if (comp_sec < 0.0) {
        printf("| %-20s | %-10ld | %-10s | %-12s | %-12s | %-10s |\n",
               bc->label, orig_bytes / 1024, "ERR", "ERR", "-", "-");
        (*fail_count)++;
        return;
    }

    long comp_bytes = file_size(compressed);

    /* --- Decompress ------------------------------------------------ */
    double decomp_sec = run_pass("-d", compressed, restored);
    if (decomp_sec < 0.0) {
        printf("| %-20s | %-10ld | %-10ld | %-12s | %-12s | %-10s |\n",
               bc->label,
               orig_bytes  / 1024,
               comp_bytes  / 1024,
               "ERR", "-", "-");
        remove(compressed);
        (*fail_count)++;
        return;
    }

    /* --- Verify round-trip ----------------------------------------- */
    int eq = files_equal(bc->input, restored);

    /* --- Compute metrics ------------------------------------------- */
    double orig_mb    = (double)orig_bytes  / (1024.0 * 1024.0);
    double comp_ratio = (orig_bytes > 0)
                        ? (100.0 * (double)comp_bytes / (double)orig_bytes)
                        : 0.0;

    /* Guard against near-zero times to avoid division by zero. */
    double comp_mbs   = (comp_sec   > 1e-9) ? orig_mb / comp_sec   : 0.0;
    double decomp_mbs = (decomp_sec > 1e-9) ? orig_mb / decomp_sec : 0.0;

    printf("| %-20s | %-10ld | %-10ld | %-12.2f | %-12.2f | %-9.1f%% |\n",
           bc->label,
           orig_bytes / 1024,
           comp_bytes / 1024,
           comp_mbs,
           decomp_mbs,
           comp_ratio);

    if (eq == 0) {
        (*pass_count)++;
    } else {
        printf("  *** ROUND-TRIP MISMATCH for %s ***\n", bc->label);
        (*fail_count)++;
    }

    /* Clean up temp files. */
    remove(compressed);
    remove(restored);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */
int main(void)
{
    printf("\n");
    printf("  Huffman Compressor — Performance Benchmark\n");
    printf("  ==========================================\n\n");

    /* Check that the binary exists. */
    if (file_size(COMPRESS_BIN) < 0) {
        fprintf(stderr,
                "ERROR: binary '%s' not found.\n"
                "       Run 'make' in the project root first.\n\n",
                COMPRESS_BIN);
        return EXIT_FAILURE;
    }

    /* Benchmark cases — add more rows here as needed. */
    static const BenchCase cases[] = {
        { "random_1KB",    "inputs/random_1KB.bin"    },
        { "random_10KB",   "inputs/random_10KB.bin"   },
        { "random_100KB",  "inputs/random_100KB.bin"  },
        { "random_1MB",    "inputs/random_1MB.bin"    },
        { "text_10KB",     "inputs/text_10KB.txt"     },
        { "text_100KB",    "inputs/text_100KB.txt"    },
        { "text_1MB",      "inputs/text_1MB.txt"      },
        { "repeated_1MB",  "inputs/repeated_1MB.bin"  },
        { "zeros_1MB",     "inputs/zeros_1MB.bin"     },
    };
    int n = (int)(sizeof(cases) / sizeof(cases[0]));

    int pass = 0, fail = 0;

    print_header();
    for (int i = 0; i < n; i++)
        run_case(&cases[i], &pass, &fail);
    print_separator();

    printf("\n  Results: %d PASSED  /  %d FAILED  (total %d)\n\n",
           pass, fail, pass + fail);

    return (fail == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
