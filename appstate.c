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
