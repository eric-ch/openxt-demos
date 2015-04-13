#include "poke.h"

struct poke_op {
    const char *cmd_str;
    const char *usage;
    const char *description;
    int (*cmd)(int argc, char *argv[]);
};

enum poke_cmd {
    POKE_HELP = 0,
    POKE_INJECT_MSI,
    POKE_IOPORT_READ,
    POKE_IOPORT_WRITE,
    POKE_MMIO_READ,
    POKE_MMIO_WRITE,
};

static int usage(int argc, char *argv[]);
static int poke_inject_msi(int argc, char *argv[]);
static int poke_io_port_read(int argc, char *argv[]);
static int poke_io_port_write(int argc, char *argv[]);
static int poke_mmio_read(int argc, char *argv[]);
static int poke_mmio_write(int argc, char *argv[]);

static const struct poke_op poke_ops[] = {
    [POKE_HELP] = { "help", "", "Display usage.", &usage },
    [POKE_INJECT_MSI] = { "msi", "<domid> <address> <data>",
                          "Inject an MSI in domain <domid>, using <data> at <address>.",
                          &poke_inject_msi },
    [POKE_IOPORT_READ] = { "io-read", "<address> <b|w|l>",
                           "Read 1|2|4 bytes from IO port at <address>." ,
                           &poke_io_port_read },
    [POKE_IOPORT_WRITE] = { "io-write", "<address> <b|w|l> <value>",
                            "Write 1|2|4 bytes <value> to IO port at <address>.",
                            &poke_io_port_write },
    [POKE_MMIO_READ] = { "mmio-read", "<pci-bdf> <BAR-id> <register-address> <b|w|l>",
                         "Read 1|2|4 bytes from device <pci-bdf> <BAR-id> at <register-address>.",
                         &poke_mmio_read },
    [POKE_MMIO_WRITE] = { "mmio-write", "<pci-bdf> <BAR-id> <register-address> <b|w|l> <value>",
                          "Write 1|2|4 bytes from <value> to device <pci-bdf> <BAR-id> at <register-address>.",
                          &poke_mmio_write },
};

/*
 * Display usage.
 */
static int usage(int argc, char *argv[])
{
    unused(argc);
    unused(argv);
    size_t i;

    for (i = 0; i < ARRAY_LEN(poke_ops); ++i) {
        INF("%s:	%s", poke_ops[i].cmd_str, poke_ops[i].usage);
        INF("		%s", poke_ops[i].description);
    }
    return 0;
}

/*
 * Inject an MSI using libxc.
 */
static int poke_inject_msi(int argc, char *argv[])
{
    unsigned long domid;
    unsigned long long address;
    unsigned long data;
    xc_interface *xch;
    int rc = 0;

    test_or_failret(argc != 3, -EINVAL, "msi: Invalid parameters for `msi' command.");

    rc = parse_ul(argv[0], &domid);
    test_or_failret(rc < 0, rc, "msi: Could not parse domid `%s' (%s).", argv[0], strerror(-rc));
    rc = parse_ull(argv[1], &address);
    test_or_failret(rc < 0, rc, "msi: Could not parse address `%s' (%s).", argv[1], strerror(-rc));
    rc = parse_ul(argv[2], &data);
    test_or_failret(rc < 0, rc, "msi: Could not parse data `%s' (%s).", argv[2], strerror(-rc));

    xch = xc_interface_open(NULL, NULL, 0);
    test_or_failerrno(rc < 0, "msi: Could not get xencrtl handle (%s).", strerror(errno));

    /* 0 | -1 as hypercall are handler through privcmd doing ioctl(). */
    rc = xc_hvm_inject_msi(xch, domid, address, data);
    if (rc) {
        rc = -errno;
        ERR("msi: Could not inject MSI @%#llx %#lx (%s).", address, data, strerror(-rc));
        xc_interface_close(xch);
    }
    INF("xc_hvm_inject_msi(xch, %lu, %#llx, %#lx);", domid, address, data);

    return rc;
}

/*
 * Read an IO port.
 */
