/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU LGPLv2.
  See the file COPYING.LIB.

  CopperFuse: C++ version Filesystem in Userspace
  Copyright (C) 2023 Chen Miao <chenmiao.ku@gmail.com>
*/

#ifndef __COPPER_FUSE_LOWLEVEL_H__
#define __COPPER_FUSE_LOWLEVEL_H__

/** ---------------------------------------------------------- *
 * Filesystem setup & teardown                                 *
 * ----------------------------------------------------------- */

/**
 * Note: Any addition to this struct needs to create a compatibility symbol
 *       for fuse_parse_cmdline(). For ABI compatibility reasons it is also
 *       not possible to remove struct members.
 */
struct copper_fuse_cmdline_opts {
	int singlethread;
	int foreground;
	int debug;
	int nodefault_subtype;
	char* mountpoint;
	int show_version;
	int show_help;
	int clone_fd;
	unsigned int max_idle_threads; /* discouraged, due to thread
	                                * destruct overhead */
	unsigned int max_threads;

public:
	int add_opt(const char* opt);
};

int add_opt_common(char** opts, const char* opt, int esc);

#endif //! __COPPER_FUSE_LOWLEVEL_H__