#include "../util.h"

#include <stdio.h>
#include <bsd/string.h>
#include <assert.h>

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
strrep_test(void)
{
	const char *ret;
	static const char test0[] = "hello my name is <name> and i am <verb> to meet you";

	ret = strrep(test0, "<name>", "peter", "<verb>", "pleased", (char *)NULL);

	if (strcmp(ret, "hello my name is peter and I am pleased to meet you") != 0)
		return 1;

	return 0;
}

int
main(int argc, char **argv)
{
	(void)argc;

	char buf[BUFSIZE];

	fgets(buf, sizeof(buf), stdin);

	if (strcmp(argv[1], "trim") == 0)
		return trim_stdin(buf);
	else if (strcmp(argv[1], "combine") == 0)
		return path_combine_stdin(buf);
	else if (strcmp(argv[1], "strrep") == 0) 
		return strrep_test();
	else {
		fprintf(stderr, "usage");
		return 1;
	}
}
