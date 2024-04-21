CC=gcc
CFLAGS=-Wall -Werror -Wvla -O0 -std=c11 -g -fsanitize=address,leak -D_POSIX_C_SOURCE=199309L
LDFLAGS=-lm
BINARIES=pe_exchange pe_trader

all: $(BINARIES)

.PHONY: clean
clean:
	rm -f $(BINARIES)

