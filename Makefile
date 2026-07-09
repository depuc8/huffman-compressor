CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -Wpedantic -g -Iinclude
TARGET  = compress

SRCS = src/main.c src/huffman.c src/heap.c src/bitio.c src/fileio.c
OBJS = $(SRCS:.c=.o)

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
	@echo "Build OK -> ./$(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Run a compress/decompress round-trip and diff the result.
.PHONY: test
test: all
	./$(TARGET) -c test/sample.txt test/sample.huff
	./$(TARGET) -d test/sample.huff test/restored.txt
	diff test/sample.txt test/restored.txt && echo "PASS" || echo "FAIL"

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET) test/sample.huff test/restored.txt

.PHONY: help
help:
	@echo "Targets: all  test  clean  help"
