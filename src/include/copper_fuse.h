/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU LGPLv2.
  See the file COPYING.LIB.

  CopperFuse: C++ version Filesystem in Userspace
  Copyright (C) 2023 Chen Miao <chenmiao.ku@gmail.com>
*/

#ifndef __COPPER_FUSE_H__
#define __COPPER_FUSE_H__

#include "copper_fuse_common.h"
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/uio.h>

#include <functional>
#include <variant>

/** ----------------------------------------------------------- *
 * Basic FUSE API					       
 * ------------------------------------------------------------ */

/**
 * Readdir flags, passed to ->readdir()
 */
enum fuse_readdir_flags {
  /**
	 * "Plus" mode.
	 *
	 * The kernel wants to prefill the inode cache during readdir.  The
	 * filesystem may honour this by filling in the attributes and setting
	 * FUSE_FILL_DIR_FLAGS for the filler function.  The filesystem may also
	 * just ignore this flag completely.
	 */
  FUSE_READDIR_PLUS = (1 << 0),
};

/**
 * Readdir flags, passed to fuse_fill_dir_t callback.
 */
enum fuse_fill_dir_flags {
  /**
	 * "Plus" mode: all file attributes are valid
	 *
	 * The attributes are used by the kernel to prefill the inode cache
	 * during a readdir.
	 *
	 * It is okay to set FUSE_FILL_DIR_PLUS if FUSE_READDIR_PLUS is not set
	 * and vice versa.
	 */
	FUSE_FILL_DIR_PLUS = (1 << 1)
};

/** Function to add an entry in a readdir() operation
 *
 * The *off* parameter can be any non-zero value that enables the
 * filesystem to identify the current point in the directory
 * stream. It does not need to be the actual physical position. A
 * value of zero is reserved to indicate that seeking in directories
 * is not supported.
 * 
 * @param buf the buffer passed to the readdir() operation
 * @param name the file name of the directory entry
 * @param stbuf file attributes, can be NULL
 * @param off offset of the next entry or zero
 * @param flags fill flags
 * @return 1 if buffer is full, zero otherwise
 */
using fuse_fill_dir_t = std::function < int(void *buf, const char *name,
                                            const struct stat *stbuf,
                                            off_t off, enum fuse_fill_dir_flags flags)>;

struct copper_fuse {

};

/**
 * Configuration of the high-level API
 *
 * This structure is initialized from the arguments passed to
 * fuse_new(), and then passed to the file system's init() handler
 * which should ensure that the configuration is compatible with the
 * file system implementation.
 * 
 * TODO My plan is to have fuse refactor into an oop architecture
 * 
 */
struct copper_fuse_config {
  /**
	 * If `set_gid` is non-zero, the st_gid attribute of each file
	 * is overwritten with the value of `gid`.
	 */
  int set_gid;
  unsigned int gid;

  /**
	 * If `set_uid` is non-zero, the st_uid attribute of each file
	 * is overwritten with the value of `uid`.
	 */
	int set_uid;
	unsigned int uid;

  /**
	 * If `set_mode` is non-zero, the any permissions bits set in
	 * `umask` are unset in the st_mode attribute of each file.
	 */
	int set_mode;
	unsigned int umask;

  /**
	 * The timeout in seconds for which name lookups will be
	 * cached.
	 */
	double entry_timeout;

  /**
	 * The timeout in seconds for which a negative lookup will be
	 * cached. This means, that if file did not exist (lookup
	 * returned ENOENT), the lookup will only be redone after the
	 * timeout, and the file/directory will be assumed to not
	 * exist until then. A value of zero means that negative
	 * lookups are not cached.
	 */
	double negative_timeout;

  /**
	 * The timeout in seconds for which file/directory attributes
	 * (as returned by e.g. the `getattr` handler) are cached.
	 */
	double attr_timeout;

  /**
	 * Allow requests to be interrupted
	 */
	int intr;

  /**
	 * Specify which signal number to send to the filesystem when
	 * a request is interrupted.  The default is hardcoded to
	 * USR1.
	 */
	int intr_signal;

  /**
	 * Normally, FUSE assigns inodes to paths only for as long as
	 * the kernel is aware of them. With this option inodes are
	 * instead remembered for at least this many seconds.  This
	 * will require more memory, but may be necessary when using
	 * applications that make use of inode numbers.
	 *
	 * A number of -1 means that inodes will be remembered for the
	 * entire life-time of the file-system process.
	 */
	int remember;

