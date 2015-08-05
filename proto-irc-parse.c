/*
 * proto-irc-parse.c
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "proto-irc.h"

enum {
    CR    = 0x0D, /* '\r' */
    LF    = 0x0A, /* '\n' */
    SPACE = 0x20, /* ' '  */
    COLON = ':',
    BANG  = '!',
    AT    = '@',

    /* Pseudo-characters */
    CRLF  = 256,
};


enum status {
    kStatusOk = 0,
    kStatusEof,
    kStatusError,
    kStatusIoError,
};


struct parser {
    irc_message_t *msg;
    w_io_t        *input;
    int            look;
    bool           ended;
    uint32_t       curbyte;
    const char    *error;
};


#define P struct parser *p
#define S enum   status *status

#define CHECK_OK status); \
    if (*status != kStatusOk) return; \
    ((void) 0
#define DUMMY ) /* Makes autoindent work */
#undef DUMMY


static void
nextchar (P, S)
{
    /* TODO: Detect I/O errors */
    switch ((p->look = w_io_getchar (p->input))) {
        case W_IO_EOF:
            *status = kStatusEof;
            break;
        case CR:
            if ((p->look = w_io_getchar (p->input)) == LF) {
                p->look = CRLF;
            } else {
                w_io_putback (p->input, p->look);
                p->look = CR;
            }
            /* fall-through */
        default:
            p->curbyte++;
    }
}


static void
skipspace (P, S)
{
    while (p->look == SPACE) {
        nextchar (p, CHECK_OK);
    }
}


static inline void
matchchar (P, int ch, const char *errmsg, S)
{
    if (p->look == ch) {
        nextchar (p, CHECK_OK);
    } else {
        p->error = errmsg ? errmsg : "unexpected input";
        *status = kStatusError;
    }
}


/*
 * <prefix> ::= <servername> | <nick> ['!' user ] ['@' host ]
 */
static void
parse_prefix (P, S)
{
    matchchar (p, ':', "Malformed prefix", CHECK_OK);

    while (p->look != SPACE && p->look != BANG && p->look != CRLF) {
        w_buf_append_char (&p->msg->prefix.nick, p->look);
        nextchar (p, CHECK_OK);
    }
    if (w_buf_size (&p->msg->prefix.nick) == 0) {
        p->error = "Nick or servername missing in prefix";
        *status = kStatusError;
        return;
    }

    if (p->look == BANG) {
        nextchar (p, CHECK_OK);
        while (p->look != SPACE && p->look != AT && p->look != CRLF) {
            w_buf_append_char (&p->msg->prefix.user, p->look);
            nextchar (p, CHECK_OK);
        }
        if (w_buf_size (&p->msg->prefix.user) == 0) {
            p->error = "User missing in prefix";
            *status = kStatusError;
            return;
        }
    }

    if (p->look == AT) {
        nextchar (p, CHECK_OK);
        while (p->look != SPACE && p->look != CRLF) {
            w_buf_append_char (&p->msg->prefix.host, p->look);
            nextchar (p, CHECK_OK);
        }
        if (w_buf_size (&p->msg->prefix.host) == 0) {
            p->error = "Host missing in prefix";
            *status = kStatusError;
            return;
        }
    }
}


static inline
bool is_command_char (int ch)
{
    return (ch >= 'A' && ch <= 'Z')
        || (ch >= 'a' && ch <= 'z')
        || (ch >= '0' && ch <= '9');
}


static inline void
parse_command (P, S)
{
    while (p->look != SPACE && p->look != CRLF) {
        w_buf_append_char (&p->msg->cmd_text, p->look);
        nextchar (p, CHECK_OK);
    }
    if (w_buf_size (&p->msg->cmd_text) == 0) {
        p->error = "Missing command";
        *status = kStatusError;
    }
}


/*
 * <message> ::= [':' <prefix> <SPACE> ] <command> <params> <crlf>
 */
static inline void
parse_message (P, S)
{
    nextchar (p, CHECK_OK);
    if (p->look == COLON) {
        parse_prefix (p, CHECK_OK);
        matchchar (p, SPACE, "Space expected", CHECK_OK);
        skipspace (p, CHECK_OK);
    }

    parse_command (p, CHECK_OK);

    if (p->look == SPACE) {
        skipspace (p, CHECK_OK);
        while (p->look != CRLF) {
            w_buf_append_char (&p->msg->params_text, p->look);
            nextchar (p, CHECK_OK);
        }
    }

    if (p->look != CRLF && *status != kStatusEof) {
        p->error = "CRLF message terminator missing";
        *status = kStatusError;
    }
}


bool
irc_message_parse (irc_message_t *msg, w_io_t *input)
{
    w_assert (msg);
    w_assert (input);

    struct parser p = { .msg = msg, .input = input, 0 };
    enum status status = kStatusOk;
    parse_message (&p, &status);
    switch (status) {
        case kStatusOk:
            return true;
        case kStatusEof:
        case kStatusError:
        case kStatusIoError:
            return false;
    }
}
