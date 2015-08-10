/*
 * proto-xmpp.c
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "wheel/wheel.h"


void
proto_xmpp_handler (w_task_listener_t *listener, w_io_t *socket)
{
    w_printerr ("$s: Client connected\n", w_task_name ());
    W_IO_NORESULT (w_io_close (socket));
}
