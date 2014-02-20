#define _GNU_SOURCE


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <asm/stat.h>
#include <dlfcn.h>
#include <string.h>
#include <unistd.h>


static int (* real_open)(const char *, int);
static int (* real_chown)(const char *, uid_t, gid_t);
static int (* real_stat)(const char *, struct stat64 *);

static int (* real_main)(int, char **);

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
    return real_open(process_path(pathname), flags);
}

int chown(const char *path, uid_t owner, gid_t group) {
	if (strstr(path, ".juniper_networks") != NULL) {
		return 0;
	}
    return real_chown(path, owner, group);
}

int stat64(const char *pathname, struct stat64 *buf) {
	if (real_stat == NULL) {
		real_stat = dlsym(RTLD_NEXT, "stat64");
    	if (real_stat == NULL) {
        	puts("Failed to dynamicly link int (* stat64)(const char * pathname, struct stat *buf)");
	        exit(3);
    	}
	}
	puts("calling real_stat");
	return real_stat(process_path(pathname), buf);
}

void init() {
    real_open = dlsym(RTLD_NEXT, "open");
    if (real_open == NULL) {
        puts("Failed to dynamicly link int (* open)(const char * pathname, int flags)");
        exit(3);
    }
    real_chown = dlsym(RTLD_NEXT, "chown");
    if (real_chown == NULL) {
        puts("Failed to dynamicly link int (* chown)(const char * path, uid_t owner, git_t group)");
        exit(3);
    }
	real_stat = dlsym(RTLD_NEXT, "stat64");
    if (real_stat == NULL) {
        puts("Failed to dynamicly link int (* stat)(const char * pathname, struct stat *buf)");
        exit(3);
    }

    jnc_client_log_path = "/root/jncclient.log";
}

void dispose() {
}

