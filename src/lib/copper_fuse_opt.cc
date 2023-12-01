#include "copper_fuse_opt.h"
#include "copper_fuse_lowlevel.h"
#include "copper_fuse_mnt_util.h"
#include "copper_log.h"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <variant>

#define PATH_MAX 64

#define FUSE_HELPER_OPT(t, p)	\
		{ t, offsetof(copper_fuse_cmdline_opts, p), 1 }

static const struct copper_fuse_opt copper_fuse_helper_opts[] = {
	FUSE_HELPER_OPT("-h", show_help),
	FUSE_HELPER_OPT("--help", show_help),
};

/** ---------------------------------------------------
 * FOR COPPER FUSE ARGS
 * ---------------------------------------------------*/

copper_fuse_args::copper_fuse_args(int _argc, char** _argv)
	: argc(_argc), argv(_argv), allocated(0) {}

static int copper_fuse_helper_opt_proc(void* data, const char* arg, int key, copper_fuse_args* outargs) {
	static_cast<void>(outargs);
	copper_fuse_cmdline_opts* opts = static_cast<copper_fuse_cmdline_opts*>(data);

	switch (key) {
	case COPPER_FUSE_OPT::KEY_NONOPT:
		if (!opts->mountpoint) {
			if (copper_fuse_mnt_parse_fd(arg) != -1) {
				return opts->add_opt(arg);
			}

			char mountpoint[PATH_MAX] = "";
			if (realpath(arg, mountpoint) == nullptr) {
				erron << "bad mount point `" << arg << "`: " << strerror(errno);
				return -1;
			}
			return opts->add_opt(arg);
		} else {
			erron << "invalid argument `" << arg << "`";
			return -1;
		}
	default:
		/* Pass through unknown options */
		return 1;
	}
	return 0;
}

static int add_default_subtype(const char* progname, struct copper_fuse_args* args) {
	// TODO
	return 0;
}

int copper_fuse_args::parse_cmdline(struct copper_fuse_cmdline_opts *opts) {
	memset(opts, 0, sizeof(copper_fuse_cmdline_opts));

	opts->max_idle_threads = std::numeric_limits<unsigned int>::max();
	opts->max_threads = std::thread::hardware_concurrency();

	if (parse_opt(opts, copper_fuse_helper_opts, copper_fuse_helper_opt_proc) == -1) 
		return -1;

	if (!opts->nodefault_subtype)
		if (add_default_subtype(argv[0], this) == -1)
			return -1;

	return 0;
}

int copper_fuse_args::parse_opt(void *data, const copper_fuse_opt *opts, copper_fuse_opt_proc_t proc) {
	int res = 0;
	copper_fuse_opt_context ctx = {
		.data = data,
		.opt  = opts,
		.proc = proc
	};

	if (!argv || !argc) return 0;

	ctx.argc = argc;
	ctx.argv = argv;

	res = ctx.opt_parse();
	if (res != -1) {
		std::swap(*this, ctx.outargs);
	}
	free(ctx.opts);

	return res;
}

int copper_fuse_args::add_arg(const char *arg) {
	char** newargv;
	char*  newarg;

	if (argv && !allocated) {
		erron << "add_arg";
		exit(-1);
	}

	newarg = strdup(arg);
	if (!newarg) {
		erron << "memory allocation failed\n";
		return -1;
	}

	newargv = (char**)realloc(argv, (argc + 2) * sizeof(char*));
	if (!newargv) {
		free(newarg);
		erron << "memory allocation failed\n";
		return -1;
	}

	argv 		= newargv;
	allocated 	= 1;
	argv[argc++] = newarg;
	argv[argc] 	 = nullptr;

	return 0;
}

/** ---------------------------------------------------
 * FOR COPPER FUSE OPT CONTEXT
 * ---------------------------------------------------*/

int copper_fuse_opt_context::opt_parse() {
	if (argc) {
		if (outargs.add_arg(argv[0]) == -1) 
			return -1;
	}

	for (argctr = 1; argctr < argc; argctr++) {
		if (process_one(argv[argctr]) == -1)
			return -1;
	}

	// TODO
	return 0;
}

int copper_fuse_opt_context::process_one(const char *arg) {
	if (nonopt || arg[0] != '-')
		return call_proc(arg, COPPER_FUSE_OPT::KEY_NONOPT, 0);
	else if (arg[1] == 'o') {
		if (arg[2]) return process_option_group(arg + 2);
		return {};
	} else if (arg[1] == '-' && !arg[2]) {
		// TODO
		return 0;
	} else 
		return {};
		// return process_gopt(arg, 0);
}

int copper_fuse_opt_context::process_option_group(const char *opts) {
	int res = 0;
	char* copy = strdup(opts);

	if (!copy) {
		erron << "memory allocation failed!";
		return -1;
	}
	res = process_real_option_group(copy);
	free(copy);

	return res;
}

int copper_fuse_opt_context::process_real_option_group(const char *opts) {
	// TODO
	return 0;
}

int copper_fuse_opt_context::call_proc(const char *arg, int key, int iso) {
	if (key == COPPER_FUSE_OPT::KEY_DISCARD)
		return 0;

	if (key != COPPER_FUSE_OPT::KEY_KEEP && proc) {
		int res = proc(data, arg, key, &outargs);
		if (res == -1 || !res) return res;
	}
	if (iso) return add_opt(arg); 
	else 	 return add_arg(arg);
}



int copper_fuse_opt_context::add_opt(const char *opt) {
	return add_opt_common(&opts, opt, 1);
}

int copper_fuse_opt_context::add_arg(const char *arg) {
	return outargs.add_arg(arg);
}