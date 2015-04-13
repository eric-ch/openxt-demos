#ifndef _PCI_H_
# define _PCI_H_

struct pci_bdf {
    unsigned int bus;
    unsigned int slot;
    unsigned int func;
};

struct pci_handle {
    int fd;
    void *p;
    size_t len;
};

/*
 * Input parsing helpers.
 */
/* bdfptr expected to have an /lspci/ like format (%02u:%02x.%1x). */
int parse_bdf(const char *bdfptr, struct pci_bdf *bdf);

/*
 * PCI resources helpers.
 */
size_t pci_bar_size(const struct pci_bdf *bdf, int bar);

struct pci_handle *pci_open_bar(const struct pci_bdf *bdf, unsigned int bar, size_t len);
void pci_close_handle(struct pci_handle *h);

#endif /* !_PCI_H_ */

