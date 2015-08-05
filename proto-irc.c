/*
 * proto-irc.c
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "proto-irc.h"

void
proto_irc_worker (w_io_t *socket)
{
    w_printerr ("$s: Client connected\n", w_task_name ());

    /* The IRC protocol is line-based. */
    irc_message_t message = { 0, };

    for (;; irc_message_reset (&message)) {
        if (!irc_message_parse (&message, socket)) {
            /* Return error to the client */
            break;
        }

        w_printerr ("origin : $B\n"
                    "user   : $B\n"
                    "host   : $B\n"
                    "command: $B\n"
                    "params : $B\n"
                    "-----\n",
                    &message.prefix.nick,
                    &message.prefix.user,
                    &message.prefix.host,
                    &message.cmd_text,
                    &message.params_text);
    }

    W_IO_NORESULT (w_io_close (socket));
    w_printerr ("$s: Connection closed\n", w_task_name ());
}

