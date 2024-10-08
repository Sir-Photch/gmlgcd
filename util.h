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

#pragma once

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/un.h>
#include <netinet/in.h>

union sockaddrs {
	struct sockaddr_un un;
	struct sockaddr_in in;
	struct sockaddr_in6 in6;
};

bool  path_combine(char *, size_t, const char *, const char *);
char *trim_whitespace(char *s);
bool  sockaddrs_to_str(char *, socklen_t, const union sockaddrs *, int);
char *strrep(const char *, ...);
