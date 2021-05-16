#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include <time.h>
#include <string.h>

#define VERSION "0.1.1"

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
	int64_t after, before;
	int n_digits;
	int dir;
	const char *format;
	char digits[12];
};

static enum state update_state(int c, enum state state)
{
	int is_boundary = c == -1 || !isalnum(c);

	if (is_boundary) {
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

	if (state == BEGIN) {
		return BOUNDARY;
	}

	return state;
}

static void print_rest(struct parser *parser, char *charbuf, enum state *new_state)
{
	*new_state = BOUNDARY;
	printf("%.*s%s", parser->n_digits, parser->digits, charbuf);
	parser->n_digits = 0;
}

static int time_matches(struct parser *parser, int64_t ts)
{
	// always match after
	if (ts <= parser->after)
		return 0;

	// match before if it's set
	if (parser->before != -1 && ts >= parser->before) {
		return 0;
	}

	if (parser->dir == -1) {
		return ts <= time(NULL);
	} else if (parser->dir == 1) {
		return ts > time(NULL);
	}

	// dir == 0 has no conditions on now
	return 1;
}

static enum state doaction(int c, enum state new_state, struct parser *parser)
{
	static char timebuf[128];
	char charbuf[2];
	int64_t ts;

	if (c == -1)
		charbuf[0] = 0;
	else {
		charbuf[0] = (char)c;
		charbuf[1] = 0;
	}

	if (parser->state == MIDDLE && new_state == BOUNDARY) {
		ts = strtoll(parser->digits, NULL, 10);
		/* found date */
		if (time_matches(parser, ts)) {
			strftime(timebuf, sizeof(timebuf), parser->format, localtime(&ts));
			printf("%s%s", timebuf, charbuf);
		} else {
			print_rest(parser, charbuf, &new_state);
		}
	} else if (new_state == MIDDLE) {
		if (parser->n_digits < 10) {
			parser->digits[parser->n_digits++] = (char)c;
			parser->digits[parser->n_digits] = 0;
		} else {
			print_rest(parser, charbuf, &new_state);
		}
	} else {
		if (c != -1)
			fputc(c, stdout);
	}

	return new_state;
}

/* static const char *state_name(enum state state) */
/* { */
/*	 switch (state) { */
/*	 case BOUNDARY: return "BOUNDARY"; */
/*	 case WAIT: return "WAIT"; */
/*	 case MIDDLE: return "MIDDLE"; */
/*	 case BEGIN: return "BEGIN"; */
/*	 } */
/*	 return "UNKN"; */
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
		/* printf("%c | %s -> %s\n", (char)c, state_name(parser->state), state_name(new_state)); */

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
	parser->after = 946684800LL;
	parser->before = -1;
	parser->dir = 0;
	parser->n_digits = 0;
	parser->format = "%F %R";
}

static void usage() {
	printf("usage: datefmt [OPTION...] [FORMAT]\n\n");
	printf("format unix timestamps from stdin\n\n");
	printf("  -a, --after <timestamp>  only format timestamps after this date \n");
	printf("  -b, --before <timestamp> only format timestamps before this date \n");
	printf("  -f, --future             only format timestamps in the future \n");
	printf("  -p, --past               only format timestamps in the past \n");
	printf("      --version	           display version information and exit \n");

	printf("\n  FORMAT\n    a strftime format string, defaults to '%%F %%R'\n");

	printf("\n  EXAMPLE\n    datefmt --after $(date -d yesterday +%%s) %%R < spreadsheet.csv\n\n");
	printf("  Created By: William Casarin <https://jb55.com>\n");
	exit(0);
}


static void shift_arg(int *argc, char **argv)
{
	if (*argc < 1)
		return;
	memmove(argv, &argv[1], ((*argc)-1) * sizeof(char*));
	*argc = *argc - 1;
}

static time_t parse_date_arg(int *argc, char **argv)
{
	char *endptr;
	time_t res;

	shift_arg(argc, argv);
	if (*argc > 0) {
		res = strtoll(argv[0], &endptr, 10);
		if (endptr == argv[0]) {
			printf("error: invalid after value '%s'\n", argv[0]);
			exit(1);
		}
		shift_arg(argc, argv);
	} else {
		printf("error: expected argument to --after\n");
		exit(1);
	}

	return res;
}

static void parse_arg(int *argc, char **argv, struct parser *parser)
{
	if (!strcmp(argv[0], "--after") || !strcmp(argv[0], "-a")) {
		parser->after = parse_date_arg(argc, argv);
	} else if (!strcmp("--help", argv[0])) {
		usage();
	} else if (!strcmp("--before", argv[0]) || !strcmp(argv[0], "-b")) {
		parser->before = parse_date_arg(argc, argv);
	} else if (!strcmp("--version", argv[0])) {
		printf("datefmt " VERSION "\n");
		exit(0);
	} else if (!strcmp("--past", argv[0]) || !strcmp("-p", argv[0])) {
		parser->dir = -1;
		shift_arg(argc, argv);
	} else if (!strcmp("--future", argv[0]) || !strcmp("-f", argv[0])) {
		parser->dir = 1;
		shift_arg(argc, argv);
	} else {
		parser->format = (const char*)argv[0];
		shift_arg(argc, argv);
	}
}

static void parse_args(int argc, char **argv, struct parser *parser)
{
	shift_arg(&argc, argv);

	while (argc > 0) {
		parse_arg(&argc, argv, parser);
	}
}

int main(int argc, char *argv[])
{
	struct parser parser;

	parser_init(&parser);
	parse_args(argc, argv, &parser);

	do {
		parser.len = fread(parser.buf, 1, READSIZE, stdin);
		process(&parser, parser.len != READSIZE);
	} while (parser.len == READSIZE);

	return 0;
}
