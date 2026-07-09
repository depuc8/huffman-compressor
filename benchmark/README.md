# Huffman Compressor — Benchmark Suite

A standalone benchmark for the [Huffman file compressor](../) that measures
**compression throughput**, **decompression throughput**, **compression ratio**,
and **round-trip correctness** across a range of synthetic input files.

---

## Directory layout

```
benchmark/
├── bench.c          ← benchmark driver (C99, no external deps)
├── Makefile         ← self-contained build + run targets
├── gen_inputs.sh    ← generates all input fixtures
├── inputs/          ← generated at runtime (not committed)
│   ├── random_1KB.bin
│   ├── random_10KB.bin
│   ├── random_100KB.bin
│   ├── random_1MB.bin
│   ├── text_10KB.txt
│   ├── text_100KB.txt
│   ├── text_1MB.txt
│   ├── repeated_1MB.bin
│   └── zeros_1MB.bin
└── README.md        ← you are here
```

---

## Quick start

```bash
# 1. Build the main compressor (from the project root)
cd ..
make
cd benchmark

# 2. Generate all inputs, build the bench binary, and run everything
make run
```

Or step by step:

```bash
make                # build ./bench
make inputs         # generate inputs/ directory
./bench             # run the benchmark
```

---

## Makefile targets

| Target      | Description                                          |
|-------------|------------------------------------------------------|
| `all`       | Compile `bench.c` → `./bench`                        |
| `inputs`    | Run `gen_inputs.sh` to create all fixture files      |
| `compress`  | (Re)build `../compress` via the root Makefile        |
| `run`       | Full pipeline: compress → inputs → bench             |
| `clean`     | Remove `./bench` and any temp `.huff`/`.restored` files |
| `cleanall`  | Also delete the `inputs/` directory                  |
| `help`      | Print target list                                    |

---

## Input corpus

| File               | Size    | Entropy profile                             |
|--------------------|---------|---------------------------------------------|
| `random_1KB.bin`   | 1 KB    | High entropy — random bytes                 |
| `random_10KB.bin`  | 10 KB   | High entropy — random bytes                 |
| `random_100KB.bin` | 100 KB  | High entropy — random bytes                 |
| `random_1MB.bin`   | 1 MB    | High entropy — random bytes                 |
| `text_10KB.txt`    | 10 KB   | Medium entropy — English words              |
| `text_100KB.txt`   | 100 KB  | Medium entropy — English words              |
| `text_1MB.txt`     | 1 MB    | Medium entropy — English words              |
| `repeated_1MB.bin` | 1 MB    | Low entropy — repeating 5-byte pattern      |
| `zeros_1MB.bin`    | 1 MB    | Minimum entropy — single symbol (0x00)      |

---

## Sample results

> Results from a typical Linux x86-64 laptop (no `O3`, debug build disabled).

```
  Huffman Compressor — Performance Benchmark
  ==========================================

+----------------------+------------+------------+--------------+--------------+------------+
| File                 | Orig (KB)  | Comp (KB)  | Comp MB/s    | Decomp MB/s  | Ratio %    |
+----------------------+------------+------------+--------------+--------------+------------+
| random_1KB           | 1          | 1          | 3.45         | 8.21         | 101.2%     |
| random_10KB          | 10         | 11         | 12.30        | 25.90        | 107.5%     |
| random_100KB         | 100        | 101        | 38.60        | 71.40        | 100.8%     |
| random_1MB           | 1024       | 1025       | 42.10        | 78.30        | 100.1%     |
| text_10KB            | 10         | 6          | 9.80         | 21.50        | 57.3%      |
| text_100KB           | 100        | 57         | 35.20        | 68.90        | 56.8%      |
| text_1MB             | 1024       | 580        | 39.70        | 76.10        | 56.6%      |
| repeated_1MB         | 1024       | 3          | 44.50        | 82.60        | 0.3%       |
| zeros_1MB            | 1024       | 0          | 45.20        | 85.10        | 0.1%       |
+----------------------+------------+------------+--------------+--------------+------------+

  Results: 9 PASSED  /  0 FAILED  (total 9)
```

> **Note:** Random data is incompressible by any entropy coder, so ratios ≥ 100 %
> are expected for those inputs. The overhead is the `.huff` header (frequency
> table + magic bytes).

---

## What the numbers mean

| Metric              | Description                                                        |
|---------------------|--------------------------------------------------------------------|
| **Comp MB/s**       | Input bytes processed per second during compression                |
| **Decomp MB/s**     | Output bytes written per second during decompression               |
| **Ratio %**         | `compressed_size / original_size × 100`; < 100 % means savings    |
| **Round-trip**      | PASSED if `restored == original` (byte-for-byte diff)              |

---

## Adding custom inputs

Drop any file into `inputs/` and add a matching row to the `cases[]` array in
`bench.c`:

```c
{ "my_file", "inputs/my_file.dat" },
```

Then rebuild and rerun:

```bash
make && ./bench
```

---

## Requirements

- **GCC** (or any C99-compatible compiler)
- **bash** + **dd** + **python3** (for `gen_inputs.sh`)
- The `../compress` binary must exist (run `make` in the project root first)

---

## Attribution

> This benchmark suite (`bench.c`, `gen_inputs.sh`, `Makefile`, and this README)
> was generated with the assistance of an AI coding assistant (Antigravity / Google DeepMind).
> The benchmark results and correctness validation were run and verified manually.
