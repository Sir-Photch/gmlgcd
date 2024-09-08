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

#pragma once

#include "platform.h"

#include <time.h>
#include <stdio.h>
#include <stdbool.h>

#include "user.h"

#define QUARANTINE_FILENAME "quarantine.txt"

struct quarantine_entry {
	struct user_id user;
	time_t last_failure;
	size_t failures;
    bool is_blocked;
	TAILQ_ENTRY(quarantine_entry) entries;
};

struct quarantine_list;

struct quarantine_list  *quarantine_new(void);
void                     quarantine_free(struct quarantine_list **);
struct quarantine_entry *quarantine_add(struct quarantine_list *,
    const struct user_id *);
struct quarantine_entry *quarantine_get_entry(struct quarantine_list *,
    struct user_id);
void                     quarantine_remove(struct quarantine_list *,
    struct quarantine_entry *);
void                     quarantine_serialize(struct quarantine_list *, FILE *);
bool                     quarantine_deserialize(struct quarantine_list *,
    FILE *);

void                     quarantine_entry_free(struct quarantine_entry **e);
