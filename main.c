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

#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <fcntl.h>
#include <limits.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/util.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <arpa/inet.h>

#include "comment.h"
#include "fcgi.h"
#include "log.h"
#include "quarantine.h"
#include "replies.h"
#include "appstate.h"
#include "sandbox.h"
#include "util.h"
#include "config.h"

#define PROJECT_NAME 	"gmlgcd"
#define SOCK_FILE 		"fcgi.sock"
#define MAX_LINE     	(2 << 14)

static bool
check_url_path(const char *gemini_url_path, unsigned short rid,
    char *commenting_path, size_t cpath_len, const char **requested_file,
    const char **errstr, const struct config *cfg)
{
	const char *slash;

	if (!(slash = strchr(gemini_url_path, '/'))) {
		warnxli(rid, "bad GEMINI_URL_PATH: %s", gemini_url_path);

		*errstr = BAD_REQUEST;
		return false;
	}

	if (!path_combine(commenting_path, cpath_len, cfg->comments_dir,
	    (*requested_file = slash) + 1)) {
		warnxli(rid, "requested path exceeds %lu: %s", cpath_len,
		    *requested_file);

		*errstr = BAD_REQUEST;
		return false;
	}

	msgli(rid, "requesting %s", *requested_file);

	if (access(commenting_path, F_OK) != 0) {
		msgli(rid, "Commentfile not available: %s", commenting_path);

		*errstr = COMMENTS_NOT_ENABLED;
		return false;
	}

	if (access(commenting_path, W_OK) != 0) {
		msgli(rid, "Commentfile not writeable: %s", commenting_path);

		*errstr = COMMENTS_NOT_ALLOWED;
		return false;
	}

	*errstr = NULL;
	return true;
}

static bool
generate_response(struct evbuffer *out, unsigned short rid,
    struct fcgi_params_head *params, struct appstate *s)
{
	char commenting_path[PATH_MAX + 1];
	char formatted_comment[COMMENTS_MAX];
	char redirection_reply[512];
	struct quarantine_entry *qent;
	struct fcgi_params_entry *p;
	struct user_input user;
	const char *errstr;
	FILE *f;
	time_t now;
	double expired_min;
	size_t body_len, hash_len;
	int commenting_fd;

	const char *server_name = NULL,
		   *gemini_url_path = NULL,
		   *requested_file = NULL,
		   *rhost = NULL,
		   *hash = NULL;

	bool valid_proto = false,
	     valid_request = false;

	memset(commenting_path, 0, sizeof(commenting_path));
	memset(&user, 0, sizeof(user));

	TAILQ_FOREACH(p, params, entries) {
		if (!rhost &&
		    (strcmp("REMOTE_ADDR", p->name) == 0 ||
		     strcmp("REMOTE_HOST", p->name) == 0)) {
			rhost = p->value;
			if (inet_pton(AF_INET, rhost, &user.id.rhost.v4) == 1)
				user.id.af = AF_INET;
			else if (inet_pton(AF_INET6, rhost,
			    &user.id.rhost.v6) == 1)
				user.id.af = AF_INET6;
			else
				rhost = NULL;
		} else if (!valid_proto &&
		    strcmp("SERVER_PROTOCOL", p->name) == 0) {
			valid_proto = strcmp("GEMINI", p->value) == 0;
		} else if (!valid_request &&
		    strcmp("REQUEST_METHOD", p->name) == 0) {
			valid_request = strcmp("GET", p->value) == 0;
		} else if (commenting_path[0] == '\0' &&
		    strcmp("GEMINI_URL_PATH", p->name) == 0) {
			gemini_url_path = p->value;
		} else if (!user.gemini_search_string &&
		    strcmp("GEMINI_SEARCH_STRING", p->name) == 0) {
			user.gemini_search_string = p->value;
		} else if (!server_name &&
		    strcmp("SERVER_NAME", p->name) == 0) {
			server_name = p->value;
		} else if (!user.name &&
		    strcmp("REMOTE_USER", p->name) == 0) {
			user.name = p->value;
		} else if (!hash &&
		    strcmp("TLS_CLIENT_HASH", p->name) == 0) {
			// strip 'SHA256:'
			if ((hash = strchr(p->value, ':')))
				hash += 1;
			else
				hash = p->value;

			if ((hash_len = strlcpy(user.id.hash, hash,
			    USER_HASH_LEN)) < USER_HASH_LEN - 1) {
				warnxli(rid, "very short hash: %s", hash);
				hash = NULL;
			}
		}
//		  else if (strcmp("TLS_CLIENT_NOT_BEFORE", p->name) == 0) {
//			bool success = strptime(p->value, "%FT%TZ", &tm);
//		}
	}

