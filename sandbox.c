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

#include "sandbox.h"
#include "log.h"

#if defined(__linux__)
#define __USE_GNU
#include <fcntl.h>
#include <linux/landlock.h>
#include <linux/prctl.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>

static int
landlock_create_ruleset(const struct landlock_ruleset_attr *attr, __u32 flags)
{
	if (!attr && (flags & LANDLOCK_CREATE_RULESET_VERSION)) {
		return syscall(SYS_landlock_create_ruleset, NULL, 0,
		    LANDLOCK_CREATE_RULESET_VERSION);
	} else {
		return syscall(SYS_landlock_create_ruleset, attr,
		    sizeof(struct landlock_ruleset_attr), 0);
	}
}

static bool
ruleset_compat(struct landlock_ruleset_attr *attr, bool allow_unsupported)
{
	int abi;

	abi = landlock_create_ruleset(NULL, LANDLOCK_CREATE_RULESET_VERSION);
	if (abi < 0) {
		if (allow_unsupported) {
			warnxl("kernel without landlock support");
			return false;
		} else
			errxl(1, "kernel without landlock support");
	}

	switch (abi) {
	case 1:
		attr->handled_access_fs &= ~LANDLOCK_ACCESS_FS_REFER;
	    /* fall through */
	case 2:
		attr->handled_access_fs &= ~LANDLOCK_ACCESS_FS_TRUNCATE;
	    /* fall through */
	case 3:
		attr->handled_access_net &=
		    ~(LANDLOCK_ACCESS_NET_BIND_TCP |
		    LANDLOCK_ACCESS_NET_CONNECT_TCP);
	    /* fall through */
	case 4:
		attr->handled_access_fs &= ~LANDLOCK_ACCESS_FS_IOCTL_DEV;
	}

	return true;
}

#elif defined(__OpenBSD__)
#	include <unistd.h>
#	include <sys/socket.h>
#endif

void
enter_the_sandbox(struct config *cfg)
{
#if defined(__linux__)
	int error, ruleset_fd;
	struct landlock_ruleset_attr rules = {0};
	struct landlock_path_beneath_attr path = {0};
	struct landlock_net_port_attr net = {0};

	rules.handled_access_fs =
	    LANDLOCK_ACCESS_FS_EXECUTE |
	    LANDLOCK_ACCESS_FS_WRITE_FILE |
	    LANDLOCK_ACCESS_FS_READ_FILE |
	    LANDLOCK_ACCESS_FS_REMOVE_DIR |
	    LANDLOCK_ACCESS_FS_REMOVE_FILE |
	    LANDLOCK_ACCESS_FS_MAKE_CHAR |
	    LANDLOCK_ACCESS_FS_MAKE_DIR |
	    LANDLOCK_ACCESS_FS_MAKE_REG |
	    LANDLOCK_ACCESS_FS_MAKE_SOCK |
	    LANDLOCK_ACCESS_FS_MAKE_FIFO |
	    LANDLOCK_ACCESS_FS_MAKE_BLOCK |
	    LANDLOCK_ACCESS_FS_MAKE_SYM |
	    LANDLOCK_ACCESS_FS_REFER |
	    LANDLOCK_ACCESS_FS_TRUNCATE |
	    LANDLOCK_ACCESS_FS_IOCTL_DEV;
	rules.handled_access_net =
	    LANDLOCK_ACCESS_NET_BIND_TCP |
	    LANDLOCK_ACCESS_NET_CONNECT_TCP;

	if (!ruleset_compat(&rules, cfg->danger_no_sandbox))
		return;

	ruleset_fd = landlock_create_ruleset(&rules, sizeof(rules));
	if (ruleset_fd == -1)
		errl(1, "landlock_create_ruleset");

	path.allowed_access = LANDLOCK_ACCESS_FS_WRITE_FILE;
	path.parent_fd = open(cfg->comments_dir, O_PATH | O_CLOEXEC);
	if (path.parent_fd == -1) {
		close(ruleset_fd);
		errl(1, "open %s", cfg->comments_dir);
	}
	error = syscall(SYS_landlock_add_rule, ruleset_fd,
	    LANDLOCK_RULE_PATH_BENEATH, &path, 0);
	if (error) {
		close(ruleset_fd);
		errl(1, "landlock_add_rule");
	}

	close(path.parent_fd);

	path.allowed_access =
	    LANDLOCK_ACCESS_FS_TRUNCATE |
	    LANDLOCK_ACCESS_FS_WRITE_FILE |
	    LANDLOCK_ACCESS_FS_READ_FILE |
	    LANDLOCK_ACCESS_FS_MAKE_REG;
	path.parent_fd = open(cfg->persistent_dir, O_PATH | O_CLOEXEC);
	if (path.parent_fd == -1) {
		close(ruleset_fd);
		errl(1, "open %s", cfg->persistent_dir);
	}
	error = syscall(SYS_landlock_add_rule, ruleset_fd,
	    LANDLOCK_RULE_PATH_BENEATH, &path, 0);
	close(path.parent_fd);

	if (error) {
		close(ruleset_fd);
		errl(1, "landlock_add_rule");
	}

	if (cfg->af == AF_UNIX) {
		path.allowed_access =
		    LANDLOCK_ACCESS_FS_MAKE_SOCK |
		    LANDLOCK_ACCESS_FS_REMOVE_FILE;
		path.parent_fd = open(cfg->listen.runtime_dir,
		    O_PATH | O_CLOEXEC);
		if (path.parent_fd == -1) {
			close(ruleset_fd);
			errl(1, "open %s", cfg->listen.runtime_dir);
		}
		error = syscall(SYS_landlock_add_rule, ruleset_fd,
		    LANDLOCK_RULE_PATH_BENEATH, &path, 0);
		close(path.parent_fd);

		if (error) {
			close(ruleset_fd);
			errl(1, "landlock_add_rule");
		}
	} else {
		net.allowed_access =
		    LANDLOCK_ACCESS_NET_BIND_TCP;
		net.port = cfg->listen.tcp.port;
		error = syscall(SYS_landlock_add_rule, ruleset_fd,
		    LANDLOCK_RULE_NET_PORT, &net, 0);
		if (error) {
			close(ruleset_fd);
			errl(1, "landlock_add_rule");
		}
	}

	if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0)) {
		close(ruleset_fd);
		errl(1, "prctl SET_NO_NEW_PRIVS");
	}

	if (syscall(SYS_landlock_restrict_self, ruleset_fd, 0)) {
		close(ruleset_fd);
		errl(1, "landlock_restrict_self");
	}

	close(ruleset_fd);

#elif defined(__OpenBSD__)
	unveil(cfg->comments_dir, "w");
	unveil(cfg->persistent_dir, "crw");

	if (cfg->af == AF_UNIX) {
		unveil(cfg->listen.runtime_dir, "c");
		pledge("stdio rpath wpath cpath unix", NULL);
	} else {
		pledge("stdio rpath wpath cpath inet", NULL);
	}

#else
#	warning "unknown platform, don't know how to sandbox myself! :/"
#endif

}
