/*
 * proto-irc.h
 * Copyright (C) 2015 Adrian Perez <aperez@igalia.com>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef PROTO_IRC_H
#define PROTO_IRC_H

#include "wheel/wheel.h"

/*
 * Standard RFC1459 IRC commands are listed in the form (N, M, NAME), where:
 *   - N is the number of mandatory parameters.
 *   - M is the number of optional parameters, when positive. A negative value
 *     indicates a variable amount of paramters up to IRC_MAX_PARAMS.
 *   - NAME is the name of the command.
 * The total maximum amount of allowed parameters is for a command is: N+M.
 */

/* 4.1 Connection registration */
#define IRC_CONNREG_CMDS(F) \
    F (1, 0, PASS)   \
    F (1, 1, NICK)   \
    F (4, 0, USER)   \
    F (3, 0, SERVER) \
    F (2, 0, OPER)   \
    F (0, 1, QUIT)   \
    F (2, 0, SQUIT)

/* 4.2 Channel operations */
#define IRC_CHANOPS_CMDS(F) \
    F (1, 1, JOIN)   \
    F (1, 0, PART)   \
    F (2, 3, MODE)   \
    F (1, 1, TOPIC)  \
    F (0, 1, NAMES)  \
    F (0, 2, LIST)   \
    F (2, 0, INVITE) \
    F (2, 1, KICK)

/* 4.3 Server queries and commands */
#define IRC_SRVQUERY_CMDS(F) \
    F (0, 1, VERSION) \
    F (0, 2, STATS)   \
    F (0, 2, LINKS)   \
    F (0, 1, TIME)    \
    F (1, 2, CONNECT) \
    F (0, 1, TRACE)   \
    F (0, 1, ADMIN)   \
    F (0, 1, INFO)

/* 4.4 Sending messages */
#define IRC_MSGSEND_CMDS(F) \
    F (2, 0, PRIVMSG) \
    F (2, 0, NOTICE)

/* 4.5 User-based queries */
#define IRC_USRQUERY_CMDS(F) \
    F (0, 2, WHO)   \
    F (1, 1, WHOIS) \
    F (1, 2, WHOWAS)

/* 4.6 Miscellaneous messages */
#define IRC_MISC_CMDS(F) \
    F (2, 0, KILL) \
    F (1, 1, PING) \
    F (1, 1, PONG) \
    F (1, 0, ERROR)

/* 5 Optionals */
#define IRC_OPTIONAL_CMDS(F) \
    F (0, 1, AWAY)     \
    F (0, 0, REHASH)   \
    F (0, 0, RESTART)  \
    F (1, 1, SUMMON)   \
    F (0, 1, USERS)    \
    F (1, 0, WALLOPS)  \
    F (1,-1, USERHOST) \
    F (1,-1, ISON)

#define IRC_MANDATORY_CMDS(F) \
    IRC_CONNREG_CMDS  (F) \
    IRC_CHANOPS_CMDS  (F) \
    IRC_SRVQUERY_CMDS (F) \
    IRC_MSGSEND_CMDS  (F) \
    IRC_USRQUERY_CMDS (F) \
    IRC_MISC_CMDS     (F)

#define IRC_ALL_CMDS(F) \
    IRC_MANDATORY_CMDS (F) \
    IRC_OPTIONAL_CMDS  (F)


