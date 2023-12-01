/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU LGPLv2.
  See the file COPYING.LIB.

  CopperFuse: C++ version Filesystem in Userspace
  Copyright (C) 2023 Chen Miao <chenmiao.ku@gmail.com>
*/

#ifndef __COPPER_FUSE_COMMON_H__
#define __COPPER_FUSE_COMMON_H__

#include "copper_cuse_lowlevel.h"

#include <cstddef>

constexpr const size_t COPPER_FUSE_MAJOR = 1;
constexpr const size_t COPPER_FUSE_MINOR = 1;
#define COPPER_MAKE_VERSION(major,minor) (major * 100 + minor)
constexpr const size_t COPPER_FUSE_VERSION = 
	COPPER_MAKE_VERSION(COPPER_FUSE_MAJOR, COPPER_FUSE_MINOR);

/**
 * Configuration parameters passed to fuse_session_loop_mt() and
 * fuse_loop_mt().
 * Deprecated and replaced by a newer private struct in FUSE API
 * version 312 (FUSE_MAKE_VERSION(3, 12)
 */
#if COPPER_FUSE_USE_VERSION < COPPER_MAKE_VERSION(3, 12)
struct fuse_loop_config_v1; /* forward declarition */
struct copper_fuse_loop_config {
#else
struct fuse_loop_config_v1 {
#endif
	/**
	 * whether to use separate device fds for each thread
	 * (may increase performance)
	 */
	int clone_fd;

	/**
	 * The maximum number of available worker threads before they
	 * start to get deleted when they become idle. If not
	 * specified, the default is 10.
	 *
	 * Adjusting this has performance implications; a very small number
	 * of threads in the pool will cause a lot of thread creation and
	 * deletion overhead and performance may suffer. When set to 0, a new
	 * thread will be created to service every operation.
	 */
	unsigned int max_idle_threads;
};

struct copper_fuse_conn_info {

};

#endif //! __COPPER_FUSE_COMMON_H__