#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/mman.h>

#include <pci.h>

/* bdfptr expected to have an /lspci/ like format (%02u:%02x.%1x). */
int parse_bdf(const char *bdfptr, struct pci_bdf *bdf)
{
    unsigned long values[3];
    unsigned int bases[3] = { 10, 16, 16 };
    unsigned int offsets[3] = { 0, 3, 6 };
    unsigned int i;

    for (i = 0; i < 3; ++i) {
        char *end;

        values[i] = strtoul(bdfptr + offsets[i], &end, bases[i]);
        if (values[i] == ULONG_MAX) {
            return -ERANGE;
        }
        if (end == (bdfptr + offsets[i])) {
            return -EINVAL;
        }
    }
    if (values[0] & ~0xff || values[1] & ~0x1f || values[2] & ~0x7) {
        return -EINVAL;
    }
    bdf->bus = values[0];
    bdf->slot = values[1];
    bdf->func = values[2];
    return 0;
}

/*
 * PCI resources helpers.
 */
#define RES_LINE_LEN    57
size_t pci_bar_size(const struct pci_bdf *bdf, int bar)
{
    char syspath[256];
    char buf[RES_LINE_LEN];
    int fd, rc;
    size_t size = 0;
    unsigned long addr, mask, flags;

    sprintf(syspath, "/sys/class/pci_bus/%04u:%02u/device/%04u:%02u:%02x.%1x/resource",
            0 /* domain */, bdf->bus, 0 /* domain */, bdf->bus, bdf->slot, bdf->func);
    fd = open(syspath, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 0;
    }
    rc = lseek(fd, RES_LINE_LEN * bar, SEEK_SET);
    if (rc != RES_LINE_LEN * bar) {
        perror("lseek");
        goto out;
    }
    rc = read(fd, buf, sizeof (buf));
    if (rc != sizeof (buf)) {
        perror("read");
        goto out;
    }
    buf[sizeof (buf) - 1] = '\0';
    rc = sscanf(buf, "%lx %lx %lx", &addr, &mask, &flags);
    if (rc != 3) {
        perror("sscanf");
        goto out;
    }
    size = (~addr & mask) + 1;
out:
    close(fd);
    return size;
}

struct pci_handle *pci_open_bar(const struct pci_bdf *bdf, unsigned int bar, size_t len)
{
    struct pci_handle *h;
    char syspath[256];
    int fd;
    void *p;

    sprintf(syspath, "/sys/class/pci_bus/%04u:%02u/device/%04u:%02u:%02x.%1x/resource%u",
            0 /* domain */, bdf->bus, 0 /* domain */, bdf->bus, bdf->slot, bdf->func, bar);
    fd = open(syspath, O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open");
        return NULL;
    }
    p = mmap(0, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (p == MAP_FAILED || !p) {
        perror("mmap");
        close(fd);
        return NULL;
    }

    h = malloc(sizeof (*h));
    if (!h) {
        close(fd);
        return NULL;
    }
    h->fd = fd;
    h->len = len;
    h->p = p;
    return h;
}

void pci_close_handle(struct pci_handle *h)
{
    if (munmap(h->p, h->len)) {
        perror("munmap");
    }
    if (close(h->fd)) {
        perror("close");
    }
    free(h);
}

