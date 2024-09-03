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
#include "version.h"

#include <time.h>
#include <unistd.h>
#include <confuse.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "config.h"
#include "log.h"

#define CONF_PATH_DEFAULT "/etc/gmlgcd.conf"

#define VERBOSE		"verbose"
#define URI_SUBPATH 	"uri-subpath"
#define COMMENTS_DIR 	"comments-dir"
#define RUNTIME_DIR	"runtime-dir"
#define TCP		"tcp"
#define THOST		"host"
#define TPORT		"port"
#define PERSISTENT_DIR	"persistent-dir"
#define	USERNAME_MAX	"username-max"
#define LINES_MAX	"lines-max"
#define COMMENT_VERBS	"comment-verbs"
#define ALLOW_LINKS	"allow-links"
#define QUARANTINE "quarantine"
#define QMODE "mode"
#define QMAX_FAILURES "max-failures"
#define QEXPIRY_MINUTES "expiry-minutes"

static _Noreturn void
usage(void)
{
	printf("%s [-vV -c <path/to/gmlgcd.conf>]\n",
	    getprogname());
	exit(0);
}

static _Noreturn void
version(void)
{
	printf("%s v" VERSION_STR "\n", getprogname());
	exit(0);
}

static _Noreturn void
config_parse_errorcb(cfg_t *cfg, const char *fmt, va_list ap)
{
	(void)cfg;
	verrx(1, fmt, ap);
}

static int
config_validate_natural(cfg_t *c, cfg_opt_t *o)
{
	int v = cfg_opt_getnint(o, cfg_opt_size(o) - 1);
	if (v < 1) {
		cfg_error(c, "%s.%s < 1, is that what you want?", c->name,
		    o->name);
		return -1;
	}

	return 0;
}

static int
config_parse_quarantine_mode(cfg_t *cfg, cfg_opt_t *opt, const char *value,
    void *result)
{
	enum quarantine_mode *qmode = result;

	if (strcmp(value, "always") == 0)
		*qmode = ALWAYS;
	else if (strcmp(value, "on-error") == 0)
		*qmode = ON_ERROR;
	else if (strcmp(value, "never") == 0)
		*qmode = NEVER;
	else {
		cfg_error(cfg,
		    "bad value for '"
		    QMODE "': %s | possible values are: " "'always' 'on-error' 'never'",
		    value);
		return -1;
	}

	return 0;
}

