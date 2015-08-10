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


typedef struct {
    const char *bind;
    unsigned    port;
    void      (*conn) (w_io_t*);
} Listener;


static void
listen_task (void *arg)
{
    const Listener *listener = (Listener*) arg;

    struct sockaddr_in sa;
    memset (&sa, 0x00, sizeof (struct sockaddr_in));
    sa.sin_family = AF_INET;
    sa.sin_port   = htons (listener->port);
    if (listener->bind) {
        /* TODO */
    }

    w_printerr ("$s: Listening on $s:$I\n", w_task_name (),
                listener->bind ? listener->bind : "*", listener->port);
    int fd = socket (AF_INET, SOCK_STREAM, 0);
    if (fd < 0) w_die ("$s: Cannot create socket: $E\n", w_task_name ());

    socklen_t slen;
    int n;
    if (getsockopt (fd, SOL_SOCKET, SO_TYPE, (void*) &n, &slen) >= 0) {
        n = 1;
        setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, (char*) &n, sizeof (int));
    }

    if (bind (fd, (struct sockaddr*) &sa, sizeof (struct sockaddr_in)) == -1)
        w_die ("$s: Cannot bind socket: $E\n", w_task_name ());

    if (listen (fd, 16) == -1)
        w_die ("$s: Cannot listen on socket: $E\n", w_task_name ());

    int flags = fcntl (fd, F_GETFL);
    if (flags < 0 || fcntl (fd, F_SETFL, flags | O_NONBLOCK) == -1)
        w_die ("$s: Cannot set socket as non-blocking: $E\n", w_task_name ());


    for (;;) {
        slen = sizeof (struct sockaddr_in);
        int new_fd = accept (fd, (void*) &sa, &slen);
        if (new_fd >= 0) {
            n = 1;
            setsockopt (new_fd, IPPROTO_TCP, TCP_NODELAY, (char*) &n, sizeof (int));
            w_io_t *client_io = w_io_unix_open_fd (new_fd);
            w_task_t *task = w_task_prepare ((w_task_func_t) listener->conn,
                                             w_io_task_open (client_io),
                                             16384);
            w_obj_unref (client_io);

            w_buf_t namebuf = W_BUF;
            const char *addr = (const char*) &sa.sin_addr;
            w_buf_format (&namebuf, "$s<$i.$i.$i.$i>", w_task_name (),
                          addr[0], addr[1], addr[2], addr[3]);
            w_task_set_name (task, w_buf_str (&namebuf));
            w_buf_clear (&namebuf);
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            w_task_yield ();
        } else {
            w_die ("$s: Error accepting connection: $E\n", w_task_name ());
        }
    }

    close (fd);
}


extern void proto_irc_worker  (w_io_t *socket);
extern void proto_xmpp_worker (w_io_t *socket);


static const auth_simple_mem_agent_entry_t auth_users[] = {
    { "op",  "op3rat0r" },
    { "joe", "jo3jo3"   },
    { "tom", "t0mt0m"   },
    { NULL },
};
auth_agent_t *auth_agent = NULL;


int
main (int argc, char **argv)
{
    auth_agent = auth_simple_mem_agent_new (auth_users);

    w_task_t *task;

    task = w_task_prepare (listen_task,
                           &((Listener) {
                             .port = 6689,
                             .conn = proto_irc_worker,
                           }), 16384);
    w_task_set_name (task, "IRC");

    task = w_task_prepare (listen_task,
                           &((Listener) {
                             .port = 5269,
                             .conn = proto_xmpp_worker,
                           }), 16384);
    w_task_set_name (task, "XMPP");

    w_task_run_scheduler ();

    w_obj_unref (auth_agent);
    return 0;
}

