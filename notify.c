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

#include "notify.h"

#include <unistd.h>
#include <sys/wait.h>

#include "log.h"

void
notify(const struct config *cfg, unsigned short rid,
    const struct user_id *user, const char *commentfile)
{
	pid_t cpid, wpid;
	int status;

	if (!cfg->notify_path)
		return;

	switch ((cpid = fork())) {
	case 0:
		execl(cfg->notify_path, cfg->notify_path, (char *)NULL);
		break;
	case -1:
		warnli(rid, "fork");
		return;
	default:
		do {
			wpid = waitpid(wpid, &status, WUNTRACED
            #ifdef WCONDINUED
			    | WCONTINUED
            #endif
			    );

			if (wpid == -1) {
				warnli(rid, "waitpid");
				return;
			}

			if (WIFEXITED(status))
				dbgxli(rid, "child exited: %s",
				    strerror(WEXITSTATUS(status)));
			else if (WIFSIGNALED(status))
				warnxli(rid, "child killed: %s",
				    strsignal(WTERMSIG(status)));
			else if (WIFSTOPPED(status))
				dbgxli(rid, "child stopped: %s",
				    strsignal(WSTOPSIG(status)));
            #ifdef WIFCONTINUED
			else if (WIFCONTINUED(status))
				dbgxli(rid, "child resumed");
            #endif
			else
				dbgxli(rid, "unexpected child status (0x%x)",
				    status);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}
}
