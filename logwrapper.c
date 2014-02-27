#include "jncclient.h"

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