	if (!rhost) {
		warnxli(rid, "invalid rhost");
		return false;
	}

	if (!server_name) {
		warnxli(rid, "invalid server_name");
		return false;
	}

	msgli(rid, "request from %s via %s", rhost, server_name);

	if (!valid_proto) {
		warnxli(rid, "invalid proto");
		return false;
	}

	if (!valid_request) {
		warnxli(rid, "invalid request");
		return false;
	}

	if (!hash) {
		msgli(rid, "missing certificate");
		return fcgi_write_stdout(out, rid, CERTIFICATE_REQUIRED,
		    sizeof(CERTIFICATE_REQUIRED));
	}

	qent = quarantine_get_entry(s->quarantine, user.id);
	time(&now);

	if (qent) {
		expired_min = difftime(now,
		    qent->last_failure) / 60.0;

		if (qent->failures > 5 && expired_min < 5.0) {
			qent->last_failure = now;
			qent->failures++;

			msgli(rid, "ratelimited: %lu failures", qent->failures);

			return fcgi_write_stdout(out, rid, SLOW_DOWN,
			    sizeof(SLOW_DOWN));
		}
	}

	if (!check_url_path(gemini_url_path, rid, commenting_path,
	    sizeof(commenting_path), &requested_file, &errstr, &s->cfg)) {
		if (!qent)
			qent = quarantine_add(s->quarantine, &user.id);

		qent->last_failure = now;
		qent->failures++;

		return fcgi_write_stdout(out, rid, errstr, strlen(errstr));
	}

	errstr = NULL;
	if (user.gemini_search_string &&
	    commentf(formatted_comment, &s->cfg, rid, user, s->cfg.allow_links,
	    &errstr)) {
		if ((commenting_fd = open(commenting_path,
		    O_WRONLY | O_APPEND)) == -1) {
			errli(rid, 1, "open(%s, O_WRONLY | O_APPEND)",
			    commenting_path);
		}

		f = fdopen(commenting_fd, "a");
		if (!f)
			errli(rid, 1, "fdopen");

		fputs(formatted_comment, f);

		fclose(f);
		commenting_fd = -1;

		msgli(rid, "Wrote %lu bytes",
		    strnlen(formatted_comment, COMMENTS_MAX));

		memset(redirection_reply, 0, sizeof(redirection_reply));

		body_len = snprintf(redirection_reply,
		    sizeof(redirection_reply), "30 gemini://%s/%s%s\r\n",
		    server_name, s->cfg.uri_subpath, requested_file);

		if (qent) {
			quarantine_remove(s->quarantine, qent);
			quarantine_entry_free(&qent);
		}

		return fcgi_write_stdout(out, rid, redirection_reply, body_len);
	}

	if (errstr) {
		if (!qent)
			qent = quarantine_add(s->quarantine, &user.id);

		qent->last_failure = now;
		qent->failures++;

		return fcgi_write_stdout(out, rid, errstr, strlen(errstr));
	}

	if (qent) {
		quarantine_remove(s->quarantine, qent);
		quarantine_entry_free(&qent);
	}

	msgli(rid, "empty query, requesting input");

	return fcgi_write_stdout(out, rid, REQUEST_INPUT,
	    sizeof(REQUEST_INPUT));
}

static bool
handle_request(struct evbuffer *in, struct evbuffer *out,
    struct fcgi_header header, bool *keep_conn,
    struct appstate *s)
{
	struct fcgi_body_begin_request body;
	bool got_params, got_stdin, success;
	unsigned short content_len, rid;

	struct fcgi_params_head params;
	struct fcgi_params_entry *_entry, *entry;

	success = true;
	got_stdin = got_params = false;

	if (evbuffer_remove(in, &body, sizeof(body)) < (int)sizeof(body)) {
		warnxl("evbuffer_remove");
		return false;
	}

	if (body.roleB0 != FCGI_RESPONDER) {
		warnxl("only FCGI_RESPONDER role is supported");
		return false;
	}

	*keep_conn = body.flags & FCGI_KEEP_CONN;

	TAILQ_INIT(&params);

