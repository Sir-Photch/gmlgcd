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

#define VERBOSE			"verbose"

#define URI_SUBPATH 	"uri-subpath"
#define COMMENTS_DIR 	"comments-dir"
#define PERSISTENT_DIR	"persistent-dir"

#define RUNTIME_DIR		"runtime-dir"

#define TCP				"tcp"
#define THOST			"host"
#define TPORT			"port"

#define HELP_TEMPLATE	"help-template-file"

#define COMMENT			"comment"
#define CVERBS			"verbs"
#define CLINES_MAX		"lines-max"
#define	CUSERNAME_MAX	"username-max"
#define CALLOW_LINKS	"allow-links"
#define CAUTH			"authentication"
#define CVALIDATE (CUSERNAME_MAX "|" CLINES_MAX)

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
config_parse_comment_auth(cfg_t *cfg, cfg_opt_t *opt, const char *value,
    void *result)
{
	enum authmode *authmode = result;

	if (strcmp(value, "none") == 0)
		*authmode = NONE;
	else if (strcmp(value, "require-username") == 0)
		*authmode = REQUIRE_USERNAME;
	else if (strcmp(value, "require-cert") == 0)
		*authmode = REQUIRE_CERT;
	else {
		cfg_error(cfg,
		    "Bad %s, possible values are: { 'none', 'require-username', 'require-cert' }",
		    cfg_opt_name(opt));
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
	cfg_opt_t comment_opts[] = {
		CFG_STR_LIST(CVERBS, NULL, CFGF_LIST),
		CFG_INT(CLINES_MAX, 4, CFGF_NONE),
		CFG_INT(CUSERNAME_MAX, 25, CFGF_NONE),
		CFG_BOOL(CALLOW_LINKS, false, CFGF_NONE),
		CFG_INT_CB(CAUTH, REQUIRE_USERNAME, CFGF_NONE, config_parse_comment_auth),
		CFG_END()
	};
	cfg_opt_t file_opts[] = {
		CFG_BOOL(VERBOSE, false, CFGF_NONE),

		CFG_SIMPLE_STR(URI_SUBPATH, &cfg->uri_subpath),
		CFG_SIMPLE_STR(COMMENTS_DIR, &cfg->comments_dir),
		CFG_SIMPLE_STR(PERSISTENT_DIR, &cfg->persistent_dir),

		CFG_STR(RUNTIME_DIR, NULL, CFGF_NONE),
		CFG_SEC(TCP, tcp_opts, CFGF_NODEFAULT),

		CFG_STR(HELP_TEMPLATE, NULL, CFGF_NONE),

		CFG_SEC(COMMENT, comment_opts, CFGF_NONE),

		CFG_END()
	};
	cfg_t *file_cfg, *tcp_cfg, *comment_cfg;
	const char *host;
	size_t i, n;
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

	if (cfg_parse(file_cfg, cfg_path) == CFG_FILE_ERROR)
		errl(1, "opening %s failed", cfg_path);

	if (!__log_verbose)
		__log_verbose = cfg_getbool(file_cfg, VERBOSE);

	comment_cfg = cfg_getsec(file_cfg, COMMENT);
	cfg_set_validate_func(comment_cfg, CVALIDATE, config_validate_natural);

	n = cfg->comment.verbs.n = cfg_size(comment_cfg, CVERBS);
	if (n > 0) {
		cfg->comment.verbs.p = calloc(cfg->comment.verbs.n,
		    sizeof(char *));
		for (i = 0; i < n; ++i)
			cfg->comment.verbs.p[i] = strdup(
			    cfg_getnstr(comment_cfg, CVERBS, i));
	}

	cfg->comment.lines_max = cfg_getint(comment_cfg, CLINES_MAX);
	cfg->comment.username_max = cfg_getint(comment_cfg, CUSERNAME_MAX);

	cfg->comment.allow_links = cfg_getbool(comment_cfg, CALLOW_LINKS);
	cfg->comment.auth = cfg_getint(comment_cfg, CAUTH);

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

	if (c->comment.verbs.p) {
		for (i = 0; i < c->comment.verbs.n; ++i)
			if (c->comment.verbs.p[i])
				free(c->comment.verbs.p[i]);

		free(c->comment.verbs.p);
	}

	memset(c, 0, sizeof(struct config));
}
