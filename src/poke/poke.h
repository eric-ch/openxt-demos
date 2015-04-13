#ifndef _POKE_H_
# define _POKE_H_

# include "config.h"

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

# ifdef HAVE_SYS_IO_H
#  include <sys/io.h>
# endif

# ifdef HAVE_SYS_MMAN_H
#  include <sys/mman.h>
# endif

# ifdef HAVE_SYS_STAT_H
#  include <sys/stat.h>
# endif

# ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
# endif

# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# endif

# ifdef HAVE_FCNTL_H
#  include <fcntl.h>
# endif

# ifdef HAVE_XENCTRL_H
#  include <xenctrl.h>
# endif

# include "utils.h"
# include "pci.h"

#endif /* !_POKE_H_ */