void
config_parse(struct config *cfg, int argc, char *const *argv)
{
	const char *cfg_path = CONF_PATH_DEFAULT;
	cfg_opt_t tcp_opts[] = {
		CFG_STR(THOST, "127.0.0.1", CFGF_NONE),
		CFG_INT(TPORT, 0, CFGF_NONE),
		CFG_END()
	};
	cfg_opt_t quarantine_opts[] = {
		CFG_PTR_CB(QMODE, "on-error", CFGF_NONE, config_parse_quarantine_mode, free),
		CFG_INT(QMAX_FAILURES, 5, CFGF_NONE),
		CFG_FLOAT(QEXPIRY_MINUTES, 5.0, CFGF_NONE)
	};
	cfg_opt_t file_opts[] = {
		CFG_BOOL(VERBOSE, false, CFGF_NONE),

		CFG_SIMPLE_STR(URI_SUBPATH, &cfg->uri_subpath),
		CFG_SIMPLE_STR(COMMENTS_DIR, &cfg->comments_dir),
		CFG_SIMPLE_STR(PERSISTENT_DIR, &cfg->persistent_dir),

		CFG_STR(RUNTIME_DIR, NULL, CFGF_NONE),
		CFG_SEC(TCP, tcp_opts, CFGF_NODEFAULT),

		CFG_INT(USERNAME_MAX, 25, CFGF_NONE),
		CFG_INT(LINES_MAX, 4, CFGF_NONE),
		CFG_STR_LIST(COMMENT_VERBS, NULL, CFGF_LIST),

		CFG_BOOL(ALLOW_LINKS, false, CFGF_NONE),

		CFG_SEC(QUARANTINE, quarantine_opts, CFGF_NONE),

		CFG_END()
	};
	cfg_t *file_cfg, *tcp_cfg, *q_cfg;
	const char *host;
	unsigned int i;
	char c;

	memset(cfg, 0, sizeof(struct config));
	__log_verbose = false;

	while ((c = getopt(argc, argv, "SVvc:")) != -1) {
		switch (c) {
		case 'c':
			if (!optarg)
				usage();
			cfg_path = optarg;
			break;
		case 'v':
			__log_verbose = true;
			break;
		case 'V':
			version();
		case 'S':
			cfg->danger_no_sandbox = true;
			break;
		default:
			usage();
		}
	}

	file_cfg = cfg_init(file_opts, CFGF_NONE);
	cfg_set_error_function(file_cfg, config_parse_errorcb);
	cfg_set_validate_func(file_cfg, USERNAME_MAX "|" LINES_MAX,
	    config_validate_natural);

		// TODO fix validate function

	if (cfg_parse(file_cfg, cfg_path) == CFG_FILE_ERROR)
		errl(1, "opening %s failed", cfg_path);

	if (!__log_verbose)
		__log_verbose = cfg_getbool(file_cfg, VERBOSE);

	cfg->username_max = cfg_getint(file_cfg, USERNAME_MAX);
	cfg->lines_max = cfg_getint(file_cfg, LINES_MAX);

	cfg->comment_verbs_len = cfg_size(file_cfg, COMMENT_VERBS);
	if (cfg->comment_verbs_len > 0) {
		cfg->comment_verbs = calloc(cfg->comment_verbs_len,
		    sizeof(char *));
		for (i = 0; i < cfg->comment_verbs_len; ++i)
			cfg->comment_verbs[i] = strdup(cfg_getnstr(file_cfg,
			    COMMENT_VERBS, i));
	}
	cfg->allow_links = cfg_getbool(file_cfg, ALLOW_LINKS);

	if (cfg_size(file_cfg, TCP) > 0) {
		tcp_cfg = cfg_getsec(file_cfg, TCP);
		host = cfg_getstr(tcp_cfg, THOST);

		if (inet_pton(AF_INET, host, &cfg->listen.tcp.ip.v4) == 1)
			cfg->af = AF_INET;
		else if (inet_pton(AF_INET6, host, &cfg->listen.tcp.ip.v6) == 1)
			cfg->af = AF_INET6;
		else
			errxl(1, "bad '" TCP "." THOST "': %s", host);

		if ((cfg->listen.tcp.port = cfg_getint(tcp_cfg, TPORT)) == 0)
			errxl(1, "'" TCP "." TPORT "' unspecified");

		// port range not checked

	} else if ((cfg->listen.runtime_dir = cfg_getstr(file_cfg,
	    RUNTIME_DIR))) {
		cfg->af = AF_UNIX;
		cfg->listen.runtime_dir = strdup(cfg->listen.runtime_dir);
	} else
		errxl(1,
		    "'" TCP "' section or '" RUNTIME_DIR "' option required");

	q_cfg = cfg_getsec(file_cfg, QUARANTINE);
	cfg->quarantine.mode = *(enum quarantine_mode *)cfg_getptr(q_cfg, QMODE);
	cfg->quarantine.max_failures = cfg_getint(q_cfg, QMAX_FAILURES);
	cfg->quarantine.expiry_minutes = cfg_getfloat(q_cfg, QEXPIRY_MINUTES);

	if (!cfg->comments_dir)
		errxl(1, "'" COMMENTS_DIR "' unset");
	if (!cfg->uri_subpath)
		errxl(1, "'" URI_SUBPATH "' unset");
	if (!cfg->persistent_dir)
		errxl(1, "'" PERSISTENT_DIR "' unset");

	cfg_free(file_cfg);
}

void
config_free(struct config *c)
{
	size_t i;

	free(c->uri_subpath);
	free(c->comments_dir);
	free(c->persistent_dir);

	if (c->af == AF_UNIX && c->listen.runtime_dir)
		free(c->listen.runtime_dir);

	if (c->comment_verbs) {
		for (i = 0; i < c->comment_verbs_len; ++i)
			if (c->comment_verbs[i])
				free(c->comment_verbs[i]);

		free(c->comment_verbs);
	}

	memset(c, 0, sizeof(struct config));
}
