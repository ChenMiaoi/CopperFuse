#include "copper_fuse_lowlevel.h"
#include "copper_log.h"
#include <cstdlib>
#include <cstring>

/** ---------------------------------------------------
 * FOR COPPER FUSE CMDLINE OPT
 * ---------------------------------------------------*/

int add_opt_common(char** opts, const char* opt, int esc) {
	unsigned old_len = *opts ? strlen(*opts) : 0;
	char* d = (char*)realloc(*opts, old_len + 1 + strlen(opt) * 2 + 1);

	if (!d) {
		erron << "memory allocation error!";
		return -1;
	}

	*opts = d;
	if (old_len) {
		d += old_len;
		*d++ = ',';
	}

	for (; *opt; opt++) {
		if (esc && (*opt == ',' || *opt == '\\'))
			*d++ = '\\';
		*d++ = *opt;
	}
	*d = '\0';

	return 0;
}

int copper_fuse_cmdline_opts::add_opt(const char *opt) {
	return add_opt_common(&mountpoint, opt, 0);
}