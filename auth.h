/*
 * auth.h
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef AUTH_H
#define AUTH_H

#include "wheel/wheel.h"

W_OBJ_DECL (auth_agent_t);

W_OBJ_DEF (auth_agent_t)
{
    w_obj_t parent;
    bool (*authenticate) (auth_agent_t *agent,
                          const char   *user,
                          const char   *pass);
};


static inline void
auth_agent_init (auth_agent_t *agent,
                 bool (*authenticate) (auth_agent_t*, const char*, const char*))
{
    w_assert (agent);
    agent->authenticate = authenticate;
}

static inline bool
auth_agent_authenticate (auth_agent_t *agent,
                         const char *user,
                         const char *pass)
{
    w_assert (agent);
    w_assert (user);
    w_assert (pass);

    return agent->authenticate
        ? (*agent->authenticate) (agent, user, pass)
        : false;
}


extern auth_agent_t* auth_pam_agent_new (const char *service);


typedef struct {
    const char *user;
    const char *pass;
} auth_simple_mem_agent_entry_t;

extern auth_agent_t*
auth_simple_mem_agent_new (const auth_simple_mem_agent_entry_t *entries);


#endif /* !AUTH_H */