  /**
	 * The default behavior is that if an open file is deleted,
	 * the file is renamed to a hidden file (.fuse_hiddenXXX), and
	 * only removed when the file is finally released.  This
	 * relieves the filesystem implementation of having to deal
	 * with this problem. This option disables the hiding
	 * behavior, and files are removed immediately in an unlink
	 * operation (or in a rename operation which overwrites an
	 * existing file).
	 *
	 * It is recommended that you not use the hard_remove
	 * option. When hard_remove is set, the following libc
	 * functions fail on unlinked files (returning errno of
	 * ENOENT): `read(2), write(2), fsync(2), close(2), f*xattr(2)`,
	 * `ftruncate(2), fstat(2), fchmod(2), fchown(2)`
	 */
	int hard_remove;

  /**
	 * Honor the st_ino field in the functions getattr() and
	 * fill_dir(). This value is used to fill in the st_ino field
	 * in the stat(2), lstat(2), fstat(2) functions and the d_ino
	 * field in the readdir(2) function. The filesystem does not
	 * have to guarantee uniqueness, however some applications
	 * rely on this value being unique for the whole filesystem.
	 *
	 * Note that this does *not* affect the inode that libfuse 
	 * and the kernel use internally (also called the "nodeid").
   *
   * ! The st_ino seen by the application cannot be repeated, 
   * but inside fuse or the kernel, the st_ino-mapped inode can be repeated
	 */
	int use_ino;

  /**
	 * If use_ino option is not given, still try to fill in the
	 * d_ino field in readdir(2). If the name was previously
	 * looked up, and is still in the cache, the inode number
	 * found there will be used.  Otherwise it will be set to -1.
	 * If use_ino option is given, this option is ignored.
	 */
	int readdir_ino;

  /**
	 * This option disables the use of page cache (file content cache)
	 * in the kernel for this filesystem. This has several affects:
	 *
	 * 1. Each read(2) or write(2) system call will initiate one
	 *    or more read or write operations, data will not be
	 *    cached in the kernel.
	 *
	 * 2. The return value of the read() and write() system calls
	 *    will correspond to the return values of the read and
	 *    write operations. This is useful for example if the
	 *    file size is not known in advance (before reading it).
	 *
	 * Internally, enabling this option causes fuse to set the
	 * `direct_io` field of `struct fuse_file_info` - overwriting
	 * any value that was put there by the file system.
	 */
	int direct_io;

  /**
	 * This option disables flushing the cache of the file
	 * contents on every open(2).  This should only be enabled on
	 * filesystems where the file data is never changed
	 * externally (not through the mounted FUSE filesystem).  Thus
	 * it is not suitable for network filesystems and other
	 * intermediate filesystems.
	 *
	 * NOTE: if this option is not specified (and neither
	 * direct_io) data is still cached after the open(2), so a
	 * read(2) system call will not always initiate a read
	 * operation.
	 *
	 * Internally, enabling this option causes fuse to set the
	 * `keep_cache` field of `struct fuse_file_info` - overwriting
	 * any value that was put there by the file system.
	 */
	int kernel_cache;

  /**
	 * This option is an alternative to `kernel_cache`. Instead of
	 * unconditionally keeping cached data, the cached data is
	 * invalidated on open(2) if if the modification time or the
	 * size of the file has changed since it was last opened.
	 */
	int auto_cache;

  /**
	 * By default, fuse waits for all pending writes to complete
	 * and calls the FLUSH operation on close(2) of every fuse fd.
	 * With this option, wait and FLUSH are not done for read-only
	 * fuse fd, similar to the behavior of NFS/SMB clients.
	 */
	int no_rofd_flush;

  /**
	 * The timeout in seconds for which file attributes are cached
	 * for the purpose of checking if auto_cache should flush the
	 * file data on open.
	 */
	int ac_attr_timeout_set;
	double ac_attr_timeout;

  /**
	 * If this option is given the file-system handlers for the
	 * following operations will not receive path information:
	 * read, write, flush, release, fallocate, fsync, readdir,
	 * releasedir, fsyncdir, lock, ioctl and poll.
	 *
	 * For the truncate, getattr, chmod, chown and utimens
	 * operations the path will be provided only if the struct
	 * fuse_file_info argument is NULL.
	 */
	int nullpath_ok;

  /**
	 *  Allow parallel direct-io writes to operate on the same file.
	 *
	 *  FUSE implementations which do not handle parallel writes on
	 *  same file/region should NOT enable this option at all as it
	 *  might lead to data inconsistencies.
	 *
	 *  For the FUSE implementations which have their own mechanism
	 *  of cache/data integrity are beneficiaries of this setting as
	 *  it now open doors to parallel writes on the same file (without
	 *  enabling this setting, all direct writes on the same file are
	 *  serialized, resulting in huge data bandwidth loss).
	 */
	int parallel_direct_writes;

