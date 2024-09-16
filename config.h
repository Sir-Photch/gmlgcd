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

#include <stdbool.h>
#include <stddef.h>
#include <netinet/in.h>

struct config {
	char *uri_subpath;
	char *comments_dir;
	char *persistent_dir;

	char *help_template;

	sa_family_t af;
	union {
		char *runtime_dir;
		struct {
			union {
				struct in_addr  v4;
				struct in6_addr v6;
			} ip;
			unsigned short port;
		} tcp;
	} listen;

	struct {
		struct {
			size_t n;
			char **p;
		} verbs;

		size_t lines_max;
		size_t username_max;

		bool allow_links;
		enum authmode {
			NONE, REQUIRE_USERNAME, REQUIRE_CERT
		} auth;
	} comment;

	bool danger_no_sandbox;
};

void config_parse(struct config *, int, char *const *);
void config_free(struct config *);
