#ifndef _V4CAT_H_
# define _V4CAT_H_

# include "config.h"

# define _GNU_SOURCE

# ifdef HAVE_ASSERT_H
#  include <assert.h>
# endif

# ifdef HAVE_ERRNO_H
#  include <errno.h>
# endif

# ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
# endif

# ifdef HAVE_MEMORY_H
#  include <memory.h>
# endif

# ifdef HAVE_STDINT_H
#  include <stdint.h>
# endif

# ifdef HAVE_STDIO_H
#  include <stdio.h>
# endif

# ifdef HAVE_STDLIB_H
#  include <stdlib.h>
# endif

# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif

# ifdef HAVE_STRING_H
#  include <string.h>
# endif

# ifdef HAVE_LIMITS_H
#  include <limits.h>
# endif

/*
# ifdef HAVE_SYS_IO_H
#  include <sys/io.h>
# endif
*/

# ifdef HAVE_SYS_MMAN_H
#  include <sys/mman.h>
# endif

# ifdef HAVE_SYS_STAT_H
#  include <sys/stat.h>
# endif

# ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
# endif

# ifdef HAVE_SYS_IOCTL_H
#  include <sys/ioctl.h>
# endif

# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# endif

# ifdef HAVE_FCNTL_H
#  include <fcntl.h>
# endif

/*
# ifdef HAVE_XENCTRL_H
#  include <xenctrl.h>
# endif
*/
# ifdef HAVE_GETOPT_H
#  include <getopt.h>
# endif

# ifdef HAVE_XEN_V4V_H
#  include <xen/v4v.h>
# else
#  error "xen/v4v.h is missing."
# endif

# ifdef HAVE_LINUX_V4V_DEV_H
#  include <linux/v4v_dev.h>
# else
#  error "linux/v4v_dev.h is missing."
# endif

# include "list.h"

static inline int parse_domid(const char *nptr, domid_t *domid)
{
    unsigned long d;
    char *end;

    d = strtoul(nptr, &end, 0);
    if (end == nptr) {
        return -EINVAL;
    }
    if (d == ULONG_MAX) {
        return -ERANGE;
    }
    if (d >= V4V_DOMID_INVALID) {
        return -EINVAL;
    }
    *domid = (domid_t)d;
    return 0;
}

# define M_TAG "v4cat: "
# include "utils.h"

#endif /* _V4CAT_H_ */

