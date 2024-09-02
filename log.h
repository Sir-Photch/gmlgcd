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

#include <stdbool.h>

extern bool __log_verbose;

void msg(const char *, ...)
	__attribute__((__format__(__printf__, 1, 2)));

void dbg(const char *, ...)
	__attribute__((__format__(__printf__, 1, 2)));

void dbgx(const char *, ...)
	__attribute__((__format__(__printf__, 1, 2)));

#define warnl(fmt, ...) do {						\
    if (__log_verbose) warn("[WRN %s:%d %s()]: " fmt, __FILE_NAME__, __LINE__, __func__, ##__VA_ARGS__);\
    else warn("[WRN %s()]: " fmt, __func__, ##__VA_ARGS__);		\
} while (0)

#define warnxl(fmt, ...) do {						\
    if (__log_verbose) warnx("[WRN %s:%d %s()]: " fmt, __FILE_NAME__, __LINE__, __func__, ##__VA_ARGS__);\
    else warnx("[WRN %s()]: " fmt, __func__, ##__VA_ARGS__);		\
} while (0)

#define errl(status, fmt, ...) do {					\
    if (__log_verbose) err(status, "[%s:%d %s()]: " fmt, __FILE_NAME__, __LINE__, __func__, ##__VA_ARGS__);\
    else err(status, "[%s()]: " fmt, __func__, ##__VA_ARGS__);		\
} while (0)

#define errxl(status, fmt, ...) do {					\
    if (__log_verbose) errx(status, "[%s:%d %s()]: " fmt, __FILE_NAME__, __LINE__, __func__, ##__VA_ARGS__);\
    else errx(status, "[%s()]: " fmt, __func__, ##__VA_ARGS__);		\
} while (0)

#define msgl(fmt, ...) do {						\
    if (__log_verbose) msg("[MSG %s:%d %s()]: " fmt, __FILE_NAME__, __LINE__, __func__, ##__VA_ARGS__);\
    else msg("[MSG %s()]: " fmt, __func__, ##__VA_ARGS__);		\
} while (0)

#define warnli(msgid, fmt, ...) do {					\
    if (__log_verbose) warn("[WRN #%d %s:%d %s()]: " fmt, msgid, __FILE_NAME__, __LINE__, __func__, ##__VA_ARGS__);\
    else warn("[WRN #%d %s()]: " fmt, msgid, __func__, ##__VA_ARGS__);	\
} while (0)

#define warnxli(msgid, fmt, ...) do {					\
    if (__log_verbose) warnx("[WRN #%d %s:%d %s()]: " fmt, msgid, __FILE_NAME__, __LINE__, __func__, ##__VA_ARGS__);\
    else warnx("[WRN #%d %s()]: " fmt, msgid, __func__, ##__VA_ARGS__);	\
} while (0)

#define errli(msgid, status, fmt, ...) do {				\
    if (__log_verbose) err(status, "[#%d %s:%d %s()]: " fmt, msgid, __FILE_NAME__, __LINE__, __func__, ##__VA_ARGS__);\
    else err(status, "[#%d %s()]: " fmt, msgid, __func__, ##__VA_ARGS__);\
} while (0)

#define errxli(msgid, status, fmt, ...) do {				\
    if (__log_verbose) errx(status, "[#%d %s:%d %s()]: " fmt, msgid, __FILE_NAME__, __LINE__, __func__, ##__VA_ARGS__);\
    else errx(status, "[#%d %s()]: " fmt, msgid, __func__, ##__VA_ARGS__);\
} while (0)

#define msgli(msgid, fmt, ...) do {					\
    if (__log_verbose) msg("[MSG #%d %s:%d %s()]: " fmt, msgid, __FILE_NAME__, __LINE__, __func__, ##__VA_ARGS__);\
    else msg("[MSG #%d %s()]: " fmt, msgid, __func__, ##__VA_ARGS__);	\
} while (0)

#define dbgxl(fmt, ...) dbgx("[DBG %s()]: " fmt, __func__, ##__VA_ARGS__)

#define dbgxli(msgid, fmt, ...) dbgx("[DBG #%d %s()]: " fmt, msgid, __func__, ##__VA_ARGS__)

#define dbgl(fmt, ...) dbg("[DBG %s()]: " fmt, __func__, ##__VA_ARGS__)

#define dbgli(msgid, fmt, ...) dbg("[DBG #%d %s()]: " fmt, msgid, __func__, ##__VA_ARGS__)