static int poke_io_port_read(int argc, char *argv[])
{
    unsigned long addr;
    unsigned long val;
    int rc;

    test_or_failret(argc != 2, -EINVAL, "io-read: Invalid parameters for `io-read' command.");

    rc = parse_ul(argv[0], &addr);
    INF("%s", __FUNCTION__);
    test_or_failret(rc < 0, rc, "io-read: Could not parse IO port address `%s' (%s).", argv[0], strerror(-rc));

    switch (argv[1][0]) {
        case 'b':
            rc = ioperm(addr, 8, 1);
            test_or_failret(rc < 0, rc, "io-read: Could not access IO port address `%s.%c' (%s).", argv[0], argv[1][0], strerror(-rc));
            val = inb(addr);
            ioperm(addr, 8, 0);
            break;
        case 'w':
            rc = ioperm(addr, 16, 1);
            test_or_failret(rc < 0, rc, "io-read: Could not access IO port address `%s.%c' (%s).", argv[0], argv[1][0], strerror(-rc));
            val = inw(addr);
            ioperm(addr, 16, 0);
            break;
        case 'l':
            rc = ioperm(addr, 32, 1);
            test_or_failret(rc < 0, rc, "io-read: Could not access IO port address `%s.%c' (%s).", argv[0], argv[1][0], strerror(-rc));
            val = inl(addr);
            ioperm(addr, 32, 0);
            break;
        default:
            ERR("io-read: Could not parse IO port size `%s' (%s).", argv[1], strerror(EINVAL));
            return -EINVAL;
    }
    INF("io-read: in%c(%#lx) -> %#lx", argv[1][0], addr, val);
    return 0;
}

/*
 * Write an IO port.
 */
static int poke_io_port_write(int argc, char *argv[])
{
    unsigned long addr;
    unsigned long val;
    int rc;

    test_or_failret(argc != 3, -EINVAL, "io-write: Invalid parameters for `io-read' command.");

    rc = parse_ul(argv[0], &addr);
    test_or_failret(rc < 0, rc, "io-write: Could not parse IO port address `%s' (%s).", argv[0], strerror(-rc));
    rc = parse_ul(argv[2], &val);
    test_or_failret(rc < 0, rc, "io-write: Could not parse value `%s' (%s).", argv[2], strerror(-rc));
    switch (argv[1][0]) {
        case 'b':
            rc = ioperm(addr, 8, 1);
            test_or_failret(rc < 0, rc, "io-write: Could not access IO port address `%s.%c' (%s).", argv[0], argv[1][0], strerror(-rc));
            outb(val, addr);
            ioperm(addr, 8, 0);
            break;
        case 'w':
            rc = ioperm(addr, 16, 1);
            test_or_failret(rc < 0, rc, "io-write: Could not access IO port address `%s.%c' (%s).", argv[0], argv[1][0], strerror(-rc));
            outw(val, addr);
            ioperm(addr, 8, 0);
            break;
        case 'l':
            rc = ioperm(addr, 32, 1);
            test_or_failret(rc < 0, rc, "io-write: Could not access IO port address `%s.%c' (%s).", argv[0], argv[1][0], strerror(-rc));
            outl(val, addr); break;
            ioperm(addr, 32, 0);
            break;
        default:
            ERR("io-write: Could not parse IO port size `%s' (%s).", argv[1], strerror(EINVAL));
            return -EINVAL;
    }
    INF("io-write: out%c(%#lx, %#lx)", argv[1][0], val, addr);
    return 0;
}

/*
 * Map and read an MMIO region.
 */
