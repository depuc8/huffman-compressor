#include <stdlib.h>
#include <stdio.h>
#include "heap.h"

/* Swap two node pointers. */
static void swap(HuffmanNode **a, HuffmanNode **b)
{
    HuffmanNode *tmp = *a;
    *a = *b;
    *b = tmp;
}

/* Move node at index i up until the heap property is restored. */
static void sift_up(MinHeap *heap, int i)
{
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (heap->nodes[parent]->frequency > heap->nodes[i]->frequency) {
            swap(&heap->nodes[parent], &heap->nodes[i]);
            i = parent;
        } else {
            break;
        }
    }
}

/* Move node at index i down until the heap property is restored. */
static void sift_down(MinHeap *heap, int i)
{
    int size = heap->size;

    while (1) {
        int smallest = i;
        int left  = 2 * i + 1;
        int right = 2 * i + 2;

        if (left < size &&
            heap->nodes[left]->frequency < heap->nodes[smallest]->frequency)
            smallest = left;

        if (right < size &&
            heap->nodes[right]->frequency < heap->nodes[smallest]->frequency)
            smallest = right;

        if (smallest != i) {
            swap(&heap->nodes[i], &heap->nodes[smallest]);
            i = smallest;
        } else {
            break;
        }
    }
}

MinHeap *heap_create(int capacity)
{
    if (capacity <= 0) {
        fprintf(stderr, "[heap] capacity must be > 0\n");
        return NULL;
    }

    MinHeap *heap = malloc(sizeof(MinHeap));
    if (!heap) {
        perror("[heap] malloc MinHeap");
        return NULL;
    }

    heap->nodes = malloc(sizeof(HuffmanNode *) * (size_t)capacity);
    if (!heap->nodes) {
        perror("[heap] malloc nodes array");
        free(heap);
        return NULL;
    }

    heap->size     = 0;
    heap->capacity = capacity;
    return heap;
}

void heap_free(MinHeap *heap)
{
    if (!heap) return;
    free(heap->nodes);
    free(heap);
}

int heap_insert(MinHeap *heap, HuffmanNode *node)
{
    /* Grow the array if we're out of space. */
    if (heap->size == heap->capacity) {
        int new_cap = heap->capacity * 2;
        HuffmanNode **tmp = realloc(heap->nodes,
                                    sizeof(HuffmanNode *) * (size_t)new_cap);
        if (!tmp) {
            perror("[heap] realloc");
            return -1;
        }
        heap->nodes    = tmp;
        heap->capacity = new_cap;
    }

    heap->nodes[heap->size] = node;
    heap->size++;
    sift_up(heap, heap->size - 1);
    return 0;
}

HuffmanNode *heap_extract_min(MinHeap *heap)
{
    if (heap->size == 0)
        return NULL;

    HuffmanNode *min_node = heap->nodes[0];

    /* Replace root with the last element and restore order. */
    heap->size--;
    heap->nodes[0] = heap->nodes[heap->size];
    sift_down(heap, 0);

    return min_node;
}

int heap_size(const MinHeap *heap)
{
    return heap ? heap->size : 0;
}
