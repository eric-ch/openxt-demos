bin_PROGRAMS = condump
noinst_HEADERS = condump.h

COMMON_INCLUDES = -I../common/include

condump_SOURCES = condump.c
condump_CFLAGS = $(COMMON_INCLUDES) -W -Wall -Werror -g
condump_CPPFLAGS = $(COMMON_INCLUDES) $(LIBXC_INC)
condump_LDADD = $(LIBXC_LIB)