static int poke_mmio_read(int argc, char *argv[])
{
    struct pci_bdf bdf;
    unsigned long bar;
    size_t bar_size;
    unsigned long long reg_addr;
    struct pci_handle *h;
    int rc = 0;

    test_or_failret(argc != 4, -EINVAL, "mmio-read: Invalid parameters for `mmio-read' command.");

    rc = parse_bdf(argv[0], &bdf);
    test_or_failret(rc < 0, rc, "mmio-read: Could not parse PCI BDF `%s' (%s).", argv[0], strerror(-rc));
    rc = parse_ul(argv[1], &bar);
    test_or_failret(rc < 0, rc, "mmio-read: Could not parse PCI BAR `%s' (%s).", argv[1], strerror(-rc));
    rc = parse_ull(argv[2], &reg_addr);
    test_or_failret(rc < 0, rc, "mmio-read: Could not parse register address `%s' (%s).", argv[2], strerror(-rc));

    bar_size = pci_bar_size(&bdf, bar);
    test_or_failret(bar_size == 0, -EIO, "mmio-read: Could not determine BAR%s size.", argv[1]);

    INF("BITE: %02u:%02x.%1x BAR%lu (size %zuK) %#llx.%c", bdf.bus, bdf.slot, bdf.func,
        bar, bar_size / 1024, reg_addr, argv[3][0]);

    h = pci_open_bar(&bdf, bar, bar_size);
    test_or_failret(!h, -ENOTTY, "mmio-read: Could not map %s BAR%s.", argv[0], argv[1]);

    switch (argv[3][0]) {
        case 'b':
            INF("mmio-read: %02u:%02x.%1x BAR%lu %#llx.%c -> %#x", bdf.bus, bdf.slot, bdf.func, bar,
                reg_addr, argv[3][0], ((uint8_t*)(h->p))[reg_addr]);
            break;
        case 'w':
            INF("mmio-read: %02u:%02x.%1x BAR%lu %#llx.%c -> %#x", bdf.bus, bdf.slot, bdf.func, bar,
                reg_addr, argv[3][0], *((uint16_t*)(&(((uint8_t*)(h->p))[reg_addr]))));
            break;
        case 'l':
            INF("mmio-read: %02u:%02x.%1x BAR%lu %#llx.%c -> %#x", bdf.bus, bdf.slot, bdf.func, bar,
                reg_addr, argv[3][0], *((uint32_t*)(&(((uint8_t*)(h->p))[reg_addr]))));
            break;
        default:
            ERR("mmio-read: Could not parse register size `%s' (%s).", argv[3], strerror(EINVAL));
            rc = -EINVAL;
    }

    pci_close_handle(h);
    return rc;
}


/*
 * Map and write an MMIO region.
 */
static int poke_mmio_write(int argc, char *argv[])
{
    struct pci_bdf bdf;
    unsigned long bar;
    unsigned long long reg_addr;
    unsigned long reg_data;
    struct pci_handle *h;
    int rc = 0;

    test_or_failret(argc != 5, -EINVAL, "mmio-write: Invalid parameters for `mmio-write' command.");

    rc = parse_bdf(argv[0], &bdf);
    test_or_failret(rc < 0, rc, "mmio-write: Could not parse PCI BDF `%s' (%s).", argv[0], strerror(-rc));
    rc = parse_ul(argv[1], &bar);
    test_or_failret(rc < 0, rc, "mmio-write: Could not parse PCI BAR `%s' (%s).", argv[1], strerror(-rc));
    rc = parse_ull(argv[2], &reg_addr);
    test_or_failret(rc < 0, rc, "mmio-write: Could not parse register address `%s' (%s).", argv[2], strerror(-rc));
    rc = parse_ul(argv[4], &reg_data);
    test_or_failret(rc < 0, rc, "mmio-write: Could not parse register data `%s' (%s).", argv[4], strerror(-rc));

    h = pci_open_bar(&bdf, bar, 4096);
    test_or_failret(!h, -ENOTTY, "mmio-write: Could not map %s BAR%s.", argv[0], argv[1]);

    switch (argv[3][0]) {
        case 'b':
            ((uint8_t*)(h->p))[reg_addr] = reg_data & 0xff;
            break;
        case 'w':
            *((uint16_t*)&((uint8_t*)(h->p))[reg_addr]) = reg_data & 0xffff;
            break;
        case 'l':
            *((uint32_t*)&((uint8_t*)(h->p))[reg_addr]) = reg_data & 0xffffffff;
            break;
        default:
            ERR("mmio-write: Could not parse register size `%s' (%s).", argv[3], strerror(EINVAL));
            rc = -EINVAL;
    }
    INF("mmio-write: %02u:%02x.%1x BAR%lu %#llx.%c <- %#lx.", bdf.bus, bdf.slot, bdf.func, bar,
        reg_addr, argv[3][0], reg_data);

    pci_close_handle(h);
    return rc;
}


int main(int argc, char *argv[])
{
    size_t i;
    int rc = 0;

    if (argc <= 1) {
        return usage(argc, argv);
    }

    for (i = 0; i < ARRAY_LEN(poke_ops); ++i) {
        if (!strcmp(poke_ops[i].cmd_str, argv[1])) {
            INF("found command %s.", argv[1]);
            rc = poke_ops[i].cmd(argc - 2, &argv[2]);
        }
    }

    return rc;
}

