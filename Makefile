#
# Makefile
# Adrian Perez, 2015-08-04 01:05
#

all: chateaud

libwheel_PATH := wheel
include wheel/Makefile.libwheel

chateaud_SRCS := chateaud.c \
                 proto-xmpp.c \
                 proto-irc.c proto-irc-parse.c \
                 auth-simple-mem.c \
                 auth-pam.c
chateaud_OBJS := $(patsubst %.c,%.o,${chateaud_SRCS})

chateaud: ${chateaud_OBJS} ${libwheel}
chateaud: CFLAGS += -O0 -g

clean: clean-chateaud

clean-chateaud:
	${RM} chateaud ${chateaud_OBJS}

.PHONY: clean-chateaud

# vim:ft=make
#
