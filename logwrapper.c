#define _GNU_SOURCE


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <asm/stat.h>
#include <dlfcn.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>

#include "jncclient.h"

static int (* open_next)(const char *, int);
static int (* chown_next)(const char *, uid_t, gid_t);
static int (* mkdir_next)(const char *, mode_t);
static ssize_t (* write_next)(int fd, const void *buf, size_t count);
static int (* __xstat_next)(int ver, const char *, struct stat *);
static int (* __fxstat_next)(int ver, int, struct stat *);

int (* main_orig)(int, char **, char **);

static const char * jnc_client_log_path;

static const char * original_log_file_name;

void init() __attribute__((constructor));
void dispose() __attribute__((destructor));

static const int log_fd = 0xFFFFFF;

int open(const char *pathname, int flags) {
    if (!strcmp(pathname, original_log_file_name)) {
        // open for log file returns fake descriptor for further handling
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
    if (strstr(original_log_file_name, pathname) != NULL) {
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
    if (fd == log_fd) {
        // redirect log output to stdout
        // TODO add options to use sysctl and stderr
        return write_next(0, buf, count);
    }
    return write_next(fd, buf, count);
}

int main_wrap(int argc, char * *argv1, char * *argv2) {
    int result = main_orig(argc, argv1, argv2);
    return result;
}



