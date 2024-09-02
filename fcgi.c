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

#include "fcgi.h"
#include "log.h"

static bool
fcgi_end_request(struct evbuffer *out, unsigned short rid)
{
	struct fcgi_record_end_request record = {
		.header = {
			.version = FCGI_VERSION_1,
			.type = FCGI_END_REQUEST,
			.requestIdB1 = rid >> 8,
			.requestIdB0 = rid & 0xFF,
			.contentLengthB0 = sizeof(struct fcgi_body_end_request)
		},
		.body = {
			.protocolStatus = FCGI_REQUEST_COMPLETE,
		},
	};

	if (evbuffer_add(out, &record, sizeof(record)) < 0) {
		warnxli(rid, "evbuffer_add");
		return false;
	}

	return true;
}

bool
fcgi_check_header(struct evbuffer *in, struct fcgi_header *out)
{
	if (evbuffer_remove(in, out, FCGI_HEADER_LEN) < FCGI_HEADER_LEN) {
		warnxl("evbuffer_remove");
		return false;
	}

	if (out->version != FCGI_VERSION_1) {
		warnxl("FCGI version %d is not supported", out->version);
		return false;
	}

	return true;
}

bool
fcgi_read_param(struct evbuffer *in, struct fcgi_params_entry *entry)
{
	int32_t name_len, val_len;
	bool long_name_len, long_val_len;

	unsigned char bytes[8];
	size_t i;

	entry->name = entry->value = NULL;

	if (evbuffer_remove(in, bytes, 2) < 2)
		return false;

	long_name_len = bytes[0] >> 7 == 1;

	if (long_name_len) {
		i = 2;

		if (evbuffer_remove(in, bytes + i, 3) < 3)
			return false;

		i += 3;
	}

	long_val_len = bytes[long_name_len ? 4 : 1] >> 7 == 1;

	if (long_val_len) {
		i = long_name_len ? 5 : 2;

		if (evbuffer_remove(in, bytes + i, 3) < 3)
			return false;
	}

	if (long_name_len) {
		name_len = ((bytes[0] & 0x7f) << 24) | (bytes[1] << 16) |
		    (bytes[2] << 8) | bytes[3];

		if (long_val_len) {
			val_len = ((bytes[4] & 0x7f) << 24) | (bytes[5] << 16) |
			    (bytes[6] << 8) | bytes[7];
		} else {
			val_len = bytes[4];
		}
	} else {
		name_len = bytes[0];

		if (long_val_len) {
			val_len = ((bytes[1] & 0x7f) << 24) | (bytes[2] << 16) |
			    (bytes[3] << 8) | bytes[4];
		} else {
			val_len = bytes[1];
		}
	}

	entry->name = calloc(name_len + 1, sizeof(char));

	if (evbuffer_remove(in, entry->name, name_len) < name_len) {
		warnxl("evbuffer_remove");
		free(entry->name);
		return false;
	}

	entry->value = calloc(val_len + 1, sizeof(char));

	if (evbuffer_remove(in, entry->value, val_len) < val_len) {
		warnxl("evbuffer_remove");
		free(entry->name);
		free(entry->value);
		return false;
	}

	return true;
}

bool
fcgi_write_stdout(struct evbuffer *out, unsigned short rid,
    const char *str,
    unsigned short str_len)
{
	struct fcgi_header header = {
		.version = FCGI_VERSION_1,
		.type = FCGI_STDOUT,
		.requestIdB1 = rid >> 8,
		.requestIdB0 = rid & 0xFF,
		.contentLengthB1 = str_len >> 8,
		.contentLengthB0 = str_len & 0xFF,
		.paddingLength = 0,
	};

	if (evbuffer_add(out, &header, FCGI_HEADER_LEN) < 0 ||
	    evbuffer_add(out, str, str_len) < 0) {
		warnxli(rid, "evbuffer_add");
		return false;
	}

	header.contentLengthB0 = header.contentLengthB1 = 0;

	return evbuffer_add(out, &header, FCGI_HEADER_LEN) == 0 &&
	    fcgi_end_request(out, rid);
}
