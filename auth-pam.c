/*
 * auth-pam.c
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#include "auth.h"
#include <security/pam_appl.h>

#ifndef CHATEAU_PAM_SERVICE
#define CHATEAU_PAM_SERVICE "chateau"
#endif /* !CHATEAU_PAM_SERVICE */


W_OBJ (auth_pam_agent_t)
{
    auth_agent_t parent;
    char *service;
};


static void
auth_pam_agent_destroy (void *obj)
{
    auth_pam_agent_t *agent = obj;
    w_free (agent->service);
}


static bool
auth_pam_agent_authenticate (auth_agent_t *agent,
                             const char   *user,
                             const char   *pass)
{
    return false;
}


auth_agent_t*
auth_pam_agent_new (const char *service)
{
    if (!service) service = CHATEAU_PAM_SERVICE;
    auth_pam_agent_t *agent = w_obj_new (auth_pam_agent_t);
    auth_agent_init (&agent->parent, auth_pam_agent_authenticate);
    agent->service = w_str_dup (service);
    return w_obj_dtor (agent, auth_pam_agent_destroy);
}

