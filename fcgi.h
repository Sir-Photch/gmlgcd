#pragma once

#include "platform.h"

#include <event2/buffer.h>
#include <stdbool.h>

// https://fastcgi-archives.github.io/FastCGI_Specification.html#S8

struct fcgi_header {
	unsigned char version;
	unsigned char type;
	unsigned char requestIdB1;
	unsigned char requestIdB0;
	unsigned char contentLengthB1;
	unsigned char contentLengthB0;
	unsigned char paddingLength;
	unsigned char reserved;
};

/*
 * Number of bytes in a FCGI_Header.  Future versions of the protocol
 * will not reduce this number.
 */
#define FCGI_HEADER_LEN  8

/*
 * Value for version component of FCGI_Header
 */
#define FCGI_VERSION_1           1

/*
 * Values for type component of FCGI_Header
 */
#define FCGI_BEGIN_REQUEST       1
#define FCGI_ABORT_REQUEST       2
#define FCGI_END_REQUEST         3
#define FCGI_PARAMS              4
#define FCGI_STDIN               5
#define FCGI_STDOUT              6
#define FCGI_STDERR              7
#define FCGI_DATA                8
#define FCGI_GET_VALUES          9
#define FCGI_GET_VALUES_RESULT  10
#define FCGI_UNKNOWN_TYPE       11
#define FCGI_MAXTYPE (FCGI_UNKNOWN_TYPE)

/*
 * Value for requestId component of FCGI_Header
 */
#define FCGI_NULL_REQUEST_ID     0

struct fcgi_body_begin_request {
	unsigned char roleB1;
	unsigned char roleB0;
	unsigned char flags;
	unsigned char reserved[5];
};

struct fcgi_record_begin_request {
	struct fcgi_header header;
	struct fcgi_body_begin_request body;
};

/*
 * Mask for flags component of FCGI_BeginRequestBody
 */
#define FCGI_KEEP_CONN  1

/*
 * Values for role component of FCGI_BeginRequestBody
 */
#define FCGI_RESPONDER  1
#define FCGI_AUTHORIZER 2
#define FCGI_FILTER     3

struct fcgi_body_end_request {
	unsigned char appStatusB3;
	unsigned char appStatusB2;
	unsigned char appStatusB1;
	unsigned char appStatusB0;
	unsigned char protocolStatus;
	unsigned char reserved[3];
};

struct fcgi_record_end_request {
	struct fcgi_header header;
	struct fcgi_body_end_request body;
};

/*
 * Values for protocolStatus component of FCGI_EndRequestBody
 */
#define FCGI_REQUEST_COMPLETE 0
#define FCGI_CANT_MPX_CONN    1
#define FCGI_OVERLOADED       2
#define FCGI_UNKNOWN_ROLE     3

/*
 * Variable names for FCGI_GET_VALUES / FCGI_GET_VALUES_RESULT records
 */
#define FCGI_MAX_CONNS  "FCGI_MAX_CONNS"
#define FCGI_MAX_REQS   "FCGI_MAX_REQS"
#define FCGI_MPXS_CONNS "FCGI_MPXS_CONNS"

struct fcgi_body_unknown_type {
	unsigned char type;
	unsigned char reserved[7];
};

struct fcgi_record_unknown_type {
	struct fcgi_header header;
	struct fcgi_body_unknown_type body;
};

struct fcgi_params_entry {
	char *name, *value;
	TAILQ_ENTRY(fcgi_params_entry) entries;
};

TAILQ_HEAD(fcgi_params_head, fcgi_params_entry);

bool fcgi_check_header(struct evbuffer *, struct fcgi_header *);
bool fcgi_read_param(struct evbuffer *, struct fcgi_params_entry *);
bool fcgi_write_stdout(struct evbuffer *, unsigned short, const char *,
    unsigned short);
