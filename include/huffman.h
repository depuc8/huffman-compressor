#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdint.h>

#define ALPHABET_SIZE 256  /* number of possible byte values */
#define MAX_CODE_LEN  256  /* max bits in a single Huffman code */

/* A node in the Huffman binary tree.
   Leaf nodes store a symbol; internal nodes just hold a combined frequency. */
typedef struct HuffmanNode {
    uint8_t  symbol;
    uint64_t frequency;
    struct HuffmanNode *left;   /* 0-bit branch */
    struct HuffmanNode *right;  /* 1-bit branch */
} HuffmanNode;

/* Stores the variable-length bit string for one symbol. */
typedef struct {
    char bits[MAX_CODE_LEN]; /* e.g. "1010" */
    int  length;             /* number of bits */
    int  valid;              /* 1 if this symbol appears in the file */
} HuffmanCode;

/* Allocate a new leaf node. Returns NULL on failure. */
HuffmanNode *huffman_create_node(uint8_t symbol, uint64_t frequency);

/* Build a Huffman tree from a frequency table. Returns the root node.
   Caller must free the tree with huffman_free_tree(). */
HuffmanNode *huffman_build_tree(uint64_t frequencies[ALPHABET_SIZE]);

/* Free all nodes in the tree recursively. */
void huffman_free_tree(HuffmanNode *root);

/* Walk the tree and fill in the code table for every symbol. */
void huffman_generate_codes(HuffmanNode *root, HuffmanCode table[ALPHABET_SIZE]);

/* Print the code table to stdout. */
void huffman_print_codes(HuffmanCode table[ALPHABET_SIZE]);

/* Return the number of distinct symbols with frequency > 0. */
int huffman_count_symbols(uint64_t frequencies[ALPHABET_SIZE]);

#endif /* HUFFMAN_H */
