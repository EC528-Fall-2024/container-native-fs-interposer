#define FUSE_USE_VERSION 31

#include <string.h>
#include "metric_collection.hpp"

static fuse_lowlevel_ops *metric_next;

// Low-level file operations

static void metrics_init(void *userdata, struct fuse_conn_info *conn)
{
	initMetrics();
	metric_next->init(userdata, conn);
}

static void metrics_destroy(void *userdata) {
	cleanupMetrics();
	metric_next->destroy(userdata);
}


fuse_lowlevel_ops metric_operations(fuse_lowlevel_ops &next) {
    metric_next = &next;

    fuse_lowlevel_ops curr = next;
    curr.init = metrics_init;
    curr.destroy = metrics_destroy;

    return curr;
}