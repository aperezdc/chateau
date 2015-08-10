/*
 * chateaud.c
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "auth.h"
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

extern void proto_irc_handler  (w_task_listener_t*, w_io_t*);
extern void proto_xmpp_handler (w_task_listener_t*, w_io_t*);


static const auth_simple_mem_agent_entry_t auth_users[] = {
    { "op",  "op3rat0r" },
    { "joe", "jo3jo3"   },
    { "tom", "t0mt0m"   },
    { NULL },
};


int
main (int argc, char **argv)
{
    w_task_t *task;
    auth_agent_t *auth_agent = auth_simple_mem_agent_new (auth_users);

    w_task_listener_t *irc_listener =
            w_task_listener_new ("tcp:6686", proto_irc_handler, auth_agent);
    task = w_task_prepare (w_task_listener_run, irc_listener, 16384);
    w_task_set_name (task, "IRC");

    w_task_listener_t *xmpp_listener =
            w_task_listener_new ("tcp:5269", proto_xmpp_handler, auth_agent);
    task = w_task_prepare (w_task_listener_run, xmpp_listener, 16384);
    w_task_set_name (task, "XMPP");

    w_task_run_scheduler ();

    w_obj_unref (xmpp_listener);
    w_obj_unref (irc_listener);
    w_obj_unref (auth_agent);

    return 0;
}

