
CFLAGS = -Wall -Werror -std=c89

datefmt: datefmt.c
	$(CC) $(CFLAGS) $< -o $@

clean: fake
	rm -f datefmt

.PHONY: fake
