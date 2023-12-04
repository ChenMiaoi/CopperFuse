/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU LGPLv2.
  See the file COPYING.LIB.

  CopperFuse: C++ version Filesystem in Userspace
  Copyright (C) 2023 Chen Miao <chenmiao.ku@gmail.com>
*/

#ifndef __COPPER_FUSE_OPT_H__
#define __COPPER_FUSE_OPT_H__

#include "copper_fuse.h"
#include "copper_fuse_config.h"
#include "copper_fuse_lowlevel.h"

#include <cstring>
#include <cstdlib>
#include <functional>
#include <ostream>
#include <string>
#include <limits>
#include <thread>
#include <numeric>

/**
 * Option description
 *
 * This structure describes a single option, and action associated
 * with it, in case it matches.
 *
 * More than one such match may occur, in which case the action for
 * each match is executed.
 *
 * There are three possible actions in case of a match:
 *
 * i) An integer (int or unsigned) variable determined by 'offset' is
 *    set to 'value'
 *
 * ii) The processing function is called, with 'value' as the key
 *
 * iii) An integer (any) or string (char *) variable determined by
 *    'offset' is set to the value of an option parameter
 *
 * 'offset' should normally be either set to
 *
 *  - 'offsetof(struct foo, member)'  actions i) and iii)
 *
 *  - -1			      action ii)
 *
 * The 'offsetof()' macro is defined in the <stddef.h> header.
 *
 * The template determines which options match, and also have an
 * effect on the action.  Normally the action is either i) or ii), but
 * if a format is present in the template, then action iii) is
 * performed.
 *
 * The types of templates are:
 *
 * 1) "-x", "-foo", "--foo", "--foo-bar", etc.	These match only
 *   themselves.  Invalid values are "--" and anything beginning
 *   with "-o"
 *
 * 2) "foo", "foo-bar", etc.  These match "-ofoo", "-ofoo-bar" or
 *    the relevant option in a comma separated option list
 *
 * 3) "bar=", "--foo=", etc.  These are variations of 1) and 2)
 *    which have a parameter
 *
 * 4) "bar=%s", "--foo=%lu", etc.  Same matching as above but perform
 *    action iii).
 *
 * 5) "-x ", etc.  Matches either "-xparam" or "-x param" as
 *    two separate arguments
 *
 * 6) "-x %s", etc.  Combination of 4) and 5)
 *
 * If the format is "%s", memory is allocated for the string unlike with
 * scanf().  The previous value (if non-NULL) stored at the this location is
 * freed.
 */
struct copper_fuse_opt {
	/** Matching template and optional parameter formatting */
	const char* templ;

	/**
	 * Offset of variable within 'data' parameter of fuse_opt_parse()
	 * or -1
	 */
	unsigned long offset;

	/**
	 * Value to set the variable to, or to be passed as 'key' to the
	 * processing function.	 Ignored if template has a format
	 */
	int value;

public:
	/**
	 * Matches the given arg in all given parameter templates by `match_template`
	 * 
	 * @param arg Parameters to be found
	 * @param sepp 
	 * @return const copper_fuse_opt* Matched opt
	 */
	const copper_fuse_opt* find_opt(const char* arg, unsigned* sepp) const;
};

using copper_fuse_opt_proc_t = 
		std::function<int(void*, const char*, int, struct copper_fuse_args*)>;

/**
 * Argument list
 */
struct copper_fuse_args {
	/** Argument count */
	int argc;

	/** Argument vector.  NULL terminated */
	char **argv;

	/** Is 'argv' allocated? */
	int allocated;

	friend std::ostream& operator<< (std::ostream& _out, const copper_fuse_args& args);
public:
	copper_fuse_args() = default;
    copper_fuse_args(int _argc, char** _argv);

public:

  	int parse_cmdline(struct copper_fuse_cmdline_opts* opts);
	/**
	 * @brief 
	 * 
	 * @param data 
	 * @param opts 
	 * @param proc 
	 * @return int 
	 */
	int parse_opt(void* data, const copper_fuse_opt opts[], copper_fuse_opt_proc_t proc);

	int add_arg(const char* arg);
	int insert_arg(int pos, const char* arg);
};

inline struct copper_fuse_opt COPPER_FUSE_OPT_KEY(const char* tmpl, int key) {
	return { tmpl, -1U, key };
}
const struct copper_fuse_opt COPPER_FUSE_OPT_END { nullptr, 0, 0 };

enum COPPER_FUSE_OPT {
	KEY_OPT 		= -1,
	KEY_NONOPT 	= -2,
	KEY_KEEP 		= -3,
	KEY_DISCARD = -4,
};

struct copper_fuse_opt_context {
	void* data;
	const struct copper_fuse_opt* opt;
	copper_fuse_opt_proc_t proc;
	int argctr;
	int argc;
	char** argv;
	struct copper_fuse_args outargs;
	char* opts;
	int nonopt;

private:
	/**
	 * Handling the parsing of a single parameter,
	 * possible cases are nonopt, "-o ", "-ofoo", "-h", and so on
	 * 
	 * @param arg Parameters to be parsed
	 * @return int Return 0 if parsing is normal, -1 otherwise
	 */
	int process_one(const char* arg);
	int process_option_group(const char* opts);
	int process_real_option_group(char* opts);
	/**
	 * For processing options  
	 * TODO
	 * @param arg 
	 * @param iso 
	 * @return int 
	 */
	int process_gopt(const char* arg, int iso);
	/**
	 * Used to handle whether the flag bits set by the option are available
	 * 
	 * @param opt The operation that needs to be identified
	 * @param sep 
	 * @param arg 
	 * @param iso 
	 * @return int Return 0 if correctly identified, -1 otherwise
	 */
	int process_opt(const copper_fuse_opt* opt, unsigned sep, const char* arg, int iso);
	int process_opt_sep_arg(const copper_fuse_opt* opt, unsigned sep, const char* arg, int iso);
public:
	int opt_parse(void);
	int add_opt(const char* opt);
	int add_arg(const char* arg);
	int next_arg(const char* arg);
	int insert_arg(int pos, const char* arg);
	int call_proc(const char* arg, int key, int iso);
};

#endif //! __COPPER_FUSE_OPT_H__