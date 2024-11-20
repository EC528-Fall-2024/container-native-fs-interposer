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

    bool addMetrics = config.contains("metrics") && config["metrics"].contains("enabled") && config["metrics"]["enabled"];
    bool addTraces = config.contains("traces") && config["traces"].contains("enabled") && config["traces"]["enabled"];
    bool addFaultyIO = config.contains("faultyIO") && config["faultyIO"].contains("enabled") && config["faultyIO"]["enabled"];
    bool addThrottleIO = config.contains("throttleIO") && config["throttleIO"].contains("enabled") && config["throttleIO"]["enabled"];
    bool addFakeIO = config.contains("fakeIO") && config["fakeIO"].contains("enabled") && config["fakeIO"]["enabled"];

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

