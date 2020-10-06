/*
 * UNG's Not GNU
 *
 * Copyright (c) 2020, Jakob Kaivo <jkk@ung.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define _XOPEN_SOURCE 700
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifndef LINE_MAX
#define LINE_MAX _POSIX_LINE_MAX
#endif

enum {
	COUNT = (1<<0),
	DUPLICATES = (1<<1),
	UNIQUES = (1<<2),
};

static void print_line(FILE *out, const char *line, uintmax_t count, unsigned int flags)
{
	if (count == 0) {
		return;
	}

	if ((flags & DUPLICATES) && (count == 1)) {
		return;
	}

	if ((flags & UNIQUES) && (count != 1)) {
		return;
	}

	if (flags & COUNT) {
		fprintf(out, "%ju ", count);
	}
	fprintf(out, "%s", line);
}

static char *skip_fields(const char *s, size_t fields)
{
	while (fields-- > 0) {
		if (*s == '\0') {
			break;
		}

		while (*s && !isblank(*s)) {
			s++;
		}

		while (*s && isblank(*s)) {
			s++;
		}
	}
	return (char*)s;
}

static char *skip_chars(const char *s, size_t chars)
{
	while (chars-- > 0) {
		if (*s == '\0') {
			break;
		}
		s++;
	}
	return (char*)s;
}

static int are_duplicates(const char *s1, const char *s2, size_t fields, size_t chars)
{
	s1 = skip_fields(s1, fields);
	s2 = skip_fields(s2, fields);

	s1 = skip_chars(s1, chars);
	s2 = skip_chars(s2, chars);

	return strcmp(s1, s2) == 0;
}

static int uniq(const char *input, const char *output, size_t fields, size_t chars, unsigned int flags)
{
	(void)fields;
	(void)chars;
	uintmax_t count = 0;
	char cur[LINE_MAX] = "";
	char prev[LINE_MAX] = "";

	FILE *in = stdin;
	if (strcmp(input, "-")) {
		in = fopen(input, "r");
	}
	if (in == NULL) {
		fprintf(stderr, "uniq: %s: %s\n", input, strerror(errno));
		return 1;
	}

	FILE *out = stdout;
	if (strcmp(output, "-")) {
		out = fopen(output, "w");
	}
	if (out == NULL) {
		fprintf(stderr, "uniq: %s: %s\n", output, strerror(errno));
		return 1;
	}

	while (fgets(cur, sizeof(cur), in) != NULL) {
		if (are_duplicates(cur, prev, fields, chars)) {
			count++;
			continue;
		}
		
		print_line(out, prev, count, flags);
		strcpy(prev, cur);
		count = 1;
	}
	print_line(out, prev, count, flags);

	fclose(out);
	fclose(in);

	return 0;
}

int main(int argc, char *argv[])
{
	uintmax_t chars = 0;
	uintmax_t fields = 0;
	unsigned int flags = 0;
	char *input = "-";
	char *output = "-";

	/* TODO: handle + prefixed options */

	int c;
	while ((c = getopt(argc, argv, "cduf:s:")) != -1) {
		switch (c) {
		case 'c':
			flags |= COUNT;
			break;

		case 'd':
			flags |= DUPLICATES;
			break;

		case 'u':
			flags |= UNIQUES;
			break;

		case 'f':
			fields = strtoumax(optarg, NULL, 0);
			/* TODO: verify < SIZE_MAX */
			break;

		case 's':
			chars = strtoumax(optarg, NULL, 0);
			/* TODO: verify < SIZE_MAX */
			break;
		}
	}

	if (argc > optind + 2) {
		fprintf(stderr, "uniq: too many operands\n");
		return 1;
	}

	if (argc > optind + 1) {
		output = argv[optind + 1];
	}

	if (argc > optind) {
		input = argv[optind];
	}

	if (!strcmp(input, output) && !strcmp(input, "-")) {
		fprintf(stderr, "uniq: output clobbers input, bailing\n");
		return 1;
	}

	return uniq(input, output, fields, chars, flags);
}
