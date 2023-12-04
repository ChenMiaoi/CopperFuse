#include "copper_fuse_opt.h"
#include "copper_fuse_lowlevel.h"
#include "copper_fuse_mnt_util.h"
#include "copper_log.h"
#include <cerrno>
#include <cstdio>
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
 * FOR COPPER FUSE OPT
 * ---------------------------------------------------*/

/**
 * Match each of the given templates in the `copper_fuse_opt array` that was set initially
 * 
 * @param t given template
 * @param arg template parameters passed in
 * @param sepp 
 * @return int If t and arg match, then return 1, otherwise return 0
 */
static int match_template(const char* t, const char* arg, unsigned* sepp) {
	info << "template = " << t;
	info << "need to match arg = " << arg;
	int arg_len = strlen(arg);

	/* If the value is of the `--xxx=x type`, check whether there is `a space` after `=` */
	const char* sep = strchr(t, '=');
	sep = sep ? sep : strchr(t, ' ');

	/* Here is the type of the judgment form `--xxx=%s` */
	if (sep && (!sep[1] || sep[2] == '%')) {
		int t_len = sep - t;
		if (sep[0] == '=') t_len++;
		if (arg_len >= t_len && strncmp(arg, t, t_len) == 0) {
			*sepp = sep - t;
			return 1;
		}
	}

	/* Here the judgment looks like the `-h` or `--help` argument */
	if (strcmp(t, arg) == 0) {
		*sepp = 0;
		return 1;
	}
	return 0;
}

const copper_fuse_opt* copper_fuse_opt::find_opt(const char *arg, unsigned int *sepp) const {
	for (const copper_fuse_opt* opt = this; opt && opt->templ; opt++) 
		if (match_template(opt->templ, arg, sepp))
			return opt;
	return nullptr;
}

/** ---------------------------------------------------
 * FOR COPPER FUSE ARGS
 * ---------------------------------------------------*/

copper_fuse_args::copper_fuse_args(int _argc, char** _argv)
	: argc(_argc), argv(_argv), allocated(0) {}

std::ostream& operator<< (std::ostream& _out, const copper_fuse_args& args) {
	_out << "args {" << "\n"
		<< "\targc = " << args.argc << "\n"
		<< "\targv [ ";
	for (int i = 0; i < args.argc; i++) {
		_out << args.argv[i] << ", ";
	}
	_out << " ]" << "\n"
		<< "\tallocated = " << args.allocated << "\n}";
	return _out;
}

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
	info;

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
		/* The actual operation is in outargs, which is also the output parameter */
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

	/* Reapply for two more Spaces than the original, because the joined and one NULL */
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

int copper_fuse_args::insert_arg(int pos, const char *arg) {
	if (pos > argc) {
		erron << "pos oversize argc";
		exit(-1);
	}
	if (add_arg(arg) == -1)	return -1;

	if (pos != argc - 1) {
		char* new_arg = argv[argc - 1];
		memmove(&argv[pos + 1], &argv[pos], sizeof(char*) * (argc - pos - 1));
		argv[pos] = new_arg;
	}
	return 0;
}

/** ---------------------------------------------------
 * FOR COPPER FUSE OPT CONTEXT
 * ---------------------------------------------------*/

int copper_fuse_opt_context::opt_parse() {
	/* if argc != 0, args will add a new arg */
	if (argc) {
		info << "outargs add " << argv[0];
		if (outargs.add_arg(argv[0]) == -1) 
			return -1;
		info << "outargs result = " << outargs;
	}

	for (argctr = 1; argctr < argc; argctr++) {
		/* process each parameter */
		if (process_one(argv[argctr]) == -1)
			return -1;
	}

	if (opts) {
		if (insert_arg(1, "-o") == -1 || insert_arg(2, opts) == -1)
			return -1;
	}

	if (nonopt && nonopt == outargs.argc && 
		strcmp(outargs.argv[outargs.argc - 1], "--") == 0) {
		free(outargs.argv[outargs.argc - 1]);
		outargs.argv[--outargs.argc] = nullptr;
	}

	return 0;
}

