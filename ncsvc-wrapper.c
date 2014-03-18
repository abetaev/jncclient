#define _GNU_SOURCE

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <string.h>
#include <signal.h>
#include <poll.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <linux/in_route.h>
#include <linux/sockios.h>
#include <linux/socket.h>
#include <libnetlink.h>

#include "ll_map.h"

void *wrap(char *call);

static int (* open_next)(const char *, int);
static int (* chown_next)(const char *, uid_t, gid_t);
static int (* mkdir_next)(const char *, mode_t);
static ssize_t (* write_next)(int fd, const void *buf, size_t count);
static int (* __xstat_next)(int ver, const char *, struct stat *);
static int (* __fxstat_next)(int ver, int, struct stat *);
static int (*ioctl_next)(int, unsigned long, void*);
static int (* __libc_start_main_next)(int (*main)(int, char **, char **), int argc, char ** ubp_av, void (*init) (void), void (*fini) (void), void (*rtld_fini) (void), void (* stack_end));

int (* main_orig)(int, char **, char **);

static const char * original_log_file_name;

static int log_fd = 0xFFFFFF;

static struct rtnl_handle rth;

void init() __attribute__((constructor));
void fini() __attribute__((destructor));

static const char * routes_file = "/tmp/jncclient_proc_net_route";

int open(const char *pathname, int flags, ...) {
    if (!strcmp(pathname, "/proc/net/route")) {
        return open_next(routes_file, flags);
    }
    if (!strcmp(pathname, original_log_file_name)) {
        // open for log file returns fake descriptor for further handling
        return (log_fd = open("/tmp/ncsvc.log", flags));
        return log_fd;
    }
    return open_next(pathname, flags);
}

int chown(const char *path, uid_t owner, gid_t group) {
    if (strstr(original_log_file_name, path) != NULL) {
        // chown for log file only returns success
        return 0;
    }
    return chown_next(path, owner, group);
}

int __xstat(int ver, const char *pathname, struct stat *buf) {
    if (!strcmp(original_log_file_name, pathname)) {
        // stat for log file will only returns success
        return 0;
    }
    return __xstat_next(ver, pathname, buf);
}

int __fxstat(int ver, int fd, struct stat *buf) {
    if (fd == log_fd) {
        // stat for out fake descriptor will always return success
        return 0;
    }
    return __fxstat_next(ver, fd, buf);
}

int mkdir(const char *pathname, mode_t mode) {
    // we don't want ncsvc to create any directories
    return 0;
}

ssize_t write(int fd, const void *buf, size_t count) {
//    if (fd == log_fd || fd == 0 || fd == 1) {
        // redirect log output to stdout
        // TODO add options to use sysctl and stderr
//        return write_next(0, buf, count);
//    }
    return write_next(fd, buf, count);
}

struct rta_addr {
	__u32 addr[8];
	int len;
};

int sockaddr2int(struct sockaddr * sa) {
    return ((struct sockaddr_in *) sa)->sin_addr.s_addr;
}

void sockaddr2rta(struct rta_addr * rta, struct sockaddr * sa) {
	struct sockaddr_in * sa_in = (struct sockaddr_in *) sa;
	rta->addr[0] = sa_in->sin_addr.s_addr;
	rta->len = 4;
}

__u8 sockaddr2len(struct sockaddr * sa) {
    struct sockaddr_in * sa_in = (struct sockaddr_in *) sa;
    __u32 addr = sa_in->sin_addr.s_addr;
    __u32 i = 1;
    __u8 c = 0;
    int maxlen = (sizeof(addr) * 8);
    while ((addr & i) && c < maxlen) {
        c ++;
        i <<= 1;
    }
    return c;
}

struct rtentry rtentries[100];
int rtentries_number = 0;

static void append_route(struct rtentry *rte) {
    FILE *f = fopen(routes_file, "a");
    if (f == NULL) {
        puts("Unable to open routes file");
        exit(1);
    }
    char buf[1024];
    sprintf(buf, "%s\t%0.8X\t%0.8X\t%0.4X\t0\t0\t%i\t%0.8X\t0\t0\t0\n", 
            rte->rt_dev == NULL ? "" : rte->rt_dev, 
            sockaddr2int(&(rte->rt_dst)), 
            sockaddr2int(&(rte->rt_gateway)),
            rte->rt_flags, 
            rte->rt_metric, 
            sockaddr2int(&(rte->rt_genmask)));
    fputs(buf, f);
    fclose(f);
}

