#define FUSE_USE_VERSION 31

#include <string.h>
#include <chrono>
#include "metric_collection.hpp"
#include <unordered_set>
#include "./config_parser.hpp"

bool addReadCounter = false;
bool addWriteCounter = false;
bool addReadLatencyHist = false;
bool addWriteLatencyHist = false;
bool addDirCounter = false;

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
	if (addReadCounter) {
        readCounter = getCounter("read_counter");
    }
	if (addWriteCounter) {
        writeCounter = getCounter("write_counter");
    }
    if (addReadLatencyHist) {
	    readLatencyHistogram = getHistogram("read_latency_histogram", "Latency distribution of read file operation", "microseconds");
	}
    if (addWriteLatencyHist) {
        writeLatencyHistogram = getHistogram("write_latency_histogram", "Latency distribution of write file operation", "microseconds");
	} 
    if (addDirCounter) {
        dirCounter = getUpDownCounter("directory_counter", "Number of directories created or deleted", "directories");	
    }
	metric_next->init(userdata, conn);
}

static void metrics_destroy(void *userdata) {
	cleanupMetrics();
	metric_next->destroy(userdata);
}

static void metrics_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
	struct fuse_file_info *fi) 
{
    if (addReadCounter) {
        readCounter->Add(size);
    }

	auto start = std::chrono::high_resolution_clock::now();
	metric_next->read(req, ino, size, off, fi);
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float, std::micro> latency = end - start;
	auto context = context::Context{}; 
	
    if (addReadLatencyHist) {
        readLatencyHistogram->Record(latency.count(), context);
    }
}

static void metrics_write_buf(fuse_req_t req, fuse_ino_t ino, fuse_bufvec *in_buf,
	off_t off, fuse_file_info *fi) 
{
	// Add total # bytes being written
	size_t totalBytes = 0;
	for (int i = 0; i < in_buf->count; i++) {
		totalBytes += in_buf->buf[i].size;
	}
    if (addWriteCounter) {
    	writeCounter->Add(totalBytes);
    }

	// Add latency to histogram
	auto start = std::chrono::high_resolution_clock::now();
	metric_next->write_buf(req, ino, in_buf, off, fi);
	auto end = std::chrono::high_resolution_clock::now();

	std::chrono::duration<float,std::micro> latency = end - start;
	auto context = context::Context{}; 
	if (addWriteLatencyHist) {
        writeLatencyHistogram->Record(latency.count(), context);
    }
}

static void metrics_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name,
    mode_t mode) {
	auto context = context::Context{}; 
    
    if (addDirCounter) {	
        dirCounter->Add(1, context);
	}

	metric_next->mkdir(req, parent, name, mode);
}

static void metrics_rmdir(fuse_req_t req, fuse_ino_t parent, const char *name) {
	
	auto context = context::Context{}; 
    if (addDirCounter) {
    	dirCounter->Add(-1, context);
    }

	metric_next->rmdir(req, parent, name);
}

fuse_lowlevel_ops metric_operations(fuse_lowlevel_ops &next) {
    // Configs
    json config = getConfig("./config/config.json");
    if (config.contains("metrics")) {
        auto configMetrics = config["metrics"];
        addReadCounter = configMetrics.contains("readCounter") && configMetrics["readCounter"];
        addWriteCounter = configMetrics.contains("writeCounter") && configMetrics["writeCounter"];
        addReadLatencyHist = configMetrics.contains("readLatencyHist") && configMetrics["readLatencyHist"];
        addWriteLatencyHist = configMetrics.contains("writeLatencyHist") && configMetrics["writeLatencyHist"];
        addDirCounter = configMetrics.contains("dirCounter") && configMetrics["dirCounter"];
    }


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
