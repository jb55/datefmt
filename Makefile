
CFLAGS = -Wall -Werror -std=c99

datefmt: datefmt.c
	$(CC) $(CFLAGS) $< -o $@

clean: fake
	rm -f datefmt

.PHONY: fake
