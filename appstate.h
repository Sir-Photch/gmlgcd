#pragma once

#include "config.h"

struct appstate {
	struct event_base *evbase;
	struct quarantine_list *quarantine;
	struct event *sock_event, *int_event, *term_event;
	struct config cfg;
};

struct appstate *appstate_new(int argc, char *const *);
void appstate_free(struct appstate **);
