#ifndef HEAP_H
#define HEAP_H

#include "huffman.h"

/* Min-heap of HuffmanNode pointers, backed by a dynamic array.
   The node with the lowest frequency is always at index 0. */
typedef struct {
    HuffmanNode **nodes;
    int           size;
    int           capacity;
} MinHeap;

/* Create an empty heap with the given initial capacity. Returns NULL on failure. */
MinHeap *heap_create(int capacity);

/* Free the heap struct (does not free the nodes inside). */
void heap_free(MinHeap *heap);

/* Insert a node into the heap. Resizes automatically if full.
   Returns 0 on success, -1 on allocation failure. */
int heap_insert(MinHeap *heap, HuffmanNode *node);

/* Remove and return the node with the smallest frequency.
   Returns NULL if the heap is empty. */
HuffmanNode *heap_extract_min(MinHeap *heap);

/* Return the number of elements currently in the heap. */
int heap_size(const MinHeap *heap);

#endif /* HEAP_H */
