#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "huffman.h"
#include "heap.h"

HuffmanNode *huffman_create_node(uint8_t symbol, uint64_t frequency)
{
    HuffmanNode *node = malloc(sizeof(HuffmanNode));
    if (!node) {
        perror("[huffman] malloc HuffmanNode");
        return NULL;
    }
    node->symbol    = symbol;
    node->frequency = frequency;
    node->left      = NULL;
    node->right     = NULL;
    return node;
}

HuffmanNode *huffman_build_tree(uint64_t frequencies[ALPHABET_SIZE])
{
    int num_symbols = huffman_count_symbols(frequencies);

    if (num_symbols == 0) {
        fprintf(stderr, "[huffman] input file is empty\n");
        return NULL;
    }

    MinHeap *heap = heap_create(num_symbols);
    if (!heap) return NULL;

    /* Insert one leaf node for each symbol that appears in the file. */
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (frequencies[i] > 0) {
            HuffmanNode *node = huffman_create_node((uint8_t)i, frequencies[i]);
            if (!node || heap_insert(heap, node) != 0) {
                free(node);
                heap_free(heap);
                return NULL;
            }
        }
    }

    /* Special case: only one distinct symbol.
       Wrap it in a dummy parent so it still gets a valid code. */
    if (heap_size(heap) == 1) {
        HuffmanNode *only  = heap_extract_min(heap);
        HuffmanNode *dummy = huffman_create_node(0, only->frequency);
        if (!dummy) {
            free(only);
            heap_free(heap);
            return NULL;
        }
        dummy->left = only;
        heap_free(heap);
        return dummy;
    }

    /* Merge the two smallest nodes until one remains — that is the root. */
    while (heap_size(heap) > 1) {
        HuffmanNode *left  = heap_extract_min(heap);
        HuffmanNode *right = heap_extract_min(heap);

        HuffmanNode *parent = huffman_create_node(0, left->frequency + right->frequency);
        if (!parent) {
            heap_free(heap);
            return NULL;
        }
        parent->left  = left;
        parent->right = right;

        if (heap_insert(heap, parent) != 0) {
            heap_free(heap);
            return NULL;
        }
    }

    HuffmanNode *root = heap_extract_min(heap);
    heap_free(heap);
    return root;
}

void huffman_free_tree(HuffmanNode *root)
{
    if (!root) return;
    huffman_free_tree(root->left);
    huffman_free_tree(root->right);
    free(root);
}

/* Recursive DFS traversal.
   Left edge appends '0', right edge appends '1'.
   When a leaf is reached, save the accumulated prefix as its code. */
static void generate_codes_recursive(HuffmanNode *node,
                                     HuffmanCode  table[ALPHABET_SIZE],
                                     char         prefix[MAX_CODE_LEN],
                                     int          depth)
{
    if (!node) return;

    if (!node->left && !node->right) {
        prefix[depth] = '\0';
        strncpy(table[node->symbol].bits, prefix, MAX_CODE_LEN - 1);
        table[node->symbol].bits[MAX_CODE_LEN - 1] = '\0';
        table[node->symbol].length = depth;
        table[node->symbol].valid  = 1;
        return;
    }

    prefix[depth] = '0';
    generate_codes_recursive(node->left,  table, prefix, depth + 1);

    prefix[depth] = '1';
    generate_codes_recursive(node->right, table, prefix, depth + 1);
}

void huffman_generate_codes(HuffmanNode *root, HuffmanCode table[ALPHABET_SIZE])
{
    memset(table, 0, sizeof(HuffmanCode) * ALPHABET_SIZE);
    if (!root) return;

    char prefix[MAX_CODE_LEN];
    generate_codes_recursive(root, table, prefix, 0);
}

void huffman_print_codes(HuffmanCode table[ALPHABET_SIZE])
{
    printf("\n%-10s %-12s %-8s %s\n", "Symbol", "Char", "Length", "Code");
    printf("%-10s %-12s %-8s %s\n", "----------", "------------", "--------", "----");

    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (!table[i].valid) continue;

        char display[8];
        if (i >= 32 && i < 127)
            snprintf(display, sizeof(display), "'%c'", (char)i);
        else
            snprintf(display, sizeof(display), "0x%02X", i);

        printf("%-10d %-12s %-8d %s\n", i, display, table[i].length, table[i].bits);
    }
    printf("\n");
}

int huffman_count_symbols(uint64_t frequencies[ALPHABET_SIZE])
{
    int count = 0;
    for (int i = 0; i < ALPHABET_SIZE; i++)
        if (frequencies[i] > 0) count++;
    return count;
}
