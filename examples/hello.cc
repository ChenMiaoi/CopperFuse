#include "copper_fuse.h"
#include "copper_fuse_common.h"
#include "copper_fuse_opt.h"
#include "copper_log.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <variant>

struct options {
    int show_help;
    std::string filename;
    std::string contents;
} op;

#define OPTION(t, p)    \
    { t, offsetof(options, p), 1 }

static const copper_fuse_opt option_spec[] = {
    OPTION("--name=%s", filename),
    OPTION("--contents=%s", contents),
    OPTION("-h", show_help),
    OPTION("--help", show_help),
    COPPER_FUSE_OPT_END
};

static void *hello_init(copper_fuse_conn_info* conn, copper_fuse_config *cfg) {
    static_cast<void>(conn);
    cfg->kernel_cache = 1;
    return nullptr;
}

static copper_fuse_operations hello_oper = {
    // .init = [](copper_fuse_conn_info* conn, copper_fuse_config* cfg) -> void* {},
    .init = hello_init,
};

static void show_help(const char* progname) {
    printf("usage: %s [options] <mountpoint>\n\n", progname);
	printf("File-system specific options:\n"
	       "    --name=<s>          Name of the \"hello\" file\n"
	       "                        (default: \"hello\")\n"
	       "    --contents=<s>      Contents \"hello\" file\n"
	       "                        (default \"Hello, World!\\n\")\n"
	       "\n");
}

int main(int argc, char* argv[]) {
    info << "argc = " << argc;
    info << "argv = " << argv[0] << " " << argv[1];
    
    int ret;
    copper_fuse_args args(argc, argv);

    op.filename = std::string(strdup("hello"));
    op.contents = std::string(strdup("Hello World\n"));
    info << "filename = " << op.filename;
    info << "contents = " << op.contents;

    if (args.parse_opt(&op, option_spec, nullptr) == -1) return 1;

    if (op.show_help) { 
        show_help(argv[0]);
        if (args.add_arg("--help") != 0) {
            erron << "add_arg failed";
            exit(-1);
        }
        info << "arg add = " << args;
        args.argv[0][0] = '\0';
    }

    ret = copper_fuse_main(args.argc, args.argv, &hello_oper, nullptr);

    // TODO args.out_free_args();
    return 0;
}