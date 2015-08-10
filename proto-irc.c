/*
 * proto-irc.c
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "proto-irc.h"
#include "auth.h"


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



static inline bool
check_nparams (const irc_message_t *msg)
{
    if (msg->cmd == IRC_CMD_UNKNOWN)
        return true;

    int8_t nparam = 0, noptparam = 0;
    if (!irc_cmd_info (msg->cmd, &nparam, &noptparam)) {
        return false;
    } else if (noptparam < 0) {
        /* Any number of optional parameters allowed. */
        return msg->n_params >= nparam;
    } else {
        return msg->n_params >= nparam && msg->n_params <= (nparam + noptparam);
    }
}


static w_io_result_t
send_error (w_task_listener_t *listener,
            w_io_t            *socket,
            irc_rpl_t          code,
            ...)
{
    va_list args;
    va_start (args, code);

    const char *format;
    const char *name;
    uint8_t narg;

    bool got_info = irc_rpl_info (code, &narg, &name, &format);
    w_assert (got_info); /* TODO: Signal server error here. */
    w_unused (got_info);

    w_io_result_t r = W_IO_RESULT (0);

    /* TODO: Sender prefix, target */
    W_IO_CHAIN (r, w_io_format (socket, ":* $I * ", (unsigned) code));
    W_IO_CHAIN (r, w_io_formatv (socket, format, args));
    W_IO_CHAIN (r, w_io_putchar (socket, CR));
    W_IO_CHAIN (r, w_io_putchar (socket, LF));
    W_IO_CHAIN (r, w_io_flush (socket));

    va_end (args);
}


void
proto_irc_handler (w_task_listener_t *listener, w_io_t *socket)
{
    w_printerr ("$s: Client connected\n", w_task_name ());

    auth_agent_t *auth_agent = listener->userdata;
    irc_message_t message = { 0, };

    w_buf_t user = W_BUF;
    w_buf_t pass = W_BUF;
    bool got_user = false; /* TODO: Change to an actual user object. */

    for (;; irc_message_reset (&message)) {
        if (!irc_message_parse (&message, socket)) {
            /* Return error to the client */
            break;
        }

        w_printerr ("origin : $B\n"
                    "user   : $B\n"
                    "host   : $B\n"
                    "command: $B ($s)\n"
                    "params : $B ($I)\n",
                    &message.prefix.nick,
                    &message.prefix.user,
                    &message.prefix.host,
                    &message.cmd_text,
                    irc_cmd_name (message.cmd),
                    &message.params_text,
                    (unsigned) message.n_params);
        for (uint8_t i = 0; i < message.n_params; i++)
            w_printerr ("  $I: $B¬\n", (unsigned) i, &message.params[i]);
        w_printerr ("-----\n");

        switch (message.cmd) {
            case IRC_CMD_NICK:
                if (!check_nparams (&message)) {
                    send_error (listener, socket, IRC_RPL_NONICKNAMEGIVEN);
                } else {
                    w_buf_clear (&user);
                    w_buf_append_buf (&user, &message.params[0]);
                    if (got_user) {
                        send_error (listener, socket, IRC_RPL_ERRONEUSNICKNAME,
                                    w_buf_str (&user));
                    } else {
                        if (w_buf_size (&user) && w_buf_size (&pass)) {
                            if (auth_agent_authenticate (auth_agent,
                                                         w_buf_str (&user),
                                                         w_buf_str (&pass))) {
                                got_user = true;
                            } else {
                                send_error (listener, socket, IRC_RPL_PASSWDMISMATCH);
                                goto close_connection;
                            }
                        }
                    }
                }
                break;

            case IRC_CMD_PASS:
                if (!check_nparams (&message)) {
                    send_error (listener, socket, IRC_RPL_NEEDMOREPARAMS,
                                w_buf_str (&message.cmd_text));
                } else {
                    w_buf_clear (&pass);
                    w_buf_append_buf (&pass, &message.params[0]);
                    if (got_user) {
                        send_error (listener, socket, IRC_RPL_ALREADYREGISTERED);
                    } else {
                        if (w_buf_size (&user) && w_buf_size (&pass)) {
                            if (auth_agent_authenticate (auth_agent,
                                                         w_buf_str (&user),
                                                         w_buf_str (&pass))) {
                                got_user = true;
                            } else {
                                send_error (listener, socket, IRC_RPL_PASSWDMISMATCH);
                                goto close_connection;
                            }
                        }
                    }
                }
                break;
        }
    }

close_connection:
    W_IO_NORESULT (w_io_flush (socket));
    W_IO_NORESULT (w_io_close (socket));
    w_printerr ("$s: Connection closed\n", w_task_name ());
}

