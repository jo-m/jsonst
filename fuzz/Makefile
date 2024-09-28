CC = afl-cc
CFLAGS = -g -O2 -std=gnu17 -Wall -Werror -Wextra -Wpedantic -Wshadow -Wsign-compare -Wunreachable-code -Wno-gnu-zero-variadic-macro-arguments -Wno-strict-prototypes -DNDEBUG

fuzztarget: Makefile fuzztarget.c ../src/jsonst.c ../src/jsonst.h ../src/jsonst_cstd.h ../src/jsonst_util.c ../src/jsonst_util.h
	$(CC) $(CFLAGS) -c fuzztarget.c -o fuzztarget.o
	$(CC) fuzztarget.o -o fuzztarget

.PHONY: clean
clean:
	-rm -f fuzztarget.o fuzztarget
