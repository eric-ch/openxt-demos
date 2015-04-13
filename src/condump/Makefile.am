COMMON_INC = -I../common/include
COMMON_INCLUDES = ../common/include/utils.h

bin_PROGRAMS = condump

condump_SOURCES = condump.c condump.h $(COMMON_INCLUDES)
condump_CFLAGS = $(COMMON_INC) -W -Wall -Werror -g
condump_CPPFLAGS = $(COMMON_INC) $(LIBXC_INC)
condump_LDADD = $(LIBXC_LIB)

