#define FUSE_USE_VERSION FUSE_MAKE_VERSION(3, 12)

#include <fuse_lowlevel.h>
#include "include/passthrough_hp.hpp"
#include "include/workload_tracing.hpp"
#include "include/metric_collection.hpp"
#include "include/otel.hpp"
#include "include/config_parser.hpp"

int main(int argc, char *argv[]) {
    // Passthrough operations
    fuse_lowlevel_ops oper {};
    assign_operations(oper);
    fuse_lowlevel_ops *lastLayer = &oper;

    // Get configuration
    json config = getConfig("./config/config.json");
    if (config == NULL) return 1;

    bool addMetrics = config.at("metrics");
    bool addTraces = config.at("traces");
    bool addFaultyIO = config.at("faultyIO");
    bool addThrottleIO = config.at("throttleIO");
    bool addFakeIO = config.at("fakeIO");
    
    // Utility layers
    fuse_lowlevel_ops metric_oper;
    if (addMetrics) {
        metric_oper = metric_operations(*lastLayer);
        lastLayer = &metric_oper;        
    }

    fuse_lowlevel_ops tracing_oper;
    if (addTraces) {
        tracing_oper = tracing_operations(*lastLayer);
        lastLayer = &tracing_oper;
    }

    return setup_fuse(argc, argv, *lastLayer);
}

