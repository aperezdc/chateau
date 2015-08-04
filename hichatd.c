/*
 * hichatd.c
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "wheel/wheel.h"
#include "irc-parser/irc_parser.h"
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


#define IRC_ITEMS(F) \
    F (nick)    \
    F (name)    \
    F (host)    \
    F (command) \
    F (param)

#define DEFINE_IRC_STRUCT_ITEMS(_name) \
    const char *_name; size_t _name ## _len;

typedef struct {
    IRC_ITEMS (DEFINE_IRC_STRUCT_ITEMS)
} IRCLine;

#undef DEFINE_IRC_STRUCT_ITEMS

#define DEFINE_IRC_STRUCT_ITEM_SAVE_FUNC(_name)     \
    static int on_irc_ ## _name (irc_parser *p,     \
                                 const char *at,    \
                                 size_t      len) { \
        IRCLine *l = (IRCLine*) p->data;            \
        l->_name = at;                              \
        l->_name ## _len = len;                     \
        return 0;                                   \
    }

IRC_ITEMS (DEFINE_IRC_STRUCT_ITEM_SAVE_FUNC)

#undef DEFINE_IRC_STRUCT_ITEM_SAVE_FUNC

static int
on_irc_end (irc_parser *p, const char *at, size_t len)
{
    return 0;
}

static int
on_irc_error (irc_parser *p, const char *at, size_t len)
{
    return 0;
}


static void
irc_worker (w_io_t *socket)
{
    w_printerr ("$s: IRC client connected\n", w_task_name ());

    /* The IRC protocol is line-based. */
    w_buf_t line = W_BUF;
    w_buf_t overflow = W_BUF;

    irc_parser_settings parser_settings;
    irc_parser_settings_init (&parser_settings,
                              on_irc_nick,
                              on_irc_name,
                              on_irc_host,
                              on_irc_command,
                              on_irc_param,
                              on_irc_end,
                              on_irc_error);

    irc_parser parser;
    irc_parser_init (&parser, &parser_settings);
    IRCLine irc_line;
    parser.data = &irc_line;

    for (;; w_buf_clear (&line)) {
        w_io_result_t r = w_io_read_line (socket, &line, &overflow, 0);
        if (w_io_failed (r)) {
            w_printerr ("$s: Read error: $R\n", w_task_name (), r);
            break;
        } else if (w_io_eof (r)) {
            w_printerr ("$s: Client disconnected\n", w_task_name ());
            break;
        } else if (w_buf_size (&line) == 0) {
            continue;
        }

        /* Parse the input line. */
        memset (&irc_line, 0x00, sizeof (IRCLine));
        irc_parser_execute (&parser,
                            w_buf_const_data (&line),
                            w_buf_size (&line));
        w_printerr ("$s: $S $S $S $S $S\n", w_task_name (),
                    irc_line.nick_len,    irc_line.nick,
                    irc_line.name_len,    irc_line.name,
                    irc_line.host_len,    irc_line.host,
                    irc_line.command_len, irc_line.command,
                    irc_line.param_len,   irc_line.param);
    }

    W_IO_NORESULT (w_io_close (socket));
}


static void
xmpp_worker (w_io_t *socket)
{
    w_printerr ("$s: XMPP client connected\n", w_task_name ());
    W_IO_NORESULT (w_io_close (socket));
}


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


int
main (int argc, char **argv)
{
    w_task_t *task;

    task = w_task_prepare (listen_task,
                           &((Listener) {
                             .port = 6689,
                             .conn = irc_worker,
                           }), 16384);
    w_task_set_name (task, "IRC");

    task = w_task_prepare (listen_task,
                           &((Listener) {
                             .port = 5269,
                             .conn = xmpp_worker,
                           }), 16384);
    w_task_set_name (task, "XMPP");

    w_task_run_scheduler ();
    return 0;
}