	while (fcgi_check_header(in, &header)) {
		content_len = (header.contentLengthB1 << 8) |
		    header.contentLengthB0;
		rid = (header.requestIdB1 << 8) | header.requestIdB0;

		switch (header.type) {
		case FCGI_PARAMS:
			if (got_params) {
				warnxli(rid,
				    "received params after stream was closed");
				success = false;
				goto free;
			}

			if (content_len == 0) {
				dbgxli(rid, "FCGI_PARAMS end");
				got_params = true;
				break;
			}

			entry = calloc(1, sizeof(struct fcgi_params_entry));

			if (!fcgi_read_param(in, entry)) {
				warnxli(rid, "bad FCGI_PARAMS");
				free(entry);
				success = false;
				goto free;
			}

			dbgxli(rid, "FCGI_PARAMS: %s=%s", entry->name,
			    entry->value);

			TAILQ_INSERT_TAIL(&params, entry, entries);

			break;

		case FCGI_STDIN:
			if (got_stdin) {
				warnxli(rid,
				    "received stdin after stream was closed");
				success = false;
				goto free;
			}

			if (!got_stdin && content_len > 0) {
				warnxli(rid, "not handling stdin");
				if (evbuffer_drain(in, content_len) == -1) {
					warnxli(rid, "evbuffer_drain");
					success = false;
					goto free;
				}
				break;
			}

			got_stdin = true;

			if (!generate_response(out, rid, &params, s)) {
				warnxli(rid, "generating response failed");
				success = false;
			} else {
				success = true;
			}

			goto free;
		default:
			warnxli(rid, "received unexpected FCGI header type: %x",
			    header.type);
			success = false;

			struct fcgi_record_unknown_type response = {
				.header = {
					.version = FCGI_VERSION_1,
					.type = FCGI_UNKNOWN_TYPE,
					.requestIdB0 = FCGI_NULL_REQUEST_ID,
					.contentLengthB0 = sizeof(struct fcgi_body_unknown_type)
				},
				.body = {
					.type = header.type,
				},
			};

			evbuffer_add(out, &response, sizeof(response));
			goto free;
		}

		if (evbuffer_drain(in, header.paddingLength) == -1) {
			warnxli(rid, "evbuffer_drain");
			success = false;
			goto free;
		}
	}

 free:
	entry = TAILQ_FIRST(&params);
	while (entry != NULL) {
		_entry = TAILQ_NEXT(entry, entries);
		free(entry->name);
		free(entry->value);
		free(entry);
		entry = _entry;
	}
	return success;
}

static void
read_cb(struct bufferevent *bev, void *ctx)
{
	struct appstate *state;
	struct evbuffer *in, *out;
	bool keep_conn, success;

	state = ctx;
	in = bufferevent_get_input(bev);
	out = bufferevent_get_output(bev);

	struct fcgi_header header;

	if (!fcgi_check_header(in, &header)) {
		warnxl("bad FCGI header");
		bufferevent_free(bev);
	}

	if (header.type == FCGI_BEGIN_REQUEST) {
		success = handle_request(in, out, header,
		    &keep_conn, state);

		if (!success)
			warnxl("handling request failed");

		if (!keep_conn || !success)
			bufferevent_free(bev);

		return;
	}
}

void
error_cb(struct bufferevent *bev, short error, void *ctx)
{
	(void)ctx;

	if (error & BEV_EVENT_EOF)
		dbgxl("connection closed");
	else if (error & BEV_EVENT_ERROR)
		warnl("error_cb");

	bufferevent_free(bev);
}

void
accept_cb(evutil_socket_t listener, short event, void *arg)
{
	(void)event;
	struct bufferevent *bev;
	struct appstate *state;
	struct sockaddr_un client;
	socklen_t client_len;
	int client_fd;

	state = arg;
	client_len = sizeof(client);

	client_fd = accept(listener, (struct sockaddr *)&client, &client_len);

	if (client_fd < 0) {
		warnl("accept");
		return;
	}

	if (FD_SETSIZE < client_fd) {
		warnxl("FD_SETSIZE < client_fd");
		close(client_fd);
		return;
	}

	evutil_make_socket_nonblocking(client_fd);
	bev = bufferevent_socket_new(state->evbase, client_fd,
	    BEV_OPT_CLOSE_ON_FREE);

	bufferevent_setcb(bev, read_cb, NULL, error_cb, state);
	bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);
	bufferevent_enable(bev, EV_READ | EV_WRITE);
}

