# huffman-compressor

Command-line file compressor written in C (C99) using Huffman Encoding — built from scratch with no external dependencies.

## Build

```bash
make
```

## Usage

```bash
# Compress
./compress -c input.txt output.huff

# Decompress
./compress -d output.huff restored.txt

# Round-trip test
make test
```

## Project Structure

```
src/
  main.c      — CLI entry point
  huffman.c   — tree construction and code generation
  heap.c      — min-heap priority queue
  bitio.c     — bit-level I/O
  fileio.c    — compress / decompress pipelines
include/      — header files
test/         — sample input
```

## Concepts Covered

- Binary tree & recursive traversal
- Min-heap priority queue
- Bit manipulation and file I/O
- Dynamic memory allocation
- Modular C with header files
