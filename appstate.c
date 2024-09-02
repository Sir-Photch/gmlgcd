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

#include <event2/event.h>
#include <limits.h>

#include "appstate.h"
#include "config.h"
#include "quarantine.h"
#include "log.h"
#include "util.h"

struct appstate *
appstate_new(int argc, char *const *argv)
{
	char pathbuf[PATH_MAX];
	struct appstate *s;
	FILE *f;

	s = calloc(1, sizeof(struct appstate));

	config_parse(&s->cfg, argc, argv);

	s->quarantine = quarantine_new();
	if (!s->quarantine)
		errl(1, "quarantine_new");

	s->evbase = event_base_new();
	if (!s->evbase)
		errl(1, "event_base_new");

	if (path_combine(pathbuf, PATH_MAX, s->cfg.persistent_dir,
	    QUARANTINE_FILENAME)) {
		if ((f = fopen(pathbuf, "r"))) {
			quarantine_deserialize(s->quarantine, f);
			fclose(f);
		} else {
			warnl("opening quarantine_path failed");
		}
	}

	return s;
}

void
appstate_free(struct appstate **s)
{
	event_base_free((*s)->evbase);
	quarantine_free(&(*s)->quarantine);
	config_free(&(*s)->cfg);
	free(*s);
	*s = NULL;
}
