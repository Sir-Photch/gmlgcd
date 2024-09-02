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

#define BAD_REQUEST "59 bad request\r\n"
#define REQUEST_INPUT "10 comment:\r\n"
#define	COMMENTS_NOT_ENABLED "51 comments not enabled\r\n"
#define COMMENTS_NOT_ALLOWED "51 comments not allowed\r\n"
#define CERTIFICATE_REQUIRED "60 certificate required\r\n"
#define USERNAME_TOO_LONG "59 username too long\r\n"
#define USERNAME_MISSING "59 username missing\r\n"
#define EMPTY_COMMENT "59 empty comment\r\n"
#define HEADERS_NOT_ALLOWED "59 headers not allowed\r\n"
#define LINKS_NOT_ALLOWED "59 links not allowed\r\n"
#define TOO_MANY_LINES "59 too many lines\r\n"
#define SLOW_DOWN "44 back off\r\n"
