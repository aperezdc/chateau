/*
 * auth-simple-mem.c
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "auth.h"


W_OBJ (auth_simple_mem_agent_t)
{
    auth_agent_t parent;
    const auth_simple_mem_agent_entry_t *entries;
};


static bool
auth_simple_mem_agent_authenticate (auth_agent_t *agent,
                                    const char   *user,
                                    const char   *pass)
{
    for (const auth_simple_mem_agent_entry_t *entry =
             ((auth_simple_mem_agent_t*) agent)->entries;
         entry->user;
         entry++)
    {
        if (strcmp (user, entry->user) == 0) {
            return strcmp (pass, entry->pass) == 0;
        }
    }
    return false;
}


auth_agent_t*
auth_simple_mem_agent_new (const auth_simple_mem_agent_entry_t *entries)
{
    w_assert (entries);

    auth_simple_mem_agent_t *agent = w_obj_new (auth_simple_mem_agent_t);
    auth_agent_init (&agent->parent, auth_simple_mem_agent_authenticate);
    agent->entries = entries;

    return (auth_agent_t*) agent;
}
