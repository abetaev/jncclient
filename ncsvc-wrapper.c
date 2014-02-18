#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>
#include <string.h>


static int (* real_open)(const char *pathname, int flags);
static int (* real_chown32)(const char *path, uid_t owner, gid_t group);
static int (* real_stat64)(const char *pathname, struct stat *buf);

static const char * jnc_client_log_path;

static const char * ncsvc_log_file_name = "ncsvc.log";

void init() __attribute__((constructor));
void dispose() __attribute__((destructor));

const char * process_path(const char *path) {
    char *filename = basename(path);
    if (!strcmp("ncsvc.log", filename)) {
        return jnc_client_log_path;
    }
    return path;
}

int open(const char *pathname, int flags) {
    return real_open(process_path(pathname), int flags);
}

int stat64(const char *pathname, struct stat *buf) {
    return real_stat64(process_path(pathname), buf);
}

int chown32(const char *path, uid_t owner, git_t group) {
    return real_chown32(process_path(path), owner, group);
}

void init() {
    real_open = dlsym(RTLD_NEXT, "open");
    if (real_open == NULL) {
        puts("Failed to dynamicly link int (* open)(const char * pathname, int flags)");
        exit(3);
    }
    real_chown32 = dlsym(RTLD_NEXT, "chown32");
    if (real_open == NULL) {
        puts("Failed to dynamicly link int (* chown32)(const char * path, uid_t owner, git_t group)");
        exit(3);
    }
    real_chown32 = dlsym(RTLD_NEXT, "stat64");
    if (real_open == NULL) {
        puts("Failed to dynamicly link int (* stat64)(const char * pathname, struct stat *buf)");
        exit(3);
    }
    jnc_client_log_path = "/root/jncclient.log";
}

void dispose() {
}

