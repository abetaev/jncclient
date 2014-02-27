#ifndef JNCCLIENT_H
#define JNCCLIENT_H

#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <asm/stat.h>
#include <dlfcn.h>
#include <string.h>
#include <signal.h>
#include <poll.h>


void *wrap(char *call);

static int (* open_next)(const char *, int);
static int (* chown_next)(const char *, uid_t, gid_t);
static int (* mkdir_next)(const char *, mode_t);
static ssize_t (* write_next)(int fd, const void *buf, size_t count);
static int (* __xstat_next)(int ver, const char *, struct stat *);
static int (* __fxstat_next)(int ver, int, struct stat *);
static int (*ioctl_next)(int, unsigned long, void*);

int (* main_orig)(int, char **, char **);

static const char * original_log_file_name;



#endif//JNCCLIENT_H