  /**
	 * The remaining options are used by libfuse internally and
	 * should not be touched.
	 */
	int   show_help;
	char* modules;
	int   debug;
};

#if defined (__cplusplus) && (__cplusplus >= 201703L)
template <typename ret_type, typename... params_type>
struct operators_wrapper {
  // using type = std::variant<std::function<ret_type(params_type...)>, ret_type(*)(params_type...)>;
	using type = std::function<ret_type(params_type...)>;
};

template <typename ret_type, typename... params_type>
using operators_wrapper_type = typename operators_wrapper<ret_type, params_type...>::type;
#else
# define operators_wrapper_type(ret_type, params_type...) \
    ret_type(*)(params_type)
#endif

/**
 * The file system operations:
 *
 * Most of these should work very similarly to the well known UNIX
 * file system operations.  A major exception is that instead of
 * returning an error in 'errno', the operation should return the
 * negated error value (-errno) directly.
 *
 * All methods are optional, but some are essential for a useful
 * filesystem (e.g. getattr).  Open, flush, release, fsync, opendir,
 * releasedir, fsyncdir, access, create, truncate, lock, init and
 * destroy are special purpose methods, without which a full featured
 * filesystem can still be implemented.
 *
 * In general, all methods are expected to perform any necessary
 * permission checking. However, a filesystem may delegate this task
 * to the kernel by passing the `default_permissions` mount option to
 * `fuse_new()`. In this case, methods will only be called if
 * the kernel's permission check has succeeded.
 *
 * Almost all operations take a path which can be of any length.
 *
 * ----------------------------------------------------------------------------------
 * 
 * Here, I want to keep the pure C style of the kernel, 
 * but I want to support a bit of modern syntax without losing too much performance. 
 * Therefore, I decided to use an "abstract" syntax
 */
struct copper_fuse_operations {
  operators_wrapper_type<int, const char*, struct stat*, struct fuse_file_info*> getattr;
  operators_wrapper_type<int, const char*, char*, size_t> readlink;
  operators_wrapper_type<int, const char*, mode_t, dev_t> mknod;
  operators_wrapper_type<int, const char*, mode_t> mkdir;
  operators_wrapper_type<int, const char*> unlink;
  operators_wrapper_type<int, const char*> rmdir;
  operators_wrapper_type<int, const char*, const char*> symlink;
  operators_wrapper_type<int, const char*, const char*, unsigned int> rename;
  operators_wrapper_type<int, const char*, const char*> link;
  operators_wrapper_type<int, const char*, mode_t, struct fuse_file_info*> chmod;
  operators_wrapper_type<int, const char*, uid_t, gid_t, struct fuse_file_info*> chown;
  operators_wrapper_type<int, const char*, off_t, struct fuse_file_info*> truncate;
  operators_wrapper_type<int, const char*, struct fuse_file_info*> open;
  operators_wrapper_type<int, const char*, char*, size_t, off_t, struct fuse_file_info*> read;
  operators_wrapper_type<int, const char*, const char*, size_t, off_t, struct fuse_file_info*> write;
  operators_wrapper_type<int, const char*, struct statvfs*> statfs;
  operators_wrapper_type<int, const char*, struct fuse_file_info*> flush;
  operators_wrapper_type<int, const char*, struct fuse_file_info*> release;
  operators_wrapper_type<int, const char*, int, struct fuse_file_info*> fsync;
  operators_wrapper_type<int, const char*, const char*, const char*, size_t, int> setxattr;
  operators_wrapper_type<int, const char*, const char*, char*, size_t> getxattr;
  operators_wrapper_type<int, const char*, char*, size_t> listxattr;
  operators_wrapper_type<int, const char*, const char*> removexattr;
  operators_wrapper_type<int, const char*, struct fuse_file_info*> opendir;
  operators_wrapper_type<int, const char*, void*, fuse_fill_dir_t, off_t, 
    struct fuse_file_info*, enum fuse_readdir_flags> readdir;
  operators_wrapper_type<int, const char*, struct fuse_file_info*> releasedir;
  operators_wrapper_type<int, const char*, int, struct fuse_file_info*> fsyncdir;
  operators_wrapper_type<void*, struct copper_fuse_conn_info*, struct copper_fuse_config*> init;
  operators_wrapper_type<void, void*> destroy;
  operators_wrapper_type<int, const char*, int> access;
  operators_wrapper_type<int, const char*, mode_t, struct fuse_file_info*> create;
  operators_wrapper_type<int, const char*, struct fuse_file_info*, int, struct flock*> lock;
  operators_wrapper_type<int, const char*, const struct timespec[2], struct fuse_file_info*> utimens;
  operators_wrapper_type<int, const char*, size_t, uint64_t*> bmap;
#if FUSE_USE_VERSION < 35
  operators_wrapper_type<int, const char*, int, void*, struct fuse_file_info*, unsigned int, void*> ioctl;
#else
	operators_wrapper_type<int, const char*, unsigned int, void*, struct fuse_file_info*, unsigned int, void*> ioctl;
#endif
	operators_wrapper_type<int, const char*, struct fuse_file_info*, struct fuse_pollhandle*, unsigned*> poll;
	operators_wrapper_type<int, const char*, struct fuse_bufvec*, off_t, struct fuse_file_info*> write_buf;
	operators_wrapper_type<int, const char*, struct fuse_bufvec**, size_t, off_t, struct fuse_file_info*> read_buf;
	operators_wrapper_type<int, const char*, struct fuse_file_info*, int> flock;
	operators_wrapper_type<int, const char*, int, off_t, off_t, struct fuse_file_info*> fallocate;
	operators_wrapper_type<ssize_t, const char*, struct fuse_file_info*, off_t, const char*, struct fuse_file_info*, off_t, size_t, int> copy_file_range;
	operators_wrapper_type<off_t, const char*, off_t, int, struct fuse_file_info*> lseek;
};

