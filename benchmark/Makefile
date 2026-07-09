# ============================================================
# benchmark/Makefile
#
# Self-contained benchmark for the Huffman compressor.
# Usage:
#   make            → build bench binary
#   make inputs     → generate all input fixtures
#   make run        → build + generate inputs + run benchmark
#   make clean      → remove build artefacts and temp files
#   make cleanall   → also delete generated input files
# ============================================================

CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=199309L
TARGET  = bench

# The compress binary lives one level up; make sure it is built first.
COMPRESS_ROOT = ..
COMPRESS_BIN  = $(COMPRESS_ROOT)/compress

.PHONY: all
all: $(TARGET)

# ---- build the benchmark binary ------------------------------------ #
$(TARGET): bench.c
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Built: ./$(TARGET)"

# ---- generate input fixtures --------------------------------------- #
.PHONY: inputs
inputs:
	@echo "Generating benchmark inputs..."
	@bash gen_inputs.sh

# ---- ensure the compress binary is up to date ---------------------- #
.PHONY: compress
compress:
	$(MAKE) -C $(COMPRESS_ROOT)

# ---- full run: build compress → generate inputs → run bench -------- #
.PHONY: run
run: compress all inputs
	@echo ""
	@./$(TARGET)

# ---- clean up artefacts (keep inputs so you don't regenerate) ------ #
.PHONY: clean
clean:
	rm -f $(TARGET) tmp_*.huff tmp_*.restored

# ---- nuke everything including generated inputs -------------------- #
.PHONY: cleanall
cleanall: clean
	rm -rf inputs/

# ---- quick help ---------------------------------------------------- #
.PHONY: help
help:
	@echo "Targets:"
	@echo "  all        Build the bench binary"
	@echo "  inputs     Generate benchmark input files"
	@echo "  compress   (Re)build the main compressor"
	@echo "  run        Build everything and run the benchmark"
	@echo "  clean      Remove bench binary and tmp files"
	@echo "  cleanall   Also remove generated input files"
