bin_PROGRAMS = poke
noinst_HEADERS = poke.h

COMMON_INCLUDES = -I../common/include

poke_SOURCES = poke.c
poke_CFLAGS = $(COMMON_INCLUDES) -W -Wall -Werror -g
poke_CPPFLAGS = $(COMMON_INCLUDES) $(LIBXC_INC)
poke_LDADD = $(LIBXC_LIB)

