/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU LGPLv2.
  See the file COPYING.LIB.

  CopperFuse: C++ version Filesystem in Userspace
  Copyright (C) 2023 Chen Miao <chenmiao.ku@gmail.com>
*/

#ifndef __COPPER_FUSE_MOUNT_UTIL_H__
#define __COPPER_FUSE_MOUNT_UTIL_H__

#include <sys/types.h>

int copper_fuse_mnt_parse_fd(const char* mountpoint);

#endif //! __COPPER_FUSE_MOUNT_UTIL_H__