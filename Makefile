
CFLAGS = -g -Wall -Werror -std=c99
PREFIX ?= /usr

datefmt: datefmt.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

install: datefmt
	mkdir -p $(PREFIX)/bin
	cp datefmt $(PREFIX)/bin

clean:
	rm -f datefmt

.PHONY: install clean
