#ifndef _UTILS_H_
# define _UTILS_H_

/*
 * GCC warning handling.
 */
# define unused(var)	(void) (var);

/*
 * Logging facilities.
 */
# include <stdio.h>

# ifndef M_TAG
#  define M_TAG ""
# endif
# define INF(fmt, ...) fprintf(stdout, M_TAG "%s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
# define WAR(fmt, ...) fprintf(stderr, "Warning:%s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
# define ERR(fmt, ...) fprintf(stderr, "Error:%s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

/*
 * Memory handling helpers.
 */
# include <stdlib.h>

static inline void *m_malloc(size_t size)
{
    void *b;

    b = malloc(size);
    if (!b) {
        perror("malloc");
        abort();
    }
    return b;
}

static inline void *m_realloc(void *ptr, size_t size)
{
    void *b;

    b = realloc(ptr, size);
    if (!b) {
        perror("realloc");
        abort();
    }
    return b;
}

static inline void *m_calloc(size_t nmemb, size_t size)
{
    void *b;

    b = calloc(nmemb, size);
    if (!b) {
        perror("calloc");
        abort();
    }
    return b;
}

static inline void *m_malloc0(size_t size)
{
    return m_calloc(1, size);
}

/*
 * Arithmetic helpers.
 */
#define KB(n)   ((n) * 1024)
#define MB(n)   (KB(n) * 1024)

#define ARRAY_LEN(arr)  (sizeof (arr) / sizeof ((arr)[0]))

/*
 * Error handling helpers.
 */
#include <errno.h>

#define test_or_failret(cond, ret, fmt, ...)    \
    if (cond) {                                 \
        ERR(fmt, ##__VA_ARGS__);                \
        return (ret);                           \
    }                                           \

#define test_or_failerrno(cond, fmt, ...)       \
    if (cond) {                                 \
        int __rc = -errno;                      \
        ERR(fmt, ##__VA_ARGS__);                \
        return (-__rc);                         \
    }

/*
 * Input parsing helpers.
 */
# include <limits.h>

static inline int parse_ul(const char *nptr, unsigned long *ul)
{
    char *end;

    *ul = strtoul(nptr, &end, 0);
    if (*ul == ULONG_MAX) {
        return -ERANGE;
    }
    if (end == nptr) {
        return -EINVAL;
    }
    return 0;
}

static inline int parse_ull(const char *nptr, unsigned long long *ull)
{
    char *end;

    *ull = strtoull(nptr, &end, 0);
    if (*ull == ULLONG_MAX) {
        return -ERANGE;
    }
    if (end == nptr) {
        return -EINVAL;
    }
    return 0;
}

#endif /* !_UTILS_H_ */

