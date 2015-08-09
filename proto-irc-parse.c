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
#define SAVECHAR( ) \
    w_buf_append_char (&p->msg->cmd_text, p->look); \
    nextchar (p, CHECK_OK)
#define MATCHSAVE(matchch) \
    w_buf_append_char (&p->msg->cmd_text, p->look); \
    if (p->look != (matchch)) goto read_command_rest; \
    nextchar (p, CHECK_OK)
#define RETCMD(_cmd) \
    if (p->look == SPACE || p->look == CRLF) { \
        p->msg->cmd = IRC_CMD_ ## _cmd; return; \
    } \
    goto read_command_rest

    switch (p->look) {
        case 'A': /* A{DMIN,WAY} */
            SAVECHAR ();
            switch (p->look) {
                case 'D': /* ADMIN */
                    MATCHSAVE ('M'); MATCHSAVE ('I'); MATCHSAVE ('N');
                    RETCMD (ADMIN);
                case 'W': /* AWAY */
                    MATCHSAVE ('A'); MATCHSAVE ('Y');
                    RETCMD (AWAY);
            }
            goto read_command_rest;

        case 'C': /* CONNECT */
            SAVECHAR ();
            MATCHSAVE ('O'); MATCHSAVE ('N'); MATCHSAVE ('N');
            MATCHSAVE ('E'); MATCHSAVE ('C'); MATCHSAVE ('T');
            RETCMD (CONNECT);

        case 'E': /* ERROR */
            SAVECHAR ();
            MATCHSAVE ('R'); MATCHSAVE ('R'); MATCHSAVE ('O'); MATCHSAVE ('R');
            RETCMD (ERROR);

        case 'I': /* I{NFO,NVITE,SON} */
            SAVECHAR ();
            switch (p->look) {
                case 'N': /* IN{FO,VITE} */
                    SAVECHAR ();
                    switch (p->look) {
                        case 'F':
                            SAVECHAR ();
                            MATCHSAVE ('O');
                            RETCMD (INFO);
                        case 'V':
                            SAVECHAR ();
                            MATCHSAVE ('I'); MATCHSAVE ('T'); MATCHSAVE ('E');
                            RETCMD (INVITE);
                    }
                    goto read_command_rest;
                case 'S': /* ISON */
                    SAVECHAR ();
                    MATCHSAVE ('O'); MATCHSAVE ('N');
                    RETCMD (ISON);
            }
            goto read_command_rest;

        case 'J': /* JOIN */
            SAVECHAR ();
            MATCHSAVE ('O'); MATCHSAVE ('I'); MATCHSAVE ('N');
            RETCMD (JOIN);

        case 'K': /* K{ICK,ILL} */
            SAVECHAR ();
            MATCHSAVE ('I');
            switch (p->look) {
                case 'C': /* KICK */
                    SAVECHAR ();
                    MATCHSAVE ('K');
                    RETCMD (KICK);
                case 'L': /* KILL */
                    SAVECHAR ();
                    MATCHSAVE ('L');
                    RETCMD (KILL);
            }
            goto read_command_rest;

        case 'L': /* L{INKS,IST} */
            SAVECHAR ();
            MATCHSAVE ('I');
            switch (p->look) {
                case 'N': /* LINKS */
                    SAVECHAR ();
                    MATCHSAVE ('K'); MATCHSAVE ('S');
                    RETCMD (LINKS);
                case 'S': /* LIST */
                    SAVECHAR ();
                    MATCHSAVE ('T');
                    RETCMD (LIST);
            }
            goto read_command_rest;

        case 'M': /* MODE */
            SAVECHAR ();
            MATCHSAVE ('O'); MATCHSAVE ('D'); MATCHSAVE ('E');
            RETCMD (MODE);

        case 'N': /* N{AMES,ICK,OTICE} */
            SAVECHAR ();
            switch (p->look) {
                case 'A': /* NAMES */
                    SAVECHAR ();
                    MATCHSAVE ('M'); MATCHSAVE ('E'); MATCHSAVE ('S');
                    RETCMD (NAMES);
                case 'I': /* NICK */
                    SAVECHAR ();
                    MATCHSAVE ('C'); MATCHSAVE ('K');
                    RETCMD (NICK);
                case 'O': /* NOTICE */
                    SAVECHAR ();
                    MATCHSAVE ('T'); MATCHSAVE ('I');
                    MATCHSAVE ('C'); MATCHSAVE ('E');
                    RETCMD (NOTICE);
            }
            goto read_command_rest;

        case 'O': /* OPER */
            SAVECHAR ();
            MATCHSAVE ('P'); MATCHSAVE ('E'); MATCHSAVE ('R');
            RETCMD (OPER);

        case 'P': /* P{ART,ASS,ING,ONG,RIVMSG} */
            SAVECHAR ();
            switch (p->look) {
                case 'A': /* PA{RT,SS} */
                    SAVECHAR ();
                    switch (p->look) {
                        case 'R': /* PART */
                            SAVECHAR ();
                            MATCHSAVE ('T');
                            RETCMD (PART);
                        case 'S': /* PASS */
                            SAVECHAR ();
                            MATCHSAVE ('S');
                            RETCMD (PASS);
                    }
                    goto read_command_rest;
                case 'I': /* PING */
                    SAVECHAR ();
                    MATCHSAVE ('N'); MATCHSAVE ('G');
                    RETCMD (PING);
                case 'O': /* PONG */
                    SAVECHAR ();
                    MATCHSAVE ('N'); MATCHSAVE ('G');
                    RETCMD (PONG);
                case 'R': /* PRIVMSG */
                    SAVECHAR ();
                    MATCHSAVE ('I'); MATCHSAVE ('V');
                    MATCHSAVE ('M'); MATCHSAVE ('S'); MATCHSAVE ('G');
                    RETCMD (PRIVMSG);
            }
            goto read_command_rest;

        case 'Q': /* QUIT */
            SAVECHAR ();
            MATCHSAVE ('U'); MATCHSAVE ('I'); MATCHSAVE ('T');
            RETCMD (QUIT);

        case 'R': /* R{EHASH,ESTART} */
            SAVECHAR ();
            MATCHSAVE ('E');
            switch (p->look) {
                case 'H': /* REHASH */
                    SAVECHAR ();
                    MATCHSAVE ('A'); MATCHSAVE ('S'); MATCHSAVE ('H');
                    RETCMD (REHASH);
                case 'S': /* RESTART */
                    SAVECHAR ();
                    MATCHSAVE ('T'); MATCHSAVE ('A');
                    MATCHSAVE ('R'); MATCHSAVE ('T');
                    RETCMD (RESTART);
            }
            goto read_command_rest;

        case 'S': /* S{ERVER,QUIT,TATS,UMMON */
            SAVECHAR ();
            switch (p->look) {
                case 'E': /* SERVER */
                    SAVECHAR ();
                    MATCHSAVE ('R'); MATCHSAVE ('V');
                    MATCHSAVE ('E'); MATCHSAVE ('R');
                    RETCMD (SERVER);
                case 'Q': /* SQUIT */
                    SAVECHAR ();
                    MATCHSAVE ('U'); MATCHSAVE ('I'); MATCHSAVE ('T');
                    RETCMD (SQUIT);
                case 'T': /* STATS */
                    SAVECHAR ();
                    MATCHSAVE ('A'); MATCHSAVE ('T'); MATCHSAVE ('S');
                    RETCMD (STATS);
                case 'U': /* SUMMON */
                    SAVECHAR ();
                    MATCHSAVE ('M'); MATCHSAVE ('M');
                    MATCHSAVE ('O'); MATCHSAVE ('N');
                    RETCMD (SUMMON);
            }
            goto read_command_rest;

        case 'T': /* T{IME,OPIC,RACE} */
            SAVECHAR ();
            switch (p->look) {
                case 'I': /* TIME */
                    SAVECHAR ();
                    MATCHSAVE ('M'); MATCHSAVE ('E');
                    RETCMD (TIME);
                case 'O': /* TOPIC */
                    SAVECHAR ();
                    MATCHSAVE ('P'); MATCHSAVE ('I'); MATCHSAVE ('C');
                    RETCMD (TOPIC);
                case 'R': /* TRACE */
                    SAVECHAR ();
                    MATCHSAVE ('A'); MATCHSAVE ('C'); MATCHSAVE ('E');
                    RETCMD (TRACE);
            }
            goto read_command_rest;

        case 'U': /* U{SER,SERHOST,SERS} */
            SAVECHAR ();
            MATCHSAVE ('S'); MATCHSAVE ('E'); MATCHSAVE ('R');
            switch (p->look) {
                case SPACE: /* USER */
                case CRLF:
                    RETCMD (USER);
                case 'H': /* USERHOST */
                    SAVECHAR ();
                    MATCHSAVE ('H'); MATCHSAVE ('O');
                    MATCHSAVE ('S'); MATCHSAVE ('T');
                    RETCMD (USERHOST);
                case 'S':
                    SAVECHAR ();
                    RETCMD (USERS);
            }
            goto read_command_rest;

        case 'V': /* VERSION */
            SAVECHAR ();
            MATCHSAVE ('E'); MATCHSAVE ('R'); MATCHSAVE ('S');
            MATCHSAVE ('I'); MATCHSAVE ('O'); MATCHSAVE ('N');
            RETCMD (VERSION);

        case 'W': /* W{ALLOPS,HO,HOIS,HOWAS} */
            SAVECHAR ();
            switch (p->look) {
                case 'A': /* WALLOPS */
                    SAVECHAR ();
                    MATCHSAVE ('L'); MATCHSAVE ('L');
                    MATCHSAVE ('O'); MATCHSAVE ('P'); MATCHSAVE ('S');
                    RETCMD (WALLOPS);
                default: /* WHO{,IS,WAS} */
                    MATCHSAVE ('H');
                    MATCHSAVE ('O');
                    switch (p->look) {
                        case SPACE: /* WHO */
                        case CRLF:
                            RETCMD (WHO);
                        case 'I': /* WHOIS */
                            SAVECHAR ();
                            MATCHSAVE ('S');
                            RETCMD (WHOIS);
                        case 'W': /* WHOWAS */
                            SAVECHAR ();
                            MATCHSAVE ('A'); MATCHSAVE ('S');
                            RETCMD (WHOWAS);
                    }
            }
    }

