#include "v4cat.h"

#define fail_on_goto(cond, rc, label)   \
    if (cond) {                         \
        rc = -errno;                    \
        goto label;                     \
    }

/*
 * Simple pipe simplex representation.
 */
struct pipe {
    struct list_head l;
    int in;     /* input fd. */
    int out;    /* output fd. */
    struct pipe *rev;   /* Optional reverse pipe (out <=> in). */
    void *owner;        /* Optional owner reference used for event/memory management. */
};

static struct pipe *pipe_init(struct pipe *p, int in, int out)
{
    INIT_LIST_HEAD(&p->l);
    p->in = in;
    p->out = out;
    p->rev = NULL;
    p->owner = NULL;
    return p;
}

static struct pipe *pipe_alloc(int in, int out)
{
    struct pipe *p;

    p = malloc(sizeof (*p));
    if (!p) {
        return NULL;
    }
    return pipe_init(p, in, out);
}

static void pipe_set_reverse(struct pipe *p, struct pipe *r)
{
    p->rev = r;
    r->rev = p;
}

static void pipe_release(struct pipe *p)
{
    if (p->in != STDIN_FILENO && p->in != STDOUT_FILENO &&
        p->in != STDERR_FILENO) {
        close(p->in);
    }
    if (p->out != STDIN_FILENO && p->out != STDOUT_FILENO &&
        p->out != STDERR_FILENO) {
        close(p->out);
    }
    free(p);
}

static inline void pipe_flush(struct list_head *pipes)
{
    struct pipe *p;

    while (!list_empty(pipes)) {
        p = list_entry(pipes->next, struct pipe, l);
        list_del(&p->l);
        pipe_release(p);
    }
}

static ssize_t pipe_splice(struct pipe *p)
{
    char buf[1024];
    ssize_t nr, nw;

    //INF("%s() pipe : { .in=%d, .out=%d }", __FUNCTION__, p->in, p->out);
    nr = read(p->in, buf, 1024);
    switch (nr) {
        case -1:
            INF("%s() read failed (%s)", __FUNCTION__, strerror(errno));
            return -errno;
        case 0:
            //INF("%s() nothing to read.", __FUNCTION__);
            return 0;
        default:
            break;
    }

    nw = write(p->out, buf, nr);
    switch (nw) {
        case -1:
            INF("%s() write failed (%s)", __FUNCTION__, strerror(errno));
            return -errno;
        case 0:
            //INF("%s() write failed (%s)", __FUNCTION__, strerror(EAGAIN));
            return -EAGAIN;
        default:
            break;
    }
    //INF("%s() spliced %d bytes from %d to %d.", __FUNCTION__, nw, p->in, p->out);
    return nw;
}

/*
 * Event handling interface.
 * XXX: So why no libevent? Because I don't want to add dependencies and only a
 *      small subset of libevent would be used here.
 */
struct event {
    struct list_head l;
    int fd;
    void *arg;
    int (*ops)(struct event *ev);
    int release;    /* Used to release events after the main event loop. */
};

static inline struct event *event_init(struct event *ev, int fd, void *arg,
                                       int (*ev_ops)(struct event *))
{
    ev->fd = fd;
    ev->arg = arg;
    ev->ops = ev_ops;
    ev->release = 0;
    INIT_LIST_HEAD(&ev->l);
    return ev;
}

static struct event *event_alloc(int fd, void *arg,
                                 int (*ev_ops)(struct event *))
{
    struct event *ev;

    ev = malloc(sizeof (*ev));
    if (!ev) {
        return NULL;
    }
    return event_init(ev, fd, arg, ev_ops);
}

static void event_add(struct event *new, struct list_head *evs)
{
    list_add_tail(&new->l, evs);
}

static void event_del(struct event *ev)
{
    list_del(&ev->l);
}

static void event_release(struct event *ev)
{
    free(ev);
}

static inline void event_flush(struct list_head *evs)
{
    struct event *e;

    while (!list_empty(evs)) {
        e = list_entry(evs->next, struct event, l);
        event_del(e);
        event_release(e);
    }
}


