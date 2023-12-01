#include "copper_fuse.h"
#include "copper_fuse_opt.h"
#include "copper_fuse_common.h"
#include "copper_fuse_lowlevel.h"

#include <cstddef>
#include <cstring>
#include <limits>
#include <thread>

int copper_fuse_main_real(int argc, char* argv[], 
	const struct copper_fuse_operations* op, size_t op_size, void* user_data) {
	copper_fuse_args args(argc, argv);
	copper_fuse* fuse;
	copper_fuse_cmdline_opts opts;
	copper_fuse_loop_config* loop_config = nullptr;

	int ret = 0;
	if (args.parse_cmdline(&opts) != 0) {
		// TODO
	}
	return {};
}