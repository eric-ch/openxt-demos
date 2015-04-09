#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <xenctrl.h>

int read_console(xc_interface *xch, unsigned int ring_size)
{
    char *buf;
    unsigned int buf_size = ring_size;
    unsigned int index = 0; /* See xc_readconsolering() */
    unsigned int n;         /* Character to be read or read when returned by xc_readconsolerin). */
    int rc = 0;

    buf = malloc(buf_size);
    if (!buf) {
        ERR("malloc(): failed (%s).", strerror(errno));
        abort();
    }
    INF("alloc'ed %dB buffer.", buf_size);

    n = ring_size;
    rc = xc_readconsolering(xch, buf, &n, 0, 1, &index);
    if (rc < 0) {
        rc = -errno;
        ERR("xc_readconsolering(): failed (%s).", strerror(errno));
        free(buf);
        return rc;
    }
    INF("read %dB from console ring, index @%d.", n, index);

    while ((n == ring_size) && (rc >= 0)) {
        buf_size += ring_size;
        if (buf_size < ring_size) {  /* Test overflow. */
            INF("buffer size overflowed, leaving.");
            errno = ERANGE;
            break;
        }
        buf = realloc(buf, buf_size);
        if (!buf) {
            ERR("realloc(): failed (%s).", strerror(errno));
            abort();
        }
        INF("realloc'ed %dB buffer.", buf_size);
        rc = xc_readconsolering(xch, &buf[index], &n, 0, 1, &index);
        INF("read %dB buffer, index@%d, buf@%d", n, index, buf_size - ring_size);
    }
    if ((rc < 0) || (buf_size < ring_size)) {
        rc = -errno;
        ERR("xc_readconsolering(): failed (%s).", strerror(errno));
        free(buf);
        return rc;
    }
    buf[buf_size - ring_size + n] = '\0';
    fprintf(stdout, "%s\n", buf);
    free(buf);
    return rc;
}

#define KB(n) ((n) * 1024)
int main(void)
{
    xc_interface *xch;
    int rc = 0;

    xch = xc_interface_open(NULL, NULL, 0);
    if (!xch) {
        ERR("xc_interface_open failed (%s)", strerror(errno));
        return 1;
    }

    rc = read_console(xch, KB(32));
    if (rc) {
        ERR("failed to read console ring (%s).", strerror(errno));
    }

    xc_interface_close(xch);
    return -rc;
}

