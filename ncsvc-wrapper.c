#define _GNU_SOURCE


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <asm/stat.h>
#include <dlfcn.h>
#include <string.h>
#include <unistd.h>


static int (* open_next)(const char *, int);
static int (* chown_next)(const char *, uid_t, gid_t);
static int (* __xstat_next)(int ver, const char *, struct stat *);
static int (* __fxstat_next)(int ver, int, struct stat *);
static int (* mkdir_next)(const char *, mode_t);

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
	 return open_next(process_path(pathname), flags);
}

int chown(const char *path, uid_t owner, gid_t group) {
	if (strstr(path, ".juniper_networks") != NULL) {
		return 0;
	}
	return chown_next(path, owner, group);
}

int __xstat(int ver, const char *pathname, struct stat *buf) {
	return __xstat_next(ver, process_path(pathname), buf);
}

int __fxstat(int ver, int fd, struct stat *buf) {
	return __fxstat_next(ver, fd, buf);
}

int mkdir(const char *pathname, mode_t mode) {
	return 0;
//	return mkdir_next(pathname, mode);
}

void *wrap(char *call) {
	void *ptr = dlsym(RTLD_NEXT, call);
	if (ptr == NULL) {
		fprintf(stderr, "Failed to dynamicly link '%s' call", call);
		exit(3);
	}
	return ptr;
}

void init() {
	open_next = wrap("open");
	chown_next = wrap("chown");
	__xstat_next = wrap("__xstat");
	__fxstat_next = wrap("__fxstat");
	mkdir_next = wrap("mkdir");

	jnc_client_log_path = "/root/jncclient.log";
}

void dispose() {
}

