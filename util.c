/**
 * gmlgcd - the gemlog comment daemon
 * Copyright (C) 2024 github.com/Sir-Photch
 *
 * This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "platform.h"

#include <cstdarg>
#include <errno.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdarg.h>

#include "util.h"

bool
path_combine(char *buf, size_t n, const char *prefix, const char *suffix)
{
	const char *slash;
	size_t len = 0;

	memset(buf, 0, n);

	if (prefix)
		len += strlcat(buf, prefix, n);

	if (suffix) {
		slash = strrchr(buf, '/');

		if (*suffix == '/')
			suffix += 1;

		if (prefix && (!slash || slash[1] != '\0'))
			len += strlcat(buf, "/", n);

		len += strlcat(buf, suffix, n);
	}

	return len < n;
}

char *
trim_whitespace(char *s)
{
	ssize_t n, i;
	char *start;

	if (!s)
		return NULL;

	n = strlen(s);

	if (n == 0)
		return s;

	start = s;

	for (i = 0; i < n; ++i) {
		if (!isspace(s[i])) {
			start = s + i;
			break;
		}
	}

	for (i = n - 1; start - s <= i; --i)
		if (isspace(s[i]))
			s[i] = '\0';
		else
			break;

	return start;
}

bool
sockaddrs_to_str(char *buf, socklen_t n, const union sockaddrs *s, int af)
{
	char port[6];
	int k;

	switch (af) {
	case AF_UNIX:
		return strlcpy(buf, s->un.sun_path, n) < n;
	case AF_INET:
		if (!inet_ntop(AF_INET, &s->in.sin_addr, buf, n))
			return false;
		if (strlcat(buf, ":", n) >= n)
			return false;
		if ((k = snprintf(memset(port, 0, sizeof(port)), sizeof(port),
		    "%u", ntohs(s->in.sin_port))) < 0 || k >= (int)sizeof(port))
			return false;
		if (strlcat(buf, port, n) >= n)
			return false;
		break;
	case AF_INET6:
		if (strlcat(buf, "[", n) >= n)
			return false;
		if (!inet_ntop(AF_INET6, &s->in6.sin6_addr, buf + 1, n - 1))
			return false;
		if (strlcat(buf, "]:", n) >= n)
			return false;
		if ((k = snprintf(memset(port, 0, sizeof(port)), sizeof(port),
		    "%u", ntohs(s->in6.sin6_port))) < 0 ||
		    k >= (int)sizeof(port))
			return false;
		if (strlcat(buf, port, n) >= n)
			return false;
		break;
	default:
		errno = EAFNOSUPPORT;
		return false;
	}

	return true;
}

/* c in first n characters of s */
char *
strnchr(const char *s, size_t n, char c)
{
	char *cp;

	if (!s)
		return NULL;

	cp = strchr(s, c);

	if (!cp)
		return NULL;

	if (cp - s > (ssize_t)n - 1)
		return NULL;

	return cp;
}

char *
strrep(const char *s, ...)
{
	const char *tok, *rep;
	char *str, *rest, *tokp, *tmp;
	size_t n;
	va_list args;

	va_start(args, s);

	str = strdup(s);

	while ((tok = va_arg(args, char *)) != NULL &&
	    (rep = va_arg(args, char *)) != NULL) {
		tokp = strstr(str, tok);

		if (!tokp)
			continue;

		*tokp = '\0';

		rest = tokp + strlen(tok);

		n = strlen(str) + strlen(rep) + strlen(rest) + 1;

		tmp = calloc(n, sizeof(char));

		strlcat(tmp, str, n);
		strlcat(tmp, rep, n);
		strlcat(tmp, rest, n);

		free(str);
		str = tmp;
	}

	va_end(args);

	return str;
}