int copper_fuse_opt_context::process_one(const char *arg) {
	info << "need to process: " << arg;
	if (nonopt || arg[0] != '-')
		/* No options, just follow the executable command */
		return call_proc(arg, COPPER_FUSE_OPT::KEY_NONOPT, 0);
	else if (arg[1] == 'o') {	/* Options such as "-o" */
		/* if Option like "-ofoo" -> "-o" and "foo" */
		if (arg[2]) return process_option_group(arg + 2);
		else {
			/* If have "-arg" after "-o" */
			if (next_arg(arg + 2) == -1) return -1;
			return process_option_group(argv[argctr]);
		}
	} else if (arg[1] == '-' && !arg[2]) {
		if (add_arg(arg) == -1)
			return -1;
		nonopt = outargs.argc;
		return 0;
	} else 
		/* Work with abbreviations like "-h", the single option and single char */
		return process_gopt(arg, 0);
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

int copper_fuse_opt_context::process_real_option_group(char *opts) {
	char* s = opts;
	char* d = s;
	int end = 0;

	while (!end) {
		if (*s == '\0') end = 1;
		if (*s == ',' || end) {
			int res;
			
			*d = '\0';
			res = process_gopt(opts, 1);
			if (res == -1) return -1;
			d = opts;
		} else {
			if (s[0] == '\\' && s[1] != '\0') {
				s++;
				if (s[0] >= '0' && s[0] <= '3' &&
					s[1] >= '0' && s[1] <= '7' &&
					s[2] >= '0' && s[2] <= '7') {
					*d++ = (s[0] - '0') * 0100 + 
							(s[1] - '0') * 0010 + 
							(s[2] - '0');
					s += 2;
				} else {
					*d++ = *s;
				}
			} else {
				*d++ = *s;
			}
		}
		s++;
	}
	return 0;
}

int copper_fuse_opt_context::process_gopt(const char *arg, int iso) {
	unsigned sep;
	const copper_fuse_opt* opt = this->opt->find_opt(arg, &sep);

	/* If the correct option exists */
	if (opt) {
		for (; opt; opt = (opt + 1)->find_opt(arg, &sep)) {
			int res;
			if (sep && opt->templ[sep] == ' ' && !arg[sep])
				res = process_opt_sep_arg(opt, sep, arg, iso);
			else 
				res = process_opt(opt, sep, arg, iso);
			if (res == -1)
				return -1;
		}
		return 0;
	} else 
		return call_proc(arg, COPPER_FUSE_OPT::KEY_OPT, iso);
}

static int process_opt_param(void* var, const char* format, const char* param, const char* arg) {
	if (format[0] != '%') {
		erron << "format error";
		exit(-1);
	}
	if (format[1] == 's') {
		char** s = (char**)var;
		char* copy = strdup(param);
		if (!copy) {
			erron << "memory allocation failed";
			return -1;
		}
		free(*s);
		*s = copy;
	} else {
		if (sscanf(param, format, var) != 1) {
			erron << "invalid parameter in option `" << arg << "`";
			return -1;
		}
	}
	return 0;
}

int copper_fuse_opt_context::process_opt(const copper_fuse_opt *opt, unsigned int sep, const char *arg, int iso) {
	if (opt->offset == -1U) {
		if (call_proc(arg, opt->value, iso) == -1)
			return -1;
	} else {
		/** To set whether the parameter in the option is available, 
		 * use sep to find the offset position of the corresponding parameter 
		 * in the option structure 
		 */
		void* var = static_cast<char*>(data) + opt->offset;
		if (sep && opt->templ[sep + 1]) {
			const char* param = arg + sep;
			if (opt->templ[sep] == '=') param++;
			if (process_opt_param(var, opt->templ + sep + 1, param, arg) == -1)
				return -1;
		} else /* Here we handle situations like -h or --help */
			*(int*)var = opt->value;
	}
	return 0;
}

int copper_fuse_opt_context::process_opt_sep_arg(const copper_fuse_opt *opt, unsigned int sep, const char *arg, int iso) {
	int res;
	char* new_arg;
	char* param;

	if (next_arg(arg) == -1) return -1;

	param = argv[argctr];
	new_arg = (char*)malloc(sep + strlen(param) + 1);
	if (!new_arg) {
		erron << "memory allocation failed";
		return -1;
	}
	memcpy(new_arg, arg, sep);
	strcpy(new_arg + sep, param);
	res = process_opt(opt, sep, arg, iso);
	free(new_arg);

	return res;
}

int copper_fuse_opt_context::next_arg(const char *arg) {
	if (argctr + 1 >= argc) {
		erron << "missing argument after `" << opt << "`";
		return -1;
	}
	argctr++;
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

int copper_fuse_opt_context::insert_arg(int pos, const char *arg) {
	return outargs.insert_arg(pos, arg);
}