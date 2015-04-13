COMMON_INC = -I../common/include
COMMON_LIB = -lpci

COMMON_INCLUDES = ../common/include/utils.h ../common/include/pci.h

bin_PROGRAMS = poke

poke_SOURCES = poke.c poke.h $(COMMON_INCLUDES)
poke_CFLAGS = $(COMMON_INC) -W -Wall -Werror -g
poke_CPPFLAGS = $(COMMON_INC) $(LIBXC_INC)
poke_LDFLAGS =  -L../common/lib/pci
poke_LDADD = $(COMMON_LIB) $(LIBXC_LIB)

