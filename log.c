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

#include <stdarg.h>
#ifdef DEBUG_BUILD
#	include <errno.h>
#endif

#include "log.h"

bool __log_verbose;

void
msg(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	printf("%s: ", getprogname());
	vprintf(fmt, ap);
	printf("\n");

	va_end(ap);
}

void
dbg(const char *fmt, ...)
{
#ifdef DEBUG_BUILD
	va_list ap;
	va_start(ap, fmt);

	printf("%s: ", getprogname());
	vprintf(fmt, ap);
	printf(":%s\n", strerror(errno));

	va_end(ap);
#else
	(void)fmt;
#endif
}

void
dbgx(const char *fmt, ...)
{
#ifdef DEBUG_BUILD
	va_list ap;
	va_start(ap, fmt);

	printf("%s: ", getprogname());
	vprintf(fmt, ap);
	printf("\n");

	va_end(ap);
#else
	(void)fmt;
#endif
}
