#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

enum state {
	BEGIN,
	WAIT,
	BOUNDARY,
	MIDDLE,
};

struct parser {
	enum state state;
	unsigned char *buf;
	size_t len;
	int n_digits;
	char digits[12];
};

static enum state update_state(int c, enum state state)
{
	if (c == -1 || isspace(c)) {
		if (state == WAIT || state == BEGIN) {
			return BOUNDARY;
		} else if (state == MIDDLE) {
			return BOUNDARY;
		}
	} else if (isdigit(c)) {
		if (state == BOUNDARY) {
			return MIDDLE;
		} else if (state == BEGIN) {
                        return MIDDLE;
                }
	}

	if (state == BOUNDARY || state == BEGIN) {
		return WAIT;
	}

	return state;
}

static enum state doaction(int c, enum state new_state, struct parser *parser)
{
        char charbuf[2];
        unsigned ts;

        if (c == -1)
                charbuf[0] = 0;
        else {
                charbuf[0] = (char)c;
                charbuf[1] = 0;
        }

        if (parser->state == MIDDLE && new_state == BOUNDARY) {
                ts = strtoul(parser->digits, NULL, 10);
                /* found date */
                printf("[%d]%s", ts, charbuf);
        } else if (new_state == MIDDLE) {
                if (parser->n_digits < 10) {
                        parser->digits[parser->n_digits++] = (char)c;
                        parser->digits[parser->n_digits] = 0;
                } else {
                        new_state = WAIT;
                        printf("%.*s%s", parser->n_digits, parser->digits, charbuf);
                        parser->n_digits = 0;
                }
        } else {
                if (c != -1)
                        fputc(c, stdout);
        }

        return new_state;
}

/* static const char *state_name(enum state state) */
/* { */
/*         switch (state) { */
/*         case BOUNDARY: return "BOUNDARY"; */
/*         case WAIT: return "WAIT"; */
/*         case MIDDLE: return "MIDDLE"; */
/*         case BEGIN: return "BEGIN"; */
/*         } */
/*         return "UNKN"; */
/* } */



static void process(struct parser *parser, int last)
{
	enum state new_state;
	int i;
	int c;

	for (i = 0; i < parser->len+last; i++) {
                if (last && i == parser->len)
                        c = -1;
		else
                        c = parser->buf[i];

		/* transition state */
		new_state = update_state(c, parser->state);

                /* debug */
                /* printf("%d | %s -> %s\n", c, state_name(parser->state), state_name(new_state)); */

		/* action */
                new_state = doaction(c, new_state, parser);

		if (new_state == BOUNDARY) {
			parser->n_digits = 0;
		}

		parser->state = new_state;
	}
}

#define READSIZE 4096

static void parser_init(struct parser *parser)
{
	static unsigned char buf[READSIZE];
	parser->buf = buf;
	parser->state = BEGIN;
	parser->len = 0;
	parser->n_digits = 0;
}

int main(int argc, const char *argv[])
{
	struct parser parser;
	parser_init(&parser);

	do {
		parser.len = fread(parser.buf, 1, READSIZE, stdin);
		process(&parser, parser.len != READSIZE);
	} while (parser.len == READSIZE);

	return 0;
}
