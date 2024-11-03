#define FUSE_USE_VERSION 31

#include <string.h>
#include <chrono>
#include "metric_collection.hpp"
#include <unordered_set>

static fuse_lowlevel_ops *metric_next;
static nostd::unique_ptr<metric_api::Counter<uint64_t>> readCounter;
static nostd::unique_ptr<metric_api::Counter<uint64_t>> writeCounter;
static nostd::unique_ptr<metric_api::Histogram<double>> readLatencyHistogram;
static nostd::unique_ptr<metric_api::Histogram<double>> writeLatencyHistogram;
static nostd::unique_ptr<metric_api::UpDownCounter<int64_t>> dirCounter;

// Low-level file operations

static void metrics_init(void *userdata, struct fuse_conn_info *conn)
{
	initMetrics();
	
	// Add instruments
	readCounter = getCounter("read_counter");
	writeCounter = getCounter("write_counter");
	readLatencyHistogram = getHistogram("read_latency_histogram", "Latency distribution of read file operation", "microseconds");
	writeLatencyHistogram = getHistogram("write_latency_histogram", "Latency distribution of write file operation", "microseconds");
	dirCounter = getUpDownCounter("directory_counter", "Number of directories created or deleted", "directories");	

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

	auto start = std::chrono::high_resolution_clock::now();
	metric_next->read(req, ino, size, off, fi);
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float, std::micro> latency = end - start;
	auto context = context::Context{}; 
	readLatencyHistogram->Record(latency.count(), context);

}

static void metrics_write_buf(fuse_req_t req, fuse_ino_t ino, fuse_bufvec *in_buf,
	off_t off, fuse_file_info *fi) 
{
	// Add total # bytes being written
	size_t totalBytes = 0;
	for (int i = 0; i < in_buf->count; i++) {
		totalBytes += in_buf->buf[i].size;
	}
	writeCounter->Add(totalBytes);

	// Add latency to histogram
	auto start = std::chrono::high_resolution_clock::now();
	metric_next->write_buf(req, ino, in_buf, off, fi);
	auto end = std::chrono::high_resolution_clock::now();

	std::chrono::duration<float,std::micro> latency = end - start;
	auto context = context::Context{}; 
	writeLatencyHistogram->Record(latency.count(), context);
}

static void metrics_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name,
    mode_t mode) {
	auto context = context::Context{}; 
	dirCounter->Add(1, context);
	
	metric_next->mkdir(req, parent, name, mode);
}

static void metrics_rmdir(fuse_req_t req, fuse_ino_t parent, const char *name) {
	
	auto context = context::Context{}; 
	dirCounter->Add(-1, context);

	metric_next->rmdir(req, parent, name);
}

fuse_lowlevel_ops metric_operations(fuse_lowlevel_ops &next) {
	metric_next = &next;

	fuse_lowlevel_ops curr = next;
	
	curr.init = metrics_init;
	curr.destroy = metrics_destroy;
	
	curr.read = metrics_read;
	curr.write_buf = metrics_write_buf;
	
	curr.mkdir = metrics_mkdir;
	curr.rmdir = metrics_rmdir;

	return curr;
}
