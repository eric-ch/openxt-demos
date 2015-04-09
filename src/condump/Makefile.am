bin_PROGRAMS = condump
noinst_HEADERS =

AM_CPPFLAGS = -I$(top_srcdir)/common/include

condump_SOURCES = condump.c
condump_CFLAGS = -W -Wall -Werror -g
condump_CPPFLAGS = ${LIBXC_INC}
condump_LDADD = ${LIBXC_LIB}

