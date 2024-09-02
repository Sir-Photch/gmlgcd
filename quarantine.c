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

#include "quarantine.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <limits.h>

#include "log.h"
#include "user.h"

#define SERIALIZED_QUARANTINE_FMT "%s|%s|%d|%d" // rhost, hash, last_t, n_failed

TAILQ_HEAD(quarantine_list, quarantine_entry);

static bool
quarantine_entry_equals(struct quarantine_entry a, struct quarantine_entry b)
{
	sa_family_t af = a.user.af;

	if (af != b.user.af)
		return false;

	switch (af) {
	case AF_INET:
		if (a.user.rhost.v4.s_addr != b.user.rhost.v4.s_addr)
			return false;
		break;
	case AF_INET6:
		if (memcmp(&a.user.rhost.v6, &b.user.rhost.v6,
		    sizeof(struct in6_addr)) != 0)
			return false;
		break;
	}

	return strcmp(a.user.hash, b.user.hash) == 0;
}

struct quarantine_list *
quarantine_new(void)
{
	struct quarantine_list *q;

	q = calloc(1, sizeof(struct quarantine_list));
	TAILQ_INIT(q);

	return q;
}

void
quarantine_free(struct quarantine_list **q)
{
	struct quarantine_entry *e1, *e2;

	e1 = TAILQ_FIRST(*q);
	while (e1 != NULL) {
		e2 = TAILQ_NEXT(e1, entries);
		free(e1);
		e1 = e2;
	}
	free(*q);
	q = NULL;
}

struct quarantine_entry *
quarantine_add(struct quarantine_list *q, const struct user_id *id)
{
	struct quarantine_entry *e;

	e = calloc(1, sizeof(struct quarantine_entry));

	e->user.rhost = id->rhost;
	e->user.af = id->af;
	memcpy(e->user.hash, id->hash, USER_HASH_LEN);

	TAILQ_INSERT_TAIL(q, e, entries);

#ifdef DEBUG_BUILD
    struct quarantine_entry *d;
    size_t len = 0;
    TAILQ_FOREACH(d, q, entries) len++;
    dbgxl("quarantine size: %lu", len);
#endif

	return e;
}

struct quarantine_entry *
quarantine_get_entry(struct quarantine_list *q, struct user_id id)
{
	struct quarantine_entry candidate = {
		.user = id,
	};
	struct quarantine_entry *e;

	TAILQ_FOREACH(e, q, entries) {
		if (quarantine_entry_equals(*e, candidate))
			return e;
	}

	return NULL;
}

void
quarantine_remove(struct quarantine_list *q, struct quarantine_entry *e)
{
	TAILQ_REMOVE(q, e, entries);

#ifdef DEBUG_BUILD
    struct quarantine_entry *d;
    size_t len = 0;
    TAILQ_FOREACH(d, q, entries) len++;
    dbgxl("quarantine size: %lu", len);
#endif
}

void
quarantine_serialize(struct quarantine_list *q, FILE *f)
{
	struct quarantine_entry *e;

	char inet_addr[INET6_ADDRSTRLEN];

	TAILQ_FOREACH(e, q, entries) {
		if (!inet_ntop(e->user.af, &e->user.rhost, inet_addr,
		    INET6_ADDRSTRLEN)) {
			warnl("inet_ntop");
			continue;
		}

		fprintf(f, "%s|%s|%lld|%lu\n", inet_addr, e->user.hash,
		    (long long)e->last_failure, e->failures);
	}
}

bool
quarantine_deserialize(struct quarantine_list *q, FILE *f)
{
	struct quarantine_entry *entry;
	char *delim, *hash, *line, *next, *rhost;
	const char *errstr;
	time_t time;
	size_t linelen, n_failed;
	bool success;

	line = NULL;
	success = true;

	while (getline(&line, &linelen, f) != -1) {
		if (!(delim = strchr(line, '|'))) {
			success = false;
			break;
		}
		*delim = '\0';
		rhost = line;
		next = delim + 1;

		if (!(delim = strchr(next, '|'))) {
			success = false;
			break;
		}
		*delim = '\0';
		hash = next;
		next = delim + 1;

		if (!(delim = strchr(next, '|'))) {
			success = false;
			break;
		}
		*delim = '\0';
		time = strtonum(next, 0, LONG_MAX, &errstr);
		if (errstr) {
			success = false;
			break;
		}
		next = delim + 1;

		if (!(delim = strchr(next, '\n'))) {
			success = false;
			break;
		}
		*delim = '\0';
		n_failed = strtonum(next, 0, LONG_MAX, &errstr);
		if (errstr) {
			success = false;
			break;
		}

		entry = calloc(1, sizeof(struct quarantine_entry));

		if (inet_pton(AF_INET, rhost, &entry->user.rhost.v4) == 1) {
			entry->user.af = AF_INET;
		} else if (inet_pton(AF_INET6, rhost, &entry->user.rhost.v6) ==
		    1) {
			entry->user.af = AF_INET6;
		} else {
			warnl("inet_pton");
			success = false;
			break;
		}
		strlcpy(entry->user.hash, hash, USER_HASH_LEN);
		entry->last_failure = time;
		entry->failures = n_failed;

		TAILQ_INSERT_TAIL(q, entry, entries);
	}

	if (line) {
		if (!success)
			warnxl("bad line: %s", line);

		free(line);
	}

	return success;
}

void
quarantine_entry_free(struct quarantine_entry **e)
{
	free(*e);
	*e = NULL;
}