int route_modify(int cmd, int flags, struct rtentry *rte) {

    for (int i = 0; i < rtentries_number; i ++) {
        if (!memcmp(rtentries + i, rte, sizeof(struct rtentry))) {
            return 0;
        }
    }

    append_route(rte);

    memcpy(rtentries + rtentries_number, rte, sizeof (struct rtentry));
    rtentries_number ++;


    struct {
        struct nlmsghdr n;
        struct rtmsg    r;
        char            buf[1024];
    } req;

    memset(&req, 0, sizeof(req));

    req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    req.n.nlmsg_flags = NLM_F_REQUEST | flags;
    req.n.nlmsg_type = cmd;
    req.r.rtm_family = rte->rt_dst.sa_family;
    req.r.rtm_table = 128;
    req.r.rtm_protocol = RTPROT_BOOT;
    req.r.rtm_scope = RT_SCOPE_UNIVERSE;
    req.r.rtm_type = RTN_UNICAST;
	req.r.rtm_dst_len = sockaddr2len(&(rte->rt_genmask));

    int dev = ll_name_to_index(rte->rt_dev);

    struct rta_addr rta_dst_addr;
	struct rta_addr rta_gw_addr;

	sockaddr2rta(&rta_dst_addr, &(rte->rt_dst));
	sockaddr2rta(&rta_gw_addr, &(rte->rt_gateway));

	addattr_l(&req.n, sizeof(req), RTA_DST, rta_dst_addr.addr, rta_dst_addr.len);
    addattr_l(&req.n, sizeof(req), RTA_GATEWAY, rta_gw_addr.addr, rta_gw_addr.len);
    addattr32(&req.n, sizeof(req), RTA_PRIORITY, rte->rt_metric);

    if (dev != 0)
        addattr32(&req.n, sizeof(req), RTA_OIF, dev);
    
    if (rtnl_talk(&rth, &req.n, 0, 0, NULL) < 0)
		puts("Error talking to netlink");

    return 0;
}

int ioctl(int fd, unsigned long request, void *arg) {
	switch (request) {
	case SIOCADDRT:
		return route_modify(RTM_NEWROUTE, NLM_F_CREATE|NLM_F_EXCL, arg);
	case SIOCDELRT:
		return route_modify(RTM_DELROUTE, 0, arg);
	default:
	    return ioctl_next(fd, request, arg);
	}
}


void *wrap(char *call) {
    void *ptr = dlsym(RTLD_NEXT, call);
    if (ptr == NULL) {
        fprintf(stderr, "Failed to dynamicly link '%s' call", call);
        exit(3);
    }
    return ptr;
}

int main_wrap(int argc, char * *argv, char * *envp) {

    char * up_script = NULL;
    char * down_script = NULL;

    int opt;

    while ((opt = getopt(argc, argv, "u:d:l:")) != -1) {
        switch (opt) {
            case 'u':
                up_script = optarg;
                break;
            case 'd':
                down_script = optarg;
                break;
            case 'l':
                printf("log: %s", optarg);
                break;
        }
    }

    int optoff = optind - 1;
    argv[optoff] = argv[0];
    optind = 0;

    int result = 0;
    if (up_script != NULL) {
        if(result = system(up_script)) {
            puts("Up script returned error");
            exit(result);
        }
    }

    result = main_orig(argc - optoff, argv + optoff, envp);

    if (down_script != NULL) {
        if(result = system(down_script)) {
            puts("Down script returned error");
            exit(result);
        }
    }

    return result;

}

int __libc_start_main(int (*main) (int, char * *, char * *), int argc, char * * argv, void (*a_init) (void), void (*fini) (void), void (*rtld_fini) (void), void (* stack_end)) {
    main_orig = main;
    return __libc_start_main_next(main_wrap, argc - optind + 1, argv + optind - 1, a_init, fini, rtld_fini, stack_end);
}

void init() {
    ioctl_next = wrap("ioctl");
    open_next = wrap("open");
    chown_next = wrap("chown");
    mkdir_next = wrap("mkdir");
    write_next = wrap("write");
    __xstat_next = wrap("__xstat");
    __fxstat_next = wrap("__fxstat");
    __libc_start_main_next = wrap("__libc_start_main");

    unsetenv("LD_PRELOAD");

    original_log_file_name = strcat(getenv("HOME"), "/.juniper_networks/network_connect/ncsvc.log");

	if (rtnl_open(&rth, 0) < 0 ) {
		puts("Unable to connect to netlink socket");
		exit(1);
	}

    FILE *f = fopen(routes_file, "w");
    if (f == NULL) {
        puts("Unable to open routes file");
        exit(1);
    }
    fputs("Iface\tDestination\tGateway \tFlags\tRefCnt\tUse\tMetric\tMask\tMTU\tWindow\tIRTT\n", f); 
    fclose(f);
}

void fini() {
    rtnl_close(&rth);
}
