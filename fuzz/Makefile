DEPS = fuzztarget.c ../src/jsonst.c ../src/include/jsonst.h ../src/include/jsonst_cstd.h ../src/jsonst_util.c ../src/include/jsonst_util.h

CFLAGS = -g -O2 -std=gnu17 -I../src/include/ -Wall -Werror -Wextra -Wpedantic -Wshadow -Wsign-compare -Wunreachable-code

CLAGS_FUZZ = -DNDEBUG

fuzztarget: CC = afl-cc
fuzztarget: CFLAGS += $(CFLAGS_FUZZ)
fuzztarget: Makefile $(DEPS)
	$(CC) $(CFLAGS) -c fuzztarget.c -o fuzztarget.o
	$(CC) fuzztarget.o -o fuzztarget

.PHONY: clean
clean:
	-rm -f fuzztarget.o fuzztarget
