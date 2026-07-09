#!/usr/bin/env bash
# gen_inputs.sh — Generate all benchmark input files.
#
# Requires: dd, /dev/urandom, /dev/zero, python3 (or perl) for text.
# Run once before executing the benchmark:
#   cd benchmark && bash gen_inputs.sh

set -euo pipefail

INPUTS="inputs"
mkdir -p "$INPUTS"

echo "[gen] Creating random binary files..."
dd if=/dev/urandom bs=1024        count=1    of="$INPUTS/random_1KB.bin"    status=none
dd if=/dev/urandom bs=1024        count=10   of="$INPUTS/random_10KB.bin"   status=none
dd if=/dev/urandom bs=1024        count=100  of="$INPUTS/random_100KB.bin"  status=none
dd if=/dev/urandom bs=1048576     count=1    of="$INPUTS/random_1MB.bin"    status=none

echo "[gen] Creating zero-byte file (best-case for single-symbol)..."
dd if=/dev/zero   bs=1048576     count=1    of="$INPUTS/zeros_1MB.bin"     status=none

echo "[gen] Creating low-entropy repeated-pattern file..."
python3 -c "
import sys
pat = b'ABCDE' * 205
data = pat * (1048576 // len(pat) + 1)
sys.stdout.buffer.write(data[:1048576])
" > "$INPUTS/repeated_1MB.bin"

echo "[gen] Creating realistic English text files..."
# Use /usr/share/dict/words if available, else fall back to python lorem.
WORDS="/usr/share/dict/words"

gen_text() {
    local size=$1
    local out=$2
    if [ -f "$WORDS" ]; then
        # Cycle through the word list until we reach the target size.
        python3 -c "
import sys, itertools
words = open('$WORDS').read().split()
cycle = itertools.cycle(words)
buf = []
total = 0
target = $size
while total < target:
    w = next(cycle) + ' '
    buf.append(w)
    total += len(w)
sys.stdout.write(''.join(buf)[:target])
" > "$out"
    else
        # Fallback: generate pseudo-random printable ASCII text.
        python3 -c "
import sys, random, string
random.seed(42)
chars = string.ascii_letters + string.digits + '     \n'  # weighted spaces
data = ''.join(random.choices(chars, k=$size))
sys.stdout.write(data)
" > "$out"
    fi
}

gen_text 10240   "$INPUTS/text_10KB.txt"
gen_text 102400  "$INPUTS/text_100KB.txt"
gen_text 1048576 "$INPUTS/text_1MB.txt"

echo ""
echo "Done. Input files:"
ls -lh "$INPUTS"