void
event_log(int severity, const char *message)
{
	switch (severity) {
	case EVENT_LOG_ERR:
		errxl(1, "libevent: %s", message);
		break;
	case EVENT_LOG_WARN:
		warnxl("libevent: %s", message);
		break;
	case EVENT_LOG_MSG:
		msgl("libevent: %s", message);
		break;
	case EVENT_LOG_DEBUG:
		dbgxl("libevent: %s", message);
		break;
	}
}

void
signal_handler(evutil_socket_t listener, short event, void *arg)
{
	(void)listener;

	char pathbuf[PATH_MAX];
	struct appstate *state;
	FILE *quarantine_file;

	if (!(event & EV_SIGNAL)) {
		warnxl("unexpected event");
		return;
	}

	state = arg;

	msgl("quitting...");

	if (state->cfg.persistent_dir &&
	    path_combine(pathbuf, PATH_MAX, state->cfg.persistent_dir,
	    QUARANTINE_FILENAME)) {
		if ((quarantine_file = fopen(pathbuf, "w"))) {
			quarantine_serialize(state->quarantine,
			    quarantine_file);
			fclose(quarantine_file);
		} else {
			warnl("fopen(quarantine_file)");
		}
	} else {
		warnxl("PATH_MAX exceeded! what??");
	}

	event_free(state->sock_event);
	event_free(state->int_event);
	event_free(state->term_event);

	state->sock_event = state->int_event = state->term_event = NULL;
}

int
main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	evutil_socket_t sock_listener;
	union sockaddrs sock;
	struct sockaddr *saddr;
	socklen_t slen;
	struct appstate *state;
	char sockpath[PATH_MAX], strbuf[1024];

	setprogname(PROJECT_NAME);

	event_set_log_callback(event_log);

	state = appstate_new(argc, argv);

	enter_the_sandbox(&state->cfg);

	switch (state->cfg.af) {
	case AF_UNIX:
		path_combine(sockpath, PATH_MAX, state->cfg.listen.runtime_dir,
		    SOCK_FILE);
		sock.un.sun_family = AF_UNIX;
		if (strlcpy(sock.un.sun_path, sockpath,
		    sizeof(sock.un.sun_path)) >= sizeof(sock.un.sun_path))
			errxl(1, "sockpath too long: %s", sockpath);

		unlink(sockpath);

		saddr = (struct sockaddr *)&sock.un;
		slen = sizeof(sock.un);
		break;
	case AF_INET:
		sock.in.sin_family = AF_INET;
		sock.in.sin_addr = state->cfg.listen.tcp.ip.v4;
		sock.in.sin_port = htons(state->cfg.listen.tcp.port);

		saddr = (struct sockaddr *)&sock.in;
		slen = sizeof(sock.in);
		break;
	case AF_INET6:
		sock.in6.sin6_family = AF_INET6;
		sock.in6.sin6_addr = state->cfg.listen.tcp.ip.v6;
		sock.in6.sin6_port = htons(state->cfg.listen.tcp.port);

		saddr = (struct sockaddr *)&sock.in6;
		slen = sizeof(sock.in6);
		break;
	default:
		__builtin_unreachable();
	}

	sock_listener = socket(state->cfg.af, SOCK_STREAM, 0);

	evutil_make_socket_nonblocking(sock_listener);

	if (bind(sock_listener, saddr, slen) < 0)
		errl(1, "bind");

	if (listen(sock_listener, 16) < 0)
		errl(1, "listen");

	state->sock_event = event_new(state->evbase, sock_listener,
	    EV_READ | EV_PERSIST, accept_cb, state);

	if (!state->sock_event)
		errxl(1, "sock_event");

	if (event_add(state->sock_event, NULL) < 0)
		errxl(1, "event_add");

	state->int_event = event_new(state->evbase, SIGINT, EV_SIGNAL,
	    signal_handler, state);
	state->term_event = event_new(state->evbase, SIGTERM, EV_SIGNAL,
	    signal_handler, state);

	if (!state->int_event || event_add(state->int_event, NULL) ||
	    !state->term_event || event_add(state->term_event, NULL))
		warnl("failed to register signals");

	sockaddrs_to_str(memset(strbuf, 0, sizeof(strbuf)), sizeof(strbuf),
	    &sock, state->cfg.af);

	msgl("listening on %s ...", strbuf);

	event_base_dispatch(state->evbase);

	close(sock_listener);

	if (state->cfg.af == AF_UNIX)
		unlink(sockpath);

	appstate_free(&state);

	return 0;
}
