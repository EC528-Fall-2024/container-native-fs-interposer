#define FUSE_USE_VERSION 31

#include <string.h>
#include "metric_collection.hpp"

static fuse_lowlevel_ops *metric_next;
static nostd::unique_ptr<metric_api::Counter<uint64_t>> readCounter;
// Low-level file operations

static void metrics_init(void *userdata, struct fuse_conn_info *conn)
{
	initMetrics();
	readCounter = getReadCounter();
	metric_next->init(userdata, conn);
}

static void metrics_destroy(void *userdata) {
	cleanupMetrics();
	metric_next->destroy(userdata);
}

static void metrics_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
	struct fuse_file_info *fi) 
{
    readCounter->Add(size);
    metric_next->read(req, ino, size, off, fi);
}


fuse_lowlevel_ops metric_operations(fuse_lowlevel_ops &next) {
    metric_next = &next;

    fuse_lowlevel_ops curr = next;
    curr.init = metrics_init;
    curr.destroy = metrics_destroy;
    curr.read = metrics_read;
    return curr;
}