/* 6.1 Error replies */
#define IRC_ERROR_RPLS(F) \
    F (401, 1, NOSUCHNICK,       "$s :No such nick/channel")                       \
    F (402, 1, NOSUCHSERVER,     "$s :No such server")                             \
    F (403, 1, NOSUCHCHANNEL,    "$s :No such channel")                            \
    F (404, 1, CANNOTSENDTOCHAN, "$s :Cannot send to channel")                     \
    F (405, 1, TOOMANYCHANNELS,  "$s :You have joined too many channels")          \
    F (406, 1, WASNOSUCHNICK,    "$s :There was no such nickname")                 \
    F (407, 1, TOOMANYTARGETS,   "$s :Duplicate recipients. No message delivered") \
    F (409, 0, NOORIGIN,         ":No origin speficied")                           \
    F (411, 1, NORECIPIENT,      ":No recipient given ($s)")                       \
    F (412, 0, NOTEXTTOSEND,     ":No text to send")                               \
    F (413, 1, NOTOPLEVEL,       "$s :No toplevel domain specified")               \
    F (414, 1, WILDTOPLEVEL,     "$s :Wildcard in toplevel domain")                \
    F (421, 1, UNKNOWNCOMMAND,   "$s :Unknown command")                            \
    F (422, 0, NOMOTD,           ":MOTD File is missing")                          \
    F (423, 1, NOADMININFO,      "$s :No administrative info available")           \
    F (424, 2, FILEERROR,        ":File error doing $s on $s")                     \
    F (431, 0, NONICKNAMEGIVEN,  ":No nickname given")                             \
    F (432, 1, ERRONEUSNICKNAME, "$s :Erroneus nickname")                          \
    F (433, 1, NICKNAMEINUSE,    "$s :Nickname already in use")                    \
    F (436, 1, NICKCOLLISION,    "$s :Nickname collision KILL")                    \
    F (441, 2, USERNOTINCHANNEL, "$s $s :They aren't on that channel")             \
    F (442, 1, NOTONCHANNEL,     "$s :You're not on that channel")                 \
    F (443, 2, USERONCHANNEL,    "$s $s :is already on channel")                   \
    F (444, 1, NOLOGIN,          "$s :User not logged in")                         \
    F (445, 0, SUMMONDISABLED,   ":SUMMON has been disabled")                      \
    F (446, 0, USERSDISABLED,    ":USERS has been disabled")                       \
    F (451, 0, NOTREGISTERED,    ":You have not been registered")                  \
    F (461, 1, NEEDMOREPARAMS,   "$s :Not enough parameters")                      \
    F (462, 0, ALREADYREGISTERED,":You may not reregister")                        \
    F (463, 0, NOPERMFORHOST,    ":Your host isn't among the privileged")          \
    F (464, 0, PASSWDMISMATCH,   ":Password incorrect")                            \
    F (465, 0, YOUREBANNEDCREEP, ":You are banned from this server")               \
    F (457, 1, KEYSET,           "$s :Channel key already set")                    \
    F (471, 1, CHANNELISFULL,    "$s :Cannot join channel (+l)")                   \
    F (472, 1, UNKNOWNMODE,      "$s :is unknown mode char for me")                \
    F (473, 1, INVITEONLYCHAN,   "$s :Cannot join channel (+i)")                   \
    F (474, 1, BANNEDFROMCHAN,   "$s :Cannot join channel (+b)")                   \
    F (475, 1, BADCHANNELKEY,    "$s :Cannot join channel (+k)")                   \
    F (481, 0, NOPRIVILEGES,     ":Permission Denied- You're not an IRC operator") \
    F (482, 1, CHANOPRIVSNEEDED, "$s :You're not channel operator")                \
    F (483, 0, CANTKILLSERVER,   ":You cant kill a server!")                       \
    F (491, 0, NOOPERHOST,       ":No O-lines for your host")                      \
    F (501, 0, UMODEUNKNOWNFLAG, ":Unknown MODE flag")                             \
    F (502, 0, USERSDONTMATCH,   ":Cant change mode for other users")