static int event_wait(struct list_head *evs, struct timeval *to)
{
    fd_set rfds;
    int n, rc, nfds = 0;
    struct event *ev = NULL, *tev = NULL;
    struct timeval __to = { .tv_sec = to->tv_sec, .tv_usec = to->tv_usec };

    FD_ZERO(&rfds);
    list_for_each_entry_safe(ev, tev, evs, l) {
        FD_SET(ev->fd, &rfds);
        //INF("Select on fd %d.", ev->fd);
        nfds = (nfds < ev->fd) ? ev->fd : nfds;
    }
    n = select(nfds + 1, &rfds, NULL, NULL, &__to);
    if (n <= 0) {
        return -errno;
    }
    //INF("Select returned %d fds after %us.", n, (unsigned int)__to.tv_sec);

    list_for_each_entry_safe(ev, tev, evs, l) {
        if (FD_ISSET(ev->fd, &rfds)) {
            //INF("fd %d ready.", ev->fd);
            rc = ev->ops(ev);
            if (rc <= 0) {
                //INF("event returned %d...", rc);
            }
            if (!--n) {
                //INF("no more fd to process.");
                /* No more fd to process. */
                break;
            }
        }
    }
    /* Release marked events. */
    list_for_each_entry_safe(ev, tev, evs, l) {
        if (ev->release) {
            event_del(ev);
            event_release(ev);
        }
    }

    return 0;
}

/*
 * v4v interface.
 */
#define V4V_STREAM_DEV  "/dev/v4v_stream"
static int v4v_socket_stream(void)
{
    int fd, rc;

    /* XXX: CLOEXEC should be default right? */
    fd = open(V4V_STREAM_DEV, O_RDWR /* | O_CLOEXEC */);
    if (fd < 0) {
        return -errno;
    }
    rc = fcntl(fd, F_GETFD);
    fail_on_goto(rc < 0, rc, fail);

    rc &= ~FD_CLOEXEC;
    fail_on_goto(fcntl(fd, F_SETFD, rc) == -1, rc, fail);
    return fd;

fail:
    close(fd);
    return rc;
}

static int v4v_bind(int fd, v4v_addr_t *vaddr)
{
    struct v4v_ring_id id;

    id.addr = *vaddr;
    id.partner = V4V_DOMID_NONE;

    if (ioctl(fd, V4VIOCBIND, &id) == -1) {
        return -errno;
    }
    return 0;
}

static int v4v_listen(int fd, int backlog)
{
    if (ioctl(fd, V4VIOCLISTEN, &backlog) == -1) {
        return -errno;
    }
    return 0;
}

static int v4v_accept(int fd, v4v_addr_t *peer)
{
    int fd2;

    fd2 = ioctl(fd, V4VIOCACCEPT, peer);
    if (fd2 == -1) {
        return -errno;
    }
    return fd2;
}

static int v4v_connect(int fd, v4v_addr_t *peer)
{
    if (ioctl(fd, V4VIOCCONNECT, peer) == -1) {
        return -errno;
    }
    return 0;
}

/*
 * v4v helpers.
 */
static int __v4v_socket_listen(unsigned long port)
{
    int fd, rc;
    v4v_addr_t vaddr;

    fd = v4v_socket_stream();
    if (fd < 0) {
        return fd;
    }
    memset(&vaddr, 0, sizeof (vaddr));
    vaddr.domain = V4V_DOMID_NONE;
    vaddr.port = port;
    fail_on_goto(v4v_bind(fd, &vaddr), rc, fail);
    fail_on_goto(v4v_listen(fd, 1), rc, fail);
    return fd;

fail:
    close(fd);
    return rc;
}

static int __v4v_socket_connect(domid_t domid, unsigned long port)
{
    int fd, rc;
    v4v_addr_t /*vaddr, */vpeer;

    fd = v4v_socket_stream();
    if (fd < 0) {
        return fd;
    }

#if 0
    /* Be nice and do not rely on V4V implementation. */
    vaddr.domain = 0;
    vaddr.port = V4V_PORT_NONE;
    if (v4v_bind(fd, &vaddr)) {
        rc = -errno;
        goto fail;
    }
#endif
    vpeer.domain = domid;
    vpeer.port = port;
    fail_on_goto(v4v_connect(fd, &vpeer), rc, fail);

    return fd;

fail:
    close(fd);
    return rc;
}

/*
 * Splice pipe input in its output.
 */
static int v4cat_splice(struct event *ev)
{
    struct pipe *p = ev->arg;
    int rc;

    rc = pipe_splice(p);
    if (rc <= 0) {
        /* Either failed or the other end has close() its fd. */
        if (p->rev) {
            list_del(&(p->rev->l));
            if (p->rev->owner)
                ((struct event *)(p->rev->owner))->release = 1;
            pipe_release(p->rev);
        }
        list_del(&p->l);
        pipe_release(p);
        ev->release = 1;
    }
    return rc;
}

