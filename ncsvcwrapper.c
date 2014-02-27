#include "jncclient.h"

static int (* __libc_start_main_next)(int (*main) (int, char * *, char * *), int argc, char * * ubp_av, void (*init) (void), void (*fini) (void), void (*rtld_fini) (void), void (* stack_end));

void *wrap(char *call) {
    void *ptr = dlsym(RTLD_NEXT, call);
    if (ptr == NULL) {
        fprintf(stderr, "Failed to dynamicly link '%s' call", call);
        exit(3);
    }
    return ptr;
}

int __libc_start_main(int (*main) (int, char * *, char * *), int argc, char * * ubp_av, void (*a_init) (void), void (*fini) (void), void (*rtld_fini) (void), void (* stack_end)) {
    main_orig = main;
    // TODO parse arguments to determine wether any start/stop scripts are required
    // TODO run start script
    int result = __libc_start_main_next(main_wrap, argc, ubp_av, a_init, fini, rtld_fini, stack_end);
    // TODO run stop script
    return result;
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

    jnc_client_log_path = "/root/jncclient.log";

    unsetenv("LD_PRELOAD");

    original_log_file_name = strcat(getenv("HOME"), "/.juniper_networks/network_connect/ncsvc.log");
}

