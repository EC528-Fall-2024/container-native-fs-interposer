#define FUSE_USE_VERSION FUSE_MAKE_VERSION(3, 12)

#include <fuse_lowlevel.h>
#include "include/passthrough_hp.hpp"
#include "include/workload_tracing.hpp"

int main(int argc, char *argv[]) {
    // Passthrough operations
    fuse_lowlevel_ops oper {};
    assign_operations(oper);

    // Workload tracing
    fuse_lowlevel_ops tracing_oper = tracing_operations(oper);

    return setup_fuse(argc, argv, tracing_oper);
}

