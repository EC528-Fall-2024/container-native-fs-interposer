#define FUSE_USE_VERSION FUSE_MAKE_VERSION(3, 12)

#include <fuse_lowlevel.h>
#include "include/passthrough_hp.hpp"
#include "include/workload_tracing.hpp"
#include "include/metric_collection.hpp"


#include "include/otel.hpp"
int main(int argc, char *argv[]) {
	
    // Passthrough operations
    fuse_lowlevel_ops oper {};
    assign_operations(oper);

    // Utility layers
    fuse_lowlevel_ops tracing_oper = tracing_operations(oper);
    fuse_lowlevel_ops metric_oper = metric_operations(tracing_oper);

    return setup_fuse(argc, argv, metric_oper);
}

