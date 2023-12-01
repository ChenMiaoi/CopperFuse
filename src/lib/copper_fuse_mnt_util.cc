#include "copper_fuse_mnt_util.h"
#include <cstdio>
#include <cstring>

int copper_fuse_mnt_parse_fd(const char *mountpoint) {
	 int fd = -1;
	 int len = 0;

	 if (sscanf(mountpoint, "/dev/fd/%u%n", &fd, &len) == 1 && 
	 	len == strlen(mountpoint)) {
		return fd;
	}

	return -1;
}