/** 
 * Extra context that may be needed by some filesystems
 *
 * The uid, gid and pid fields are not filled in case of a writepage
 * operation.
 */
struct copper_fuse_context {
  /** Pointer to the fuse object */
	struct copper_fuse* fuse;

	/** User ID of the calling process */
	uid_t uid;

	/** Group ID of the calling process */
	gid_t gid;

	/** Process ID of the calling thread */
	pid_t pid;

	/** Private filesystem data */
	void* private_data;

	/** Umask of the calling process */
	mode_t umask;
};

/**
 * Main function of FUSE.
 *
 * This is for the lazy.  This is all that has to be called from the
 * main() function.
 *
 * This function does the following:
 *   - parses command line options, and handles --help and
 *     --version
 *   - installs signal handlers for INT, HUP, TERM and PIPE
 *   - registers an exit handler to unmount the filesystem on program exit
 *   - creates a fuse handle
 *   - registers the operations
 *   - calls either the single-threaded or the multi-threaded event loop
 *
 * Most file systems will have to parse some file-system specific
 * arguments before calling this function. It is recommended to do
 * this with fuse_opt_parse() and a processing function that passes
 * through any unknown options (this can also be achieved by just
 * passing NULL as the processing function). That way, the remaining
 * options can be passed directly to fuse_main().
 *
 * fuse_main() accepts all options that can be passed to
 * fuse_parse_cmdline(), fuse_new(), or fuse_session_new().
 *
 * Option parsing skips argv[0], which is assumed to contain the
 * program name. This element must always be present and is used to
 * construct a basic ``usage: `` message for the --help
 * output. argv[0] may also be set to the empty string. In this case
 * the usage message is suppressed. This can be used by file systems
 * to print their own usage line first. See hello.c for an example of
 * how to do this.
 *
 * Note: this is currently implemented as a macro.
 *
 * The following error codes may be returned from fuse_main():
 *   1: Invalid option arguments
 *   2: No mount point specified
 *   3: FUSE setup failed
 *   4: Mounting failed
 *   5: Failed to daemonize (detach from session)
 *   6: Failed to set up signal handlers
 *   7: An error occurred during the life of the file system
 *
 * @param argc the argument counter passed to the main() function
 * @param argv the argument vector passed to the main() function
 * @param op the file system operation
 * @param private_data Initial value for the `private_data`
 *            field of `struct fuse_context`. May be overridden by the
 *            `struct fuse_operations.init` handler.
 * @return 0 on success, nonzero on failure
 *
 * Example usage, see hello.c
 */
/*
  int fuse_main(int argc, char *argv[], const struct fuse_operations *op,
  void *private_data);
*/
#define copper_fuse_main(argc, argv, op, private_data)  \
  copper_fuse_main_real(argc, argv, op, sizeof(*(op)), private_data)

/**
 * The real main function
 *
 * Do not call this directly, use fuse_main()
 */
int copper_fuse_main_real(int argc, char *argv[], 
  const struct copper_fuse_operations *op, size_t op_size, void *private_data);

#endif //! __COPPER_FUSE_H__