/* 6.2 Command responses */
#define IRC_CMDRESP_RPLS(F) \
    F (300, 0, NONE,            "")                                       \
    F (302, 0, USERHOST,        NULL)                                     \
    F (303, 0, ISON,            NULL)                                     \
    F (301, 2, AWAY,            "$s :$s")                                 \
    F (305, 0, UNAWAY,          ":You're no longer marked as being away") \
    F (306, 0, NOWAWAY,         ":You have been marked as being away")    \
    F (311, 4, WHOISUSER,       "$s $s $s * :$s")                         \
    F (312, 3, WHOISSERVER,     "$s $s :$s")                              \
    F (313, 1, WHOISOPERATOR,   "$s :is an IRC operator")                 \
    F (317, 2, WHOISIDLE,       "$s $I :seconds idle")                    \
    F (318, 1, ENDOFWHOIS,      "$s :End of /WHOIS list")                 \
    F (319, 0, WHOISCHANNELS,   NULL)                                     \
    F (314, 4, WHOWASUSER,      "$s $s $s * :$s")                         \
    F (369, 1, ENDOFWHOWAS,     "$s :End of WHOWAS")                      \
    F (321, 0, LISTSTART,       "Channel :Users  Name")                   \
    F (322, 3, LIST,            "$s $I :$s")                              \
    F (323, 0, LISTEND,         ":End of /LIST")                          \
    F (324, 3, CHANNELMODEIS,   "$s $s $s")                               \
    F (331, 1, NOTOPIC,         "$s :No topic is set")                    \
    F (332, 2, TOPIC,           "$s :$s")                                 \
    F (341, 2, INVITING,        "$s $s")                                  \
    F (342, 1, SUMMONING,       "$s :Summoning user to IRC")              \
    F (351, 4, VERSION,         "$s.$s $s :$s")                           \
    F (352, 8, WHOREPLY,        "$s $s $s $s $s $s :$I $s")               \
    F (315, 1, ENDOFWHO,        "$s :End of /WHO list")                   \
    F (353, 0, NAMREPLY,        NULL)                                     \
    F (366, 1, ENDOFNAMES,      "$s :End of /NAMES list")                 \
    F (364, 4, LINKS,           "$s $s :$I $s")                           \
    F (365, 1, ENDOFLINKS,      "$s :End of /LINKS list")                 \
    F (367, 2, BANLIST,         "$s $s")                                  \
    F (368, 1, ENDOFBANLIST,    "$s :End of channel ban list")            \
    F (371, 1, INFO,            ":$s")                                    \
    F (374, 0, ENDOFINFO,       ":End of /INFO list")                     \
    F (375, 1, MOTDSTART,       ":- $s Message of the day - ")            \
    F (372, 1, MOTD,            ":- $s")                                  \
    F (376, 0, ENDOFMOTD,       ":End of /MOTD command")                  \
    F (381, 0, YOUREOPER,       ":You are now an IRC operator")           \
    F (382, 1, REHASHING,       "$s :Rehashing")                          \
    F (391, 0, TIME,            NULL)                                     \
    F (392, 0, USERSSTART,      ":UserID   Terminal  Host")               \
    F (393, 0, USERS,           NULL)                                     \
    F (394, 0, ENDOFUSERS,      ":End of users")                          \
    F (395, 0, NOUSERS,         ":Nobody logged in")                      \
    F (200, 3, TRACELINK,       "$s $s $s")                               \
    F (201, 2, TRACECONNECTING, "Try. $s $s")                             \
    F (202, 2, TRACEHANDSHAKE,  "H.S. $s $s")                             \
    F (203, 2, TRACEUNKNOWN,    "???? $s $s")                             \
    F (204, 2, TRACEOPERATOR,   "Oper $s $s")                             \
    F (205, 2, TRACEUSER,       "User $s $s")                             \
    F (206, 5, TRACESERVER,     "Serv $s $iS $iC $s $s")                  \
    F (208, 2, TRACENEWTYPE,    "$s 0 $s")                                \
    F (261, 2, TRACELOG,        "File $s $s")                             \
    F (211, 7, STATSLINKINFO,   "$s $I $I $I $I $I $I")                   \
    F (212, 2, STATSCOMMANDS,   "$s $I")                                  \
    F (213, 4, STATSCLINE,      "C $s * $s $I $s")                        \
    F (214, 4, STATSNLINE,      "N $s * $s $I $s")                        \
    F (215, 4, STATSILINE,      "I $s * $s $I $s")                        \
    F (216, 4, STATSKLINE,      "K $s * $s $I $s")                        \
    F (218, 4, STATSYLINE,      "Y $s $I $I $I")                          \
    F (219, 1, ENDOFSTATS,      "$s :End of /STATS report")               \
    F (241, 3, STATSLLINE,      "L $s * $s $I")                           \
    F (242, 0, STATSUPTIME,     NULL)                                     \
    F (243, 2, STATSOLINE,      "O $s * $s")                              \
    F (244, 2, STATSHLINE,      "H $s * $s")                              \
    F (221, 1, UMODEIS,         "$s")                                     \
    F (251, 3, LUSERCLIENT,     ":There are $I users and $I invisible on $I servers") \
    F (252, 1, LUSEROP,         "$I :operator(s) online")                 \
    F (253, 1, LUSERUNKNOWN,    "$I :unknown connection(s)")              \
    F (254, 1, LUSERCHANNELS,   "$I :channels formed")                    \
    F (255, 2, LUSERME,         ":I have $I clients and $I servers")      \
    F (256, 1, ADMINME,         "$s :Administrative info")                \
    F (257, 1, ADMINLOC1,       ":$s")                                    \
    F (258, 1, ADMINLOC2,       ":$s")                                    \
    F (259, 1, ADMINEMAIL,      ":$s")                                    \

#define IRC_ALL_RPLS(F) \
    IRC_ERROR_RPLS   (F) \
    IRC_CMDRESP_RPLS (F)

typedef enum {
    IRC_CMD_UNKNOWN = 0,

#define IRC_CMD_ENUM_ITEM(nparam, noptparam, name) \
    IRC_CMD_ ## name,

    IRC_ALL_CMDS (IRC_CMD_ENUM_ITEM)

#undef IRC_CMD_ENUM_ITEM
} irc_cmd_t;


typedef enum {
#define IRC_RPL_ENUM_ITEM(code, narg, name, fmts) \
    IRC_RPL_ ## name = code,

    IRC_ALL_RPLS (IRC_RPL_ENUM_ITEM)

#undef IRC_RPL_ENUM_ITEM
} irc_rpl_t;


static inline bool
irc_rpl_info (irc_rpl_t code, uint8_t *narg, const char **name, const char **fmts)
{
    w_assert (narg);
    w_assert (name);
    w_assert (fmts);

    switch (code) {
#define IRC_RPL_SWITCH_ITEM(_code, _narg, _name, _fmts) \
        case IRC_RPL_ ## _name: \
            *narg = _narg;  \
            *name = #_name; \
            *fmts = _fmts;  \
            return true;

        IRC_ALL_RPLS (IRC_RPL_SWITCH_ITEM)

#undef IRC_RPL_SWITCH_ITEM
        default:
            return false;
    }
}


static inline const char*
irc_cmd_name (irc_cmd_t cmd)
{
    switch (cmd) {
#define IRC_CMD_SWITCH_ITEM(nparam, noptparam, _name) \
        case IRC_CMD_ ## _name: return "IRC_CMD_" #_name;

        IRC_ALL_CMDS (IRC_CMD_SWITCH_ITEM)

#undef IRC_CMD_SWITCH_ITEM
        default:
            return NULL;
    }
}


static inline const char*
irc_cmd_info (irc_cmd_t cmd,
              int8_t   *nparam,
              int8_t   *noptparam)
{
    switch (cmd) {
#define IRC_CMD_SWITCH_ITEM(_nparam, _noptparam, _name) \
        case IRC_CMD_ ## _name: \
            if (nparam) *nparam = (_nparam); \
            if (noptparam) *noptparam = (_noptparam); \
            return "IRC_CMD_" #_name;

        IRC_ALL_CMDS (IRC_CMD_SWITCH_ITEM)

#undef IRC_CMD_SWITCH_ITEM
    }
}


enum {
    IRC_MAX_PARAMS = 15,
};


typedef struct {
    struct {
        union {
            w_buf_t nick;
            w_buf_t servername;
        };
        w_buf_t     user;
        w_buf_t     host;
    } prefix;

    w_buf_t         cmd_text;
    irc_cmd_t       cmd;

    uint8_t         n_params;
    w_buf_t         params_text;
    w_buf_t         params[IRC_MAX_PARAMS];
} irc_message_t;


static inline void
irc_message_reset (irc_message_t *msg)
{
    w_assert (msg);
    w_buf_clear (&msg->prefix.nick);
    w_buf_clear (&msg->prefix.user);
    w_buf_clear (&msg->prefix.host);
    w_buf_clear (&msg->cmd_text);
    w_buf_clear (&msg->params_text);
    memset (msg, 0x00, sizeof (irc_message_t));
}


extern bool irc_message_parse (irc_message_t *msg, w_io_t *input);


#endif /* !PROTO_IRC_H */
