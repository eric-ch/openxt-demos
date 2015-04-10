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
};

static int usage(int argc, char *argv[]);
static int poke_inject_msi(int argc, char *argv[]);

static const struct poke_op poke_ops[] = {
    [POKE_HELP] = { "help", "", "Display usage.", &usage },
    [POKE_INJECT_MSI] = { "msi", "<domid> <address> <data>",
                          "Inject an MSI in domain <domid>, using <data> at <address>.",
                          &poke_inject_msi },
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
            rc = poke_ops[i].cmd(argc - 2, &argv[2]);
        }
    }

    return rc;
}