read_command_rest:
    while (p->look != SPACE && p->look != CRLF) {
        SAVECHAR ();
    }
    if (w_buf_size (&p->msg->cmd_text) == 0) {
        p->error = "Missing command";
        *status = kStatusError;
    }

#undef RETCMD
#undef SAVECHAR
#undef MATCHSAVE
}


static inline void
parse_param (P, S)
{
    if (p->msg->n_params >= IRC_MAX_PARAMS) {
        p->error = "Too many parameters";
        *status = kStatusError;
        return;
    }

    if (w_buf_size (&p->msg->params_text) > 0) {
        w_buf_append_char (&p->msg->params_text, ' ');
    }

    size_t start_pos = w_buf_size (&p->msg->params_text);

    if (p->look == COLON) {
        w_buf_append_char (&p->msg->params_text, COLON);
        start_pos++;
        /* Read parameter until the end of the line. */
        nextchar (p, CHECK_OK);
        while (p->look != CRLF) {
            w_buf_append_char (&p->msg->params_text, p->look);
            nextchar (p, CHECK_OK);
        }
    } else {
        /* Read parameter until space or end of line. */
        while (p->look != SPACE && p->look != CRLF) {
            w_buf_append_char (&p->msg->params_text, p->look);
            nextchar (p, CHECK_OK);
        }
    }

    /* Arrange for params[n] to point to the data at params_text */
    p->msg->params[p->msg->n_params++] = (w_buf_t) {
        .data = w_buf_data (&p->msg->params_text) + start_pos,
        .size = w_buf_size (&p->msg->params_text) - start_pos,
    };
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

    while (p->look == SPACE) {
        skipspace (p, CHECK_OK);
        parse_param (p, CHECK_OK);
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
