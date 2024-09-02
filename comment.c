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

#include <time.h>
#include <ctype.h>

#include "config.h"
#include "comment.h"
#include "log.h"
#include "replies.h"
#include "util.h"

static const char CN_PREFIX[] = "/CN=";

#define DEFAULT_COMMENT_VERBS_LEN 4
static const char *DEFAULT_COMMENT_VERBS[DEFAULT_COMMENT_VERBS_LEN] = {
	"says", "thinks", "argues", "writes"
};

bool
commentf(char formatted_comment[COMMENTS_MAX], const struct config *cfg,
    unsigned short rid,
    struct user_input user, bool allow_links, const char **errstatus)
{
	const char **comment_verbs = cfg->comment_verbs ?
	    (const char **)cfg->comment_verbs : DEFAULT_COMMENT_VERBS;
	size_t comment_verbs_len = cfg->comment_verbs ?
	    cfg->comment_verbs_len : DEFAULT_COMMENT_VERBS_LEN;
	struct tm utc;
	char *col;
	time_t now;
	size_t n_lines;
	ssize_t nontruncated_len;
	const char *message, *nextline, *username;

	*errstatus = NULL;
	username = NULL;

	col = strnchr(user.gemini_search_string, cfg->username_max, ':');

	if (col && col[1] == ' ') {
		*col = '\0';
		username = trim_whitespace(user.gemini_search_string);
	} else if (user.name) {
		if (strstr(user.name, CN_PREFIX))
			username = user.name + sizeof(CN_PREFIX) - 1;
		else
			username = user.name;
	} else {
		warnxli(rid, "username missing");
		*errstatus = USERNAME_MISSING;
		return false;
	}

	if (col)
		message = col + 1;
	else
		message = user.gemini_search_string;

	while (*message != '\0' && isspace(*message))
		message++;

	if (*message == '\0') {
		warnxli(rid, "empty comment from %s", username);
		*errstatus = EMPTY_COMMENT;
		return false;
	}

	nextline = message;
	n_lines = 0;
	do {
		if (++n_lines > cfg->lines_max) {
			warnxli(rid, "too many lines (%lu) from %s", n_lines,
			    username);
			*errstatus = TOO_MANY_LINES;
			return false;
		}
		switch (*nextline) {
		case '#':
			warnxli(rid, "headers from %s", username);
			*errstatus = HEADERS_NOT_ALLOWED;
			return false;
		case '=':
			if (!allow_links && nextline[1] == '>') {
				*errstatus = LINKS_NOT_ALLOWED;
				return false;
			}
			break;
		default:
			break;
		}
	} while ((nextline = strchr(nextline, '\n')) &&
	    *(nextline += 1) != '\0');

	time(&now);
	gmtime_r(&now, &utc);

	nontruncated_len = snprintf(formatted_comment, COMMENTS_MAX,
	    "### %s (%s) %s:\n%s\n--- %d-%02d-%02d %d:%02d (UTC)\n\n", username,
	    user.id.hash, comment_verbs[rand() % comment_verbs_len],
	    message, utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday,
	    utc.tm_hour, utc.tm_min);

//	nontruncated_len = snprintf(formatted_comment, COMMENTS_MAX,
//	    "## %s (%s) %s:\n### %d-%02d-%02d %d:%02d (UTC)\n%s\n\n", username,
//	    user.id.hash, COMMENT_VERBS[rand() % COMMENT_VERBS_LEN],
//	    utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday, utc.tm_hour,
//	    utc.tm_min, message);

//	nontruncated_len = snprintf(formatted_comment, COMMENTS_MAX,
//	    "### On %d-%02d-%02d %d:%d (UTC), %s (%s) %s:\n%s\n\n", utc.tm_year + 1900,
//	    utc.tm_mon + 1, utc.tm_mday, utc.tm_hour, utc.tm_min, username,
//	    hash, COMMENT_VERBS[rand() % COMMENT_VERBS_LEN],
//	    message);

	if (nontruncated_len > COMMENTS_MAX) {
		warnxli(rid, "COMMENTS_MAX exceeded by %s (%s)", username,
		    user.id.hash);
	}

	return true;
}
