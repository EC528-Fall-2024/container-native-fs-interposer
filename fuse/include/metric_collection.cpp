#define FUSE_USE_VERSION 31

#include <string.h>
#include <chrono>
#include "metric_collection.hpp"

static fuse_lowlevel_ops *metric_next;
static nostd::unique_ptr<metric_api::Counter<uint64_t>> readCounter;
static nostd::unique_ptr<metric_api::Counter<uint64_t>> writeCounter;
static nostd::unique_ptr<metric_api::Histogram<double>> latencyHistogram;

// Low-level file operations

static void metrics_init(void *userdata, struct fuse_conn_info *conn)
{
	initMetrics();
	
	// Add instruments
	readCounter = getCounter("read_counter");
	writeCounter = getCounter("write_counter");
	latencyHistogram = getHistogram("latency_histogram", "Latency distribution of each file operation", "ms");

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

static void metrics_write_buf(fuse_req_t req, fuse_ino_t ino, fuse_bufvec *in_buf,
	off_t off, fuse_file_info *fi) 
{
	size_t totalBytes = 0;
	for (int i = 0; i < in_buf->count; i++) {
		totalBytes += in_buf->buf[i].size;
	}
	writeCounter->Add(totalBytes);

	auto start = std::chrono::high_resolution_clock::now();
	metric_next->write_buf(req, ino, in_buf, off, fi);
	auto end = std::chrono::high_resolution_clock::now();
	auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	auto context = context::Context{}; 
	latencyHistogram->Record(latency, context);
}

fuse_lowlevel_ops metric_operations(fuse_lowlevel_ops &next) {
    metric_next = &next;

    fuse_lowlevel_ops curr = next;
    curr.init = metrics_init;
    curr.destroy = metrics_destroy;
    curr.read = metrics_read;
    curr.write_buf = metrics_write_buf;

    return curr;
}
