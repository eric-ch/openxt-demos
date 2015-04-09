AC_INIT([openxt-demos], [0.1])
AM_INIT_AUTOMAKE

# Check for programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_CHECK_PROG(MD5SUM, md5sum, md5sum)
AC_CHECK_PROG(GREP, grep, grep)

# Check for headers
AC_CHECK_HEADERS([stdio.h stdlib.h string.h errno.h assert.h])
AC_HEADER_TIME
AC_HEADER_ASSERT

# Check for specific structures/declarations
AC_STRUCT_TM

# Check compiler characteristics.
AC_C_INLINE
AC_C_CONST
AC_SYS_LARGEFILE

# Output files
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([src/config.h])

## This one doesn't have pkg-config support...
AC_CHECK_HEADERS([xenctrl.h], [LIBXC_INC=""; LIBXC_LIB="-lxenctrl"], [AC_MSG_ERROR(["libxc required."])])
AC_SUBST(LIBXC_INC)
AC_SUBST(LIBXC_LIB)

AC_CONFIG_FILES([Makefile 
	         src/condump/Makefile ])
AC_OUTPUT
