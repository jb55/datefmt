
CFLAGS = -g -Wall -Werror -std=c99
PREFIX ?= /usr

datefmt: datefmt.c
	$(CC) $(CFLAGS) $< -o $@

install: datefmt
	mkdir -p $(PREFIX)/bin
	cp datefmt $(PREFIX)/bin

clean: fake
	rm -f datefmt

.PHONY: fake
