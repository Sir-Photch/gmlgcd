#include "../util.h"

#include <stdio.h>
#include <bsd/string.h>

#define BUFSIZE 2048

int
trim_stdin(char buf[BUFSIZE])
{
	fprintf(stdout, "%s", trim_whitespace(buf));

	return 0;
}

int
path_combine_stdin(char buf[BUFSIZE])
{
	char a[BUFSIZE/2], b[BUFSIZE/2], pbuf[PATH_MAX], *spc;

	spc = strchr(buf, ' ');
	*spc = '\0';

	strlcpy(a, buf, sizeof(a));
	strlcpy(b, spc + 1, sizeof(b));

	if (!path_combine(pbuf, PATH_MAX, a, b))
		return 1;

	fprintf(stdout, "%s", pbuf);

	return 0;
}

int
main(int argc, char **argv)
{
	(void)argc;

	char buf[BUFSIZE];

	if (!fgets(buf, sizeof(buf), stdin)) {
		fprintf(stderr, "bad input");
		return 1;
	}

	if (strcmp(argv[1], "trim") == 0)
		return trim_stdin(buf);
	else if (strcmp(argv[1], "combine") == 0)
		return path_combine_stdin(buf);
	else {
		fprintf(stderr, "usage");
		return 1;
	}
}