static struct event *__pipe_event_alloc(int in, int out,
                                        int (*ev_ops)(struct event *),
                                        struct list_head *pipes)
{
    struct pipe *p;
    struct event *ev;

    p = pipe_alloc(in, out);
    if (!p) {
        return NULL;
    }
    list_add(&p->l, pipes);

    ev = event_alloc(p->in, p, ev_ops);
    if (!ev) {
        list_del(&p->l);
        pipe_release(p);
        return NULL;
    }
    p->owner = ev;

    return ev;
}

static struct event *__join_event_alloc(int sfd, int ifd, int ofd,
                                        int (*ev_ops)(struct event *),
                                        struct list_head *pipes)
{
    struct event *ev;
    struct pipe *in, *out;

    in = pipe_alloc(sfd, ofd);
    if (!in) {
        return NULL;
    }
    out = pipe_alloc(ifd, sfd);
    if (!out) {
        free(in);
        return NULL;
    }
    pipe_set_reverse(in, out);
    ev = event_alloc(sfd, in, ev_ops);
    if (!ev) {
        free(out);
        free(in);
        return NULL;
    }
    in->owner = out->owner = ev;
    list_add(&in->l, pipes);
    list_add(&out->l, pipes);

    return ev;
}

/*
 * Accept new client.
 */
static int v4cat_accept(struct event *ev)
{
    struct list_head *pipes = ev->arg;
    struct event *rev;
    int fd, rc;
    v4v_addr_t peer = { .domain = 0, .port = 0 };

    fd = v4v_accept(ev->fd, &peer);
    if (fd < 0) {
        rc = -errno;
        INF("%s() failed (%s)", __FUNCTION__, strerror(errno));
        return rc;
    }

    rev = __join_event_alloc(fd, STDIN_FILENO, STDOUT_FILENO,
                             v4cat_splice, pipes);
    if (!rev) {
        close(fd);
        return -ENOMEM;
    }
    event_add(rev, &ev->l);

    return fd;
}

/*
 * Send STDIN to all clients.
 */
static int v4cat_broadcast(struct event *ev)
{
    struct list_head *pipes = ev->arg;
    struct pipe *p, *tp;
    char buf[1024];
    int nr, nw;

    /* If there is more, select() will tell us anyway. */
    nr = read(ev->fd, buf, 1024);
    if (nr < 0) {
        return -errno;
    }

    if (list_empty(pipes)) {
        INF("No client yet.");
        /* We always return >0 to make this event persistent. */
        return 1;
    }
    p = list_entry(pipes->next, struct pipe, l);
    while (&(p->l) != pipes) {
        tp = list_entry(p->l.next, struct pipe, l);
        if (p->out == STDOUT_FILENO) {
            p = tp;
            continue;
        }
        nw = write(p->out, buf, nr);
        if (nw != nr) {
            /* The other end closed or we failed, anyway release. */
            /* Keep next pointer valid. */
            if (tp == p->rev) {
                tp = list_entry(tp->l.next, struct pipe, l);
            }
            /* Remove the v4cat_splice event recv end of the pipe. */
            ((struct event *)(p->owner))->release = 1;
            list_del(&(p->rev->l));
            list_del(&p->l);
            pipe_release(p->rev);
            pipe_release(p);
        }
        p = tp;
    }
    /* We always return >0 to make this event persistent. */
    return 1;
}

/*
 * Server side.
 */
static int v4cat_listen(unsigned long port)
{
    int rc, fd;
    struct list_head pipes;     /* List of pipes to clients (accepted ones). */
    struct list_head events;    /* List of events to be managed. */
    struct event *accept, *broadcast;
    struct timeval to = { .tv_sec = 30, .tv_usec = 0 };

    fd = __v4v_socket_listen(port);
    if (fd < 0) {
        return fd;
    }

    INIT_LIST_HEAD(&pipes);
    /* Read from STDIN only and broadcast to every client. */
    accept = event_alloc(STDIN_FILENO, &pipes, v4cat_broadcast);
    //event_init(&broadcast, STDIN_FILENO, &pipes, v4cat_broadcast);
    /* Accept inbound connection requests. */
    broadcast = event_alloc(fd, &pipes, v4cat_accept);
    //event_init(&accept, fd, &pipes, v4cat_accept);

    INIT_LIST_HEAD(&events);
    event_add(broadcast, &events);
    event_add(accept, &events);

    do {
        rc = event_wait(&events, &to);
    } while (!rc);

    /* Cleanup. */
    pipe_flush(&pipes);
    event_flush(&events);
    close(fd);

    return rc;
}

