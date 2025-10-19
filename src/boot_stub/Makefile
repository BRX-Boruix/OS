# Makefile for Limine Boot Stub

CC = gcc
LD = ld

CFLAGS = -m64 -std=c99 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
         -Wall -Wextra -mcmodel=kernel -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -fno-pic \
         -O2

LDFLAGS = -m elf_x86_64 -T stub.ld

STUB_BIN = ../../build/stub.bin

all: $(STUB_BIN)

$(STUB_BIN): stub.o stub.ld
	$(LD) $(LDFLAGS) -o $@ stub.o

stub.o: stub.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f stub.o $(STUB_BIN)

.PHONY: all clean
