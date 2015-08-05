#
# Makefile
# Adrian Perez, 2015-08-04 01:05
#

all: hichatd

libwheel_PATH := wheel
include wheel/Makefile.libwheel

hichatd_SRCS := hichatd.c proto-hichat.c \
                proto-xmpp.c \
                proto-irc.c proto-irc-parse.c
hichatd_OBJS := $(patsubst %.c,%.o,${hichatd_SRCS})

hichatd: ${hichatd_OBJS} ${libwheel}
hichatd: CFLAGS += -O0 -g

clean: clean-hichatd

clean-hichatd:
	${RM} hichatd ${hichatd_OBJS}

.PHONY: clean-hichatd

# vim:ft=make
#