/*
 * Client side.
 */
static int v4cat_connect(domid_t domid, unsigned long port)
{
    int rc, fd;
    struct list_head events, pipes;
    struct event *in, *out;
    struct timeval to = { .tv_sec = 30, .tv_usec = 0 };

    fd = __v4v_socket_connect(domid, port);
    if (fd < 0) {
        return -errno;
    }

    INIT_LIST_HEAD(&pipes);
    in = __pipe_event_alloc(fd, STDOUT_FILENO, v4cat_splice, &pipes);
    out = __pipe_event_alloc(STDIN_FILENO, fd, v4cat_splice, &pipes);
    pipe_set_reverse(in->arg, out->arg);

    INIT_LIST_HEAD(&events);
    event_add(in, &events);
    event_add(out, &events);

    do {
        rc = event_wait(&events, &to);
    } while (!rc && !list_empty(&events));

    /* Cleanup. */
    pipe_flush(&pipes);
    event_flush(&events);
    close(fd);

    return rc;
}

/*
 * Display usage.
 */
static int usage(int rc)
{
    INF("Basic usages:");
    INF("v4cat [options] domid port");
    INF("v4cat -l -p local_port");
    INF("Options:");
    INF("	-l, --listen	listen mode, for inbound connects.");
    INF("	-p, --port	local port number");

    return rc;
}

/*
 * Supported options, assumes there is always a short format for every long
 * one.
 */
#define OPT_STR "hlp:"
static struct option long_options[] = {
    { "listen",   no_argument,          0,  'l' },
    { "port",     required_argument,    0,  'p' },
    { "help",     no_argument,          0,  'h' },
    { 0,            0,                  0,  0 },
};

static inline int is_valid_port(unsigned long port)
{
    return (port > 0) && (port < 65535);
}

int main(int argc, char *argv[])
{
    int rc = 0;
    unsigned long local_port = 0;
    unsigned long port = 0;
    int listen = 0;
    domid_t domid = V4V_DOMID_NONE;

    if (argc < 1) {
        return usage(EINVAL);
    }

    do {
        int opt, longindex;

        opt = getopt_long(argc, argv, OPT_STR, long_options, &longindex);
        switch (opt) {
            case -1:
                //INF("getopt_long() is done!");
                goto getopt_done;
            case 0:
                WAR("Malformated option \"%s\", please fix the code.",
                    long_options[longindex].name);
                continue;

            case 'h':
                //INF("-h|--help");
                return usage(0);

            case 'l':
                //INF("-l|--listen");
                listen = 1;
                continue;
            case 'p':
                //INF("-p|--local-port");
                rc = parse_ul(optarg, &local_port);
                if (rc || !is_valid_port(local_port)) {
                    ERR("Invalid local port %lu.", local_port);
                    return -rc;
                }
                continue;

            default:
                ERR("Unknown option '%c'.", opt);
                return usage(EINVAL);
        }

    } while (1);

getopt_done:
    while (optind < argc) {
        if (domid == V4V_DOMID_NONE) {
            parse_domid(argv[optind++], &domid);
            //INF("domid=%u", domid);
        } else if (!port) {
            rc = parse_ul(argv[optind++], &port);
            if (rc || !is_valid_port(port)) {
                ERR("Invalid port %s.", argv[optind - 1]);
                return -rc;
            }
            //INF("port=%lu", port);
        } else {
            INF("%s not handled...", argv[optind++]);
        }
    }

    /*
     * Sanity checks.
     */
    if (!listen && (domid == V4V_DOMID_NONE)) {
        ERR("Missing domid.");
        return EINVAL;
    }
    if (listen && !local_port) {
        ERR("Missing local port.");
        return EINVAL;
    }
    if (!listen && !port) {
        ERR("Missing port.");
        return EINVAL;
    }

    if (listen) {
        INF("Open listening socket on <any>:%lu.", local_port);
        rc = v4cat_listen(local_port);
        if (rc) {
            ERR("Error:%s", strerror(-rc));
        }
    } else {
        INF("Open socket to dom%u:%lu.", domid, port);
        rc = v4cat_connect(domid, port);
        if (rc) {
            ERR("Error: %s", strerror(-rc));
        }
    }

    return -rc;
}

