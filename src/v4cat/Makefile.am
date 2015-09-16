COMMON_INC = -I../common/include
##COMMON_LIB = -lpci

COMMON_INCLUDES = ../common/include/utils.h ../common/include/pci.h

bin_PROGRAMS = v4cat

v4cat_SOURCES = v4cat.c v4v.h $(COMMON_INCLUDES)
v4cat_CFLAGS = $(COMMON_INC) -W -Wall -Werror -g
v4cat_CPPFLAGS = $(COMMON_INC) $(LIBXC_INC)
#v4v_LDFLAGS =  -L../common/lib/pci
v4cat_LDADD = $(COMMON_LIB) $(LIBXC_LIB)

