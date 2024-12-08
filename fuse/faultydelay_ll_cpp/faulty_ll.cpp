/*
  FUSE: Fault Injection Filesystem in Userspace; Hilario Gonzalez Fall 2024
  Adapted from fuse lowlevel passthrough fs: Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  
	In the functions lo_read(), lo_write_buf(), lo_flush(), lo_do_readdir(), lo_open(), lo_opendir() I introduced randomized IO errors, data truncation, and delays.
	Several configurable parameters can be adjusted in config file to customize fs performance.
	I also created a helper function to write to a log that will track when these errors have occured, so that we have a concrete artifact to compare against that documented all the errors

	g++ -Wall -D_FILE_OFFSET_BITS=64 faulty_ll.cpp include/config_parser.cpp -I/usr/local/include -I./include -L/usr/local/lib -lopentelemetry_trace -lopentelemetry_resources -lopentelemetry_exporter_ostream_span -lopentelemetry_common `pkg-config fuse3 --cflags --libs` -o faulty_ll
	docker build -t my-fuse-app .
	docker images //list images
	docker rmi <image_name> //remove image
	docker run -it --privileged -v /dev/fuse:/dev/fuse --name fuse-container  my-fuse-app bash
	docker create --privileged -v /dev/fuse:/dev/fuse --name fuse-container my-fuse-app
	docker start fuse-container
	docker exec -it fuse-container sh
	docker stop fuse-container && docker rm fuse-container
	//then mount to mountpoint and cd until testmount to test IO, errors logged on local text file in mountpoint/

	//Kube commands
	kubectl apply -f fuse-faulty.yaml
	kubectl get pods
	kubectl logs fuse-faulty
	kubectl exec -it fuse-faulty -- /bin/bash  ##use this to get a bash shell in the container
	kubectl delete pod fuse-pod ##use this to clean up

*/

/* Openttelemetry in another cmake possibly?
# CMakeLists.txt
find_package(opentelemetry-cpp CONFIG REQUIRED)
...
target_include_directories(foo PRIVATE ${OPENTELEMETRY_CPP_INCLUDE_DIRS})
target_link_libraries(foo PRIVATE ${OPENTELEMETRY_CPP_LIBRARIES}
*/

/** @file
 *
 * This file system injects IO faults into the existing file system hierarchy of the
 * system, starting at the root file system. This is implemented by
 * just "passing through" all requests to the corresponding user-space
 * libc functions, after simulating faults. This implementation uses the low-level API.
 *
 * When writeback caching is enabled (-o writeback mount option), it
 * is only possible to write to files for which the mounting user has
 * read permissions. This is because the writeback cache requires the
 * kernel to be able to issue read requests for all files (which the
 * passthrough filesystem cannot satisfy if it can't read the file in
 * the underlying filesystem).
 *
 * Compile with:
 *
 * 	   g++ -Wall -D_FILE_OFFSET_BITS=64 faulty_ll.cpp include/config_parser.cpp -I/usr/local/include -I./include -L/usr/local/lib -lopentelemetry_trace -lopentelemetry_resources -lopentelemetry_exporter_ostream_span -lopentelemetry_common `pkg-config fuse3 --cflags --libs` -o faulty_ll
 */

#define FUSE_USE_VERSION FUSE_MAKE_VERSION(3, 16)

#include <fuse_lowlevel.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <stddef.h>
#include <stdbool.h>
#include <cstring>
#include <fstream>
#include <limits.h>
#include <dirent.h>
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <sys/file.h>
#include <sys/xattr.h>
#include <time.h>
#include <iostream>
#include <stdarg.h>
#include "./include/config_parser.hpp"

#include "libfuse/passthrough_helpers.h"
#include "faulty_ll.hpp"

//#include <opentelemetry/exporters/otlp/otlp_grpc_exporter.h>
//#include <opentelemetry/exporters/otlp/otlp_grpc_exporter_options.h>

#include <opentelemetry/exporters/ostream/span_exporter_factory.h>
#include <opentelemetry/exporters/ostream/span_exporter.h>
#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/sdk/resource/semantic_conventions.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/sdk/trace/exporter.h>
#include <opentelemetry/sdk/trace/processor.h>
#include <opentelemetry/sdk/trace/simple_processor_factory.h>
#include <opentelemetry/sdk/trace/simple_processor.h>
#include <opentelemetry/sdk/trace/batch_span_processor.h>

#include <opentelemetry/sdk/trace/tracer_provider_factory.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/sdk/trace/tracer.h>
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/scope.h>
#include <opentelemetry/trace/span.h>
#include <opentelemetry/trace/tracer_provider.h>

#include <opentelemetry/sdk/logs/logger_provider.h>
#include <opentelemetry/logs/provider.h>
#include <opentelemetry/logs/logger.h>

#include <chrono>
#include <iomanip>
#include <ctime>
#include <sstream>

/* We are re-using pointers to our `struct lo_inode` and `struct
   lo_dirp` elements as inodes. This means that we must be able to
   store uintptr_t values in a fuse_ino_t variable. The following
   incantation checks this condition at compile time. */
#if defined(__GNUC__) && (__GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 6) && !defined __cplusplus
_Static_assert(sizeof(fuse_ino_t) >= sizeof(uintptr_t),
	       "fuse_ino_t too small to hold uintptr_t values!");
#else
struct _uintptr_to_must_hold_fuse_ino_t_dummy_struct \
	{ unsigned _uintptr_to_must_hold_fuse_ino_t:
			((sizeof(fuse_ino_t) >= sizeof(uintptr_t)) ? 1 : -1); };
#endif

struct lo_inode {
	struct lo_inode *next; /* protected by lo->mutex */
	struct lo_inode *prev; /* protected by lo->mutex */
	int fd;
	ino_t ino;
	dev_t dev;
	uint64_t refcount; /* protected by lo->mutex */
};

enum {
	CACHE_NEVER,
	CACHE_NORMAL,
	CACHE_ALWAYS,
};

struct lo_data {
	pthread_mutex_t mutex;
	int debug;
	int writeback;
	int flock;
	int xattr;
	char *source;
	double timeout;
	int cache;
	int timeout_set;
	struct lo_inode root; /* protected by lo->mutex */
};

static const struct fuse_opt lo_opts[] = {
	{ "writeback",
	  offsetof(struct lo_data, writeback), 1 },
	{ "no_writeback",
	  offsetof(struct lo_data, writeback), 0 },
	{ "source=%s",
	  offsetof(struct lo_data, source), 0 },
	{ "flock",
	  offsetof(struct lo_data, flock), 1 },
	{ "no_flock",
	  offsetof(struct lo_data, flock), 0 },
	{ "xattr",
	  offsetof(struct lo_data, xattr), 1 },
	{ "no_xattr",
	  offsetof(struct lo_data, xattr), 0 },
	{ "timeout=%lf",
	  offsetof(struct lo_data, timeout), 0 },
	{ "timeout=",
	  offsetof(struct lo_data, timeout_set), 1 },
	{ "cache=never",
	  offsetof(struct lo_data, cache), CACHE_NEVER },
	{ "cache=auto",
	  offsetof(struct lo_data, cache), CACHE_NORMAL },
	{ "cache=always",
	  offsetof(struct lo_data, cache), CACHE_ALWAYS },

	FUSE_OPT_END
};

static void passthrough_ll_help(void)
{
	std::cout << 
"    -o writeback           Enable writeback\n" <<
"    -o no_writeback        Disable write back\n" <<
"    -o source=/home/dir    Source directory to be mounted\n" <<
"    -o flock               Enable flock\n" <<
"    -o no_flock            Disable flock\n" <<
"    -o xattr               Enable xattr\n" <<
"    -o no_xattr            Disable xattr\n" <<
"    -o timeout=1.0         Caching timeout\n" <<
"    -o timeout=0/1         Timeout is set\n" <<
"    -o cache=never         Disable cache\n" <<
"    -o cache=auto          Auto enable cache\n" <<
"    -o cache=always        Cache always\n" << std::endl;
}

std::string ERRLOGFILE = "usr/src/myapp/testmount/error_log.txt"; //for function calls to the error log
int FFAILRATE = 0; //likelihood of file failure = 1/failrate
int DFAILRATE = 0; //likelihood of directory failure
bool CONFIGSEED = 0; //user can set configseed to one and it will use default value 0 or they can choose their own seed by setting seednum
int SEEDNUM = 0; //default 0 user can change
int DELAYTIME = 3;

static fuse_lowlevel_ops *faulty_next;

// OTEL helper functions:
namespace trace = opentelemetry::trace;
namespace sdktrace = opentelemetry::sdk::trace;
//namespace otlp = opentelemetry::exporter::otlp;
namespace trace_exporter = opentelemetry::exporter::trace;
namespace resource       = opentelemetry::sdk::resource;

namespace logs_api = opentelemetry::logs;
namespace logs_sdk = opentelemetry::sdk::logs;

//global to be flushed elsewhere
std::shared_ptr<std::ofstream> file_handle;

//Otel Setup
void InitTracer(){
    std::string file_path =  "usr/src/myapp/testmount/ostream_out.txt";
    file_handle = std::make_shared<std::ofstream>(file_path.c_str());
    if (!file_handle->is_open()) {
            throw std::runtime_error("Failed to open output file for tracing.");
    }


    //sdktrace::BatchSpanProcessorOptions bspOpts{}; //for use with batch span processor
	auto console_exporter = std::make_unique<trace_exporter::OStreamSpanExporter>(*file_handle);
    //auto processor = std::make_unique<sdktrace::BatchSpanProcessor>(std::move(console_exporter), bspOpts); //replace with processor if want batched
	auto processor = std::make_unique<sdktrace::SimpleSpanProcessor>(std::move(console_exporter));
    
	resource::ResourceAttributes attributes = {
        {resource::SemanticConventions::kServiceName, "fs-faulty-IO"},
        {resource::SemanticConventions::kHostName, "local-host"}
    };
    auto resource = resource::Resource::Create(attributes);
    auto provider = std::make_shared<sdktrace::TracerProvider>(std::move(processor), resource);
    auto nostd_provider = opentelemetry::nostd::shared_ptr<trace::TracerProvider>(provider);

    // Set the global trace provider
    trace::Provider::SetTracerProvider(nostd_provider);
}

/* Initialization for using OTEL GRPC to Jaeger
void InitTracer(){
	//OTLP GRPC Exporter init
	otlp::OtlpGrpcExporterOptions opts;
	opts.endpoint = "localhost:4317"; //need jaeger endpoint
	opts.use_ssl_credentials = true;
	opts.ssl_credentials_cacert_as_string = "ssl-certificate";

	auto exporter = std::make_unique<otlp::OtlpGrpcExporter>(opts);
	auto processor = std::make_unique<sdktrace::SimpleSpanProcessor>(std::move(exporter));
  	
  	resource::ResourceAttributes attributes = {
        {resource::SemanticConventions::kServiceName, "fs-faulty-IO"},
        {resource::SemanticConventions::kHostName, "local-host"}
    };
    auto resource = resource::Resource::Create(attributes);
	auto provider = std::make_shared<sdktrace::TracerProvider>(std::move(processor), resource);
	auto nostd_provider = opentelemetry::nostd::shared_ptr<trace::TracerProvider>(provider);
  	// Set the global trace provider
	trace::Provider::SetTracerProvider(nostd_provider);
  
}
*/

bool otel_is_init = false;
void otel_init(){
    InitTracer();
    otel_is_init = true;
    return;
}

//get timestamp
std::string getCurrentTime(){
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&currentTime), "%Y-%m-%d %H:%M:%S");
    return ss.str();//returns formatted string of timestamp
}

//trace instrumentation
opentelemetry::nostd::shared_ptr<opentelemetry::trace::Span> traceAndSpan(const std::string& whereFault){
    if(!otel_is_init){
        otel_init();
    }
    // Start tracing
    auto tracer = opentelemetry::trace::Provider::GetTracerProvider()->GetTracer("faulty_file_system_tracer");
    return tracer->StartSpan(whereFault); //span is associated with one IO operation which could have more than one fault 
    //use this function by  calling auto span = traceAndSpan("fault_location");
    //after an operation calls this function it must also call "span->End();"
    //span->AddEvent("event_name", {{"key", "value"}});
    //span->SetAttribute("attribute_key", "attribute_value");
}

//Faulty IO helpers:

//Rand seeding logic
bool rand_is_init = false;
void init_random_seed(){
	if(CONFIGSEED){
		if(!rand_is_init){//seed with seednum when mounted
    		srand(static_cast<unsigned>(SEEDNUM));//custom seed if user wants to control/if they want consistency
			rand_is_init = true;
		}
	}else{
		if(!rand_is_init){//Seed randomly when mounted
    		srand(static_cast<unsigned>(time(nullptr)));
			rand_is_init = true;
		}
	}
}

//local file logging
void log_error(const char *error_message, std::string file_name,fuse_ino_t ino){
    // Open the file in append mode ("a"), creates it if it doesn't exist
    FILE *file = fopen(file_name.c_str(), "a");

    if (file == nullptr) {//error handle
        std::cout << ("Error opening file!\n") << std::endl;
        return;
    }

    // Get the current time
    time_t rawtime;
    struct tm *timeinfo;
    char time_buffer[80];//string for formatted time
    time(&rawtime);  // Get current time
        timeinfo = localtime(&rawtime);  // Convert to local time format
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", timeinfo);// Format the time as: "YYYY-MM-DD HH:MM:SS"

    // Write the error message with a timestamp to the file
    fprintf(file, "[%s] ERROR: %s. Inode Number: %ld\n", time_buffer, error_message, ino);

    // Close the file
    fclose(file);
}

void log_alert(const char *error_message, const char *file_name, fuse_ino_t ino){//alert used for debug, to print to a file since cout doesnt work
    // Open the file in append mode ("a"), creates it if it doesn't exist
    FILE *file = fopen(file_name, "a");

    if (file == nullptr) {//error handle
        std::cout << ("Error opening file!\n") << std::endl;
        return;
    }

    // Get the current time
    time_t rawtime;
    struct tm *timeinfo;
    char time_buffer[80];//string for formatted time
    time(&rawtime);  // Get current time
        timeinfo = localtime(&rawtime);  // Convert to local time format
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", timeinfo);// Format the time as: "YYYY-MM-DD HH:MM:SS"

    // Write the error message with a timestamp to the file
    fprintf(file, "[%s] ALERT: %s. Inode: %ld\n", time_buffer, error_message, ino);

    // Close the file
    fclose(file);
}

static struct lo_data *lo_data(fuse_req_t req)
{
	return (struct lo_data *) fuse_req_userdata(req);
}

static struct lo_inode *lo_inode(fuse_req_t req, fuse_ino_t ino)
{
	if (ino == FUSE_ROOT_ID)
		return &lo_data(req)->root;
	else
		return (struct lo_inode *) (uintptr_t) ino;
}

static int lo_fd(fuse_req_t req, fuse_ino_t ino)
{
	return lo_inode(req, ino)->fd;
}

static bool lo_debug(fuse_req_t req)
{
	return lo_data(req)->debug != 0;
}

static void lo_init(void *userdata,
		    struct fuse_conn_info *conn)
{
	struct lo_data *lo = (struct lo_data*) userdata;

	if (lo->writeback &&
	    conn->capable & FUSE_CAP_WRITEBACK_CACHE) {
		if (lo->debug)
			fuse_log(FUSE_LOG_DEBUG, "lo_init: activating writeback\n");
		conn->want |= FUSE_CAP_WRITEBACK_CACHE;
	}
	if (lo->flock && conn->capable & FUSE_CAP_FLOCK_LOCKS) {
		if (lo->debug)
			fuse_log(FUSE_LOG_DEBUG, "lo_init: activating flock locks\n");
		conn->want |= FUSE_CAP_FLOCK_LOCKS;
	}

	/* Disable the receiving and processing of FUSE_INTERRUPT requests */
	#ifdef FUSE_HAS_NO_INTERRUPT
	conn->no_interrupt = 1;
	#endif
}

static void lo_destroy(void *userdata)
{
	struct lo_data *lo = (struct lo_data*) userdata;

	while (lo->root.next != &lo->root) {
		struct lo_inode* next = lo->root.next;
		lo->root.next = next->next;
		close(next->fd);
		free(next);
	}
}

static void lo_getattr(fuse_req_t req, fuse_ino_t ino,
			     struct fuse_file_info *fi)
{
	int res;
	struct stat buf;
	struct lo_data *lo = lo_data(req);
	int fd = fi ? fi->fh : lo_fd(req, ino);

	(void) fi;

	res = fstatat(fd, "", &buf, AT_EMPTY_PATH | AT_SYMLINK_NOFOLLOW);
	if (res == -1){
		return (void) fuse_reply_err(req, errno);
	}

	fuse_reply_attr(req, &buf, lo->timeout);
}

static void lo_setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr,
		       int valid, struct fuse_file_info *fi)
{
	int saverr;
	char procname[64];
	struct lo_inode *inode = lo_inode(req, ino);
	int ifd = inode->fd;
	int res;

	if (valid & FUSE_SET_ATTR_MODE) {
		if (fi) {
			res = fchmod(fi->fh, attr->st_mode);
		} else {
			sprintf(procname, "/proc/self/fd/%i", ifd);
			res = chmod(procname, attr->st_mode);
		}
		if (res == -1)
			goto out_err;
	}
	if (valid & (FUSE_SET_ATTR_UID | FUSE_SET_ATTR_GID)) {
		uid_t uid = (valid & FUSE_SET_ATTR_UID) ?
			attr->st_uid : (uid_t) -1;
		gid_t gid = (valid & FUSE_SET_ATTR_GID) ?
			attr->st_gid : (gid_t) -1;

		res = fchownat(ifd, "", uid, gid,
			       AT_EMPTY_PATH | AT_SYMLINK_NOFOLLOW);
		if (res == -1)
			goto out_err;
	}
	if (valid & FUSE_SET_ATTR_SIZE) {
		if (fi) {
			res = ftruncate(fi->fh, attr->st_size);
		} else {
			sprintf(procname, "/proc/self/fd/%i", ifd);
			res = truncate(procname, attr->st_size);
		}
		if (res == -1)
			goto out_err;
	}
	if (valid & (FUSE_SET_ATTR_ATIME | FUSE_SET_ATTR_MTIME)) {
		struct timespec tv[2];

		tv[0].tv_sec = 0;
		tv[1].tv_sec = 0;
		tv[0].tv_nsec = UTIME_OMIT;
		tv[1].tv_nsec = UTIME_OMIT;

		if (valid & FUSE_SET_ATTR_ATIME_NOW)
			tv[0].tv_nsec = UTIME_NOW;
		else if (valid & FUSE_SET_ATTR_ATIME)
			tv[0] = attr->st_atim;

		if (valid & FUSE_SET_ATTR_MTIME_NOW)
			tv[1].tv_nsec = UTIME_NOW;
		else if (valid & FUSE_SET_ATTR_MTIME)
			tv[1] = attr->st_mtim;

		if (fi)
			res = futimens(fi->fh, tv);
		else {
			sprintf(procname, "/proc/self/fd/%i", ifd);
			res = utimensat(AT_FDCWD, procname, tv, 0);
		}
		if (res == -1)
			goto out_err;
	}

	return lo_getattr(req, ino, fi);

out_err:
	saverr = errno;
	fuse_reply_err(req, saverr);
}

static struct lo_inode *lo_find(struct lo_data *lo, struct stat *st)
{
	struct lo_inode *p;
	struct lo_inode *ret = nullptr;

	pthread_mutex_lock(&lo->mutex);
	for (p = lo->root.next; p != &lo->root; p = p->next) {
		if (p->ino == st->st_ino && p->dev == st->st_dev) {
			assert(p->refcount > 0);
			ret = p;
			ret->refcount++;
			break;
		}
	}
	pthread_mutex_unlock(&lo->mutex);
	return ret;
}

static int lo_do_lookup(fuse_req_t req, fuse_ino_t parent, const char *name,
			 struct fuse_entry_param *e)
{
	int newfd;
	int res;
	int saverr;
	struct lo_data *lo = lo_data(req);
	struct lo_inode *inode;

	memset(e, 0, sizeof(*e));
	e->attr_timeout = lo->timeout;
	e->entry_timeout = lo->timeout;

	newfd = openat(lo_fd(req, parent), name, O_PATH | O_NOFOLLOW);
	if (newfd == -1)
		goto out_err;

	res = fstatat(newfd, "", &e->attr, AT_EMPTY_PATH | AT_SYMLINK_NOFOLLOW);
	if (res == -1)
		goto out_err;

	inode = lo_find(lo_data(req), &e->attr);
	if (inode) {
		close(newfd);
		newfd = -1;
	} else {
		struct lo_inode *prev, *next;

		saverr = ENOMEM;
		inode = static_cast<struct lo_inode *>(calloc(1, sizeof(struct lo_inode)));
		if (!inode)
			goto out_err;

		inode->refcount = 1;
		inode->fd = newfd;
		inode->ino = e->attr.st_ino;
		inode->dev = e->attr.st_dev;

		pthread_mutex_lock(&lo->mutex);
		prev = &lo->root;
		next = prev->next;
		next->prev = inode;
		inode->next = next;
		inode->prev = prev;
		prev->next = inode;
		pthread_mutex_unlock(&lo->mutex);
	}
	e->ino = (uintptr_t) inode;

	if (lo_debug(req))
		fuse_log(FUSE_LOG_DEBUG, "  %lli/%s -> %lli\n",
			(unsigned long long) parent, name, (unsigned long long) e->ino);

	return 0;

out_err:
	saverr = errno;
	if (newfd != -1)
		close(newfd);
	return saverr;
}

static void lo_lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
	struct fuse_entry_param e;
	int err;

	if (lo_debug(req))
		fuse_log(FUSE_LOG_DEBUG, "lo_lookup(parent=%" PRIu64 ", name=%s)\n",
			parent, name);

	err = lo_do_lookup(req, parent, name, &e);
	if (err)
		fuse_reply_err(req, err);
	else
		fuse_reply_entry(req, &e);
}

static void lo_mknod_symlink(fuse_req_t req, fuse_ino_t parent,
			     const char *name, mode_t mode, dev_t rdev,
			     const char *link)
{
	int res;
	int saverr;
	struct lo_inode *dir = lo_inode(req, parent);
	struct fuse_entry_param e;

	res = mknod_wrapper(dir->fd, name, link, mode, rdev);

	saverr = errno;
	if (res == -1)
		goto out;

	saverr = lo_do_lookup(req, parent, name, &e);
	if (saverr)
		goto out;

	if (lo_debug(req))
		fuse_log(FUSE_LOG_DEBUG, "  %lli/%s -> %lli\n",
			(unsigned long long) parent, name, (unsigned long long) e.ino);

	fuse_reply_entry(req, &e);
	return;

out:
	fuse_reply_err(req, saverr);
}

static void lo_mknod(fuse_req_t req, fuse_ino_t parent,
		     const char *name, mode_t mode, dev_t rdev)
{
	lo_mknod_symlink(req, parent, name, mode, rdev, nullptr);
}

static void lo_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name,
		     mode_t mode)
{
	lo_mknod_symlink(req, parent, name, S_IFDIR | mode, 0, nullptr);
}

static void lo_symlink(fuse_req_t req, const char *link,
		       fuse_ino_t parent, const char *name)
{
	lo_mknod_symlink(req, parent, name, S_IFLNK, 0, link);
}

static void lo_link(fuse_req_t req, fuse_ino_t ino, fuse_ino_t parent,
		    const char *name)
{
	int res;
	struct lo_data *lo = lo_data(req);
	struct lo_inode *inode = lo_inode(req, ino);
	struct fuse_entry_param e;
	char procname[64];
	int saverr;

	memset(&e, 0, sizeof(struct fuse_entry_param));
	e.attr_timeout = lo->timeout;
	e.entry_timeout = lo->timeout;

	sprintf(procname, "/proc/self/fd/%i", inode->fd);
	res = linkat(AT_FDCWD, procname, lo_fd(req, parent), name,
		     AT_SYMLINK_FOLLOW);
	if (res == -1)
		goto out_err;

	res = fstatat(inode->fd, "", &e.attr, AT_EMPTY_PATH | AT_SYMLINK_NOFOLLOW);
	if (res == -1)
		goto out_err;

	pthread_mutex_lock(&lo->mutex);
	inode->refcount++;
	pthread_mutex_unlock(&lo->mutex);
	e.ino = (uintptr_t) inode;

	if (lo_debug(req))
		fuse_log(FUSE_LOG_DEBUG, "  %lli/%s -> %lli\n",
			(unsigned long long) parent, name,
			(unsigned long long) e.ino);

	fuse_reply_entry(req, &e);
	return;

out_err:
	saverr = errno;
	fuse_reply_err(req, saverr);
}

static void lo_rmdir(fuse_req_t req, fuse_ino_t parent, const char *name)
{
	int res;

	res = unlinkat(lo_fd(req, parent), name, AT_REMOVEDIR);

	fuse_reply_err(req, res == -1 ? errno : 0);
}

static void lo_rename(fuse_req_t req, fuse_ino_t parent, const char *name,
		      fuse_ino_t newparent, const char *newname,
		      unsigned int flags)
{
	int res;

	if (flags) {
		fuse_reply_err(req, EINVAL);
		return;
	}

	res = renameat(lo_fd(req, parent), name,
			lo_fd(req, newparent), newname);

	fuse_reply_err(req, res == -1 ? errno : 0);
}

static void lo_unlink(fuse_req_t req, fuse_ino_t parent, const char *name)
{
	int res;

	res = unlinkat(lo_fd(req, parent), name, 0);

	fuse_reply_err(req, res == -1 ? errno : 0);
}

static void unref_inode(struct lo_data *lo, struct lo_inode *inode, uint64_t n)
{
	if (!inode)
		return;

	pthread_mutex_lock(&lo->mutex);
	assert(inode->refcount >= n);
	inode->refcount -= n;
	if (!inode->refcount) {
		struct lo_inode *prev, *next;

		prev = inode->prev;
		next = inode->next;
		next->prev = prev;
		prev->next = next;

		pthread_mutex_unlock(&lo->mutex);
		close(inode->fd);
		free(inode);

	} else {
		pthread_mutex_unlock(&lo->mutex);
	}
}

static void lo_forget_one(fuse_req_t req, fuse_ino_t ino, uint64_t nlookup)
{
	struct lo_data *lo = lo_data(req);
	struct lo_inode *inode = lo_inode(req, ino);

	if (lo_debug(req)) {
		fuse_log(FUSE_LOG_DEBUG, "  forget %lli %lli -%lli\n",
			(unsigned long long) ino,
			(unsigned long long) inode->refcount,
			(unsigned long long) nlookup);
	}

	unref_inode(lo, inode, nlookup);
}

static void lo_forget(fuse_req_t req, fuse_ino_t ino, unsigned long nlookup)
{
	lo_forget_one(req, ino, nlookup);
	fuse_reply_none(req);
}

static void lo_forget_multi(fuse_req_t req, size_t count,
				struct fuse_forget_data *forgets)
{
	size_t i;

	for (i = 0; i < count; i++)
		lo_forget_one(req, forgets[i].ino, forgets[i].nlookup);
	fuse_reply_none(req);
}

static void lo_readlink(fuse_req_t req, fuse_ino_t ino)
{
	char buf[PATH_MAX + 1];
	int res;

	res = readlinkat(lo_fd(req, ino), "", buf, sizeof(buf));
	if (res == -1)
		return (void) fuse_reply_err(req, errno);

	if (res == sizeof(buf))
		return (void) fuse_reply_err(req, ENAMETOOLONG);

	buf[res] = '\0';

	fuse_reply_readlink(req, buf);
}

struct lo_dirp {
	DIR *dp;
	struct dirent *entry;
	off_t offset;
};

static struct lo_dirp *lo_dirp(struct fuse_file_info *fi)
{
	return (struct lo_dirp *) (uintptr_t) fi->fh;
}

static void lo_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
	int error = ENOMEM;
	struct lo_data *lo = lo_data(req);
	struct lo_dirp *d;
	int fd;

	if (rand() % DFAILRATE == 0) {  // Example: 10% probability of error
		log_error("lo_opendir: An unexpected failure occurred", ERRLOGFILE, ino); 
        //otel span 
		auto span = traceAndSpan("faulty_lo_opendir");
		span->SetAttribute("Operation", "directory.open");
		span->SetAttribute("inode_number", (ino));
		span->AddEvent("Abrupt Exit Simulated", {{"Timestamp", getCurrentTime()}, {"error_type", "ENOENT"}});
		span->End();
		file_handle->flush();
		fuse_reply_err(req, ENOENT);//simulates space was too full to flush
		return; // Exit the function early after sending the error
    }else{
		d = static_cast<struct lo_dirp *>(calloc(1, sizeof(struct lo_dirp)));
		if (d == nullptr)
			goto out_err;

		fd = openat(lo_fd(req, ino), ".", O_RDONLY);
		if (fd == -1)
			goto out_errno;

		d->dp = fdopendir(fd);
		if (d->dp == nullptr)
			goto out_errno;

		d->offset = 0;
		d->entry = nullptr;

		fi->fh = (uintptr_t) d;
		if (lo->cache == CACHE_ALWAYS) fi->cache_readdir = 1;
		
		//simulate delayed io
		if (rand() % DFAILRATE == 0) {  // Example: 10% probability of delay
			sleep(DELAYTIME); // Delay for DELAYTIME seconds
			log_error("lo_opendir: An unexpected delay occurred", ERRLOGFILE, ino);
			auto span = traceAndSpan("faulty_lo_opendir");
			span->SetAttribute("Operation", "directory.open");
			span->SetAttribute("inode_number", (ino));
			span->AddEvent("Delayed Opendir Simulated", {{"Timestamp", getCurrentTime()}, {"delay_time", DELAYTIME}});
			span->End();
			file_handle->flush();//flush ostream_out.txt buffer
		}

		fuse_reply_open(req, fi);
		return;
	}
out_errno:
	error = errno;
out_err:
	if (d) {
		if (fd != -1)
			close(fd);
		free(d);
	}
	fuse_reply_err(req, error);
}

static int is_dot_or_dotdot(const char *name)
{
	return name[0] == '.' && (name[1] == '\0' ||
				  (name[1] == '.' && name[2] == '\0'));
}

static void lo_do_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
			  off_t offset, struct fuse_file_info *fi, int plus)
{
	struct lo_dirp *d = lo_dirp(fi);
	char *buf;
	char *p;
	size_t rem = size;
	int err;

	//simulate failed io
	if (rand() % DFAILRATE == 0) {  // Example: 10% probability of error
		log_error("lo_do_readdir: An unexpected failure occurred", ERRLOGFILE, ino); 
        //otel span 
		auto span = traceAndSpan("faulty_lo_do_readdir");
		span->SetAttribute("Operation", "directory.read");
		span->SetAttribute("Dir_offset", (offset));
		span->SetAttribute("inode_number", (ino));
		span->AddEvent("Abrupt Exit Simulated", {{"Timestamp", getCurrentTime()}, {"error_type", "EIO"}});
		span->End();
		file_handle->flush();
		fuse_reply_err(req, EIO);
		return; // Exit the function early after sending the error
    }else{

		//simulate delayed io
		if (rand() % DFAILRATE == 0) {  // Example: 10% probability of delay
			sleep(DELAYTIME); // Delay for DELAYTIME seconds
			log_error("lo_do_readdir: An unexpected delay occurred", ERRLOGFILE, ino);
			auto span = traceAndSpan("faulty_lo_do_readdir");
			span->SetAttribute("Operation", "directory.read");
			span->SetAttribute("Dir_offset", (offset));
			span->SetAttribute("inode_number", (ino));
			span->AddEvent("Delayed Directory Read Simulated", {{"Timestamp", getCurrentTime()}, {"delay_time", DELAYTIME}});
			span->End();
			file_handle->flush();//flush ostream_out.txt buffer
		}	
	
		buf = static_cast<char *>(calloc(1, size));
		if (!buf) {
			err = ENOMEM;
			goto error;
		}
		p = buf;

		if (offset != d->offset) {
			seekdir(d->dp, offset);
			d->entry = nullptr;
			d->offset = offset;
		}
		while (1) {
			size_t entsize;
			off_t nextoff;
			const char *name;

			if (!d->entry) {
				errno = 0;
				d->entry = readdir(d->dp);
				if (!d->entry) {
					if (errno) {  // Error
						err = errno;
						goto error;
					} else {  // End of stream
						break; 
					}
				}
			}
			nextoff = d->entry->d_off;
			name = d->entry->d_name;
			fuse_ino_t entry_ino = 0;
			if (plus) {
				struct fuse_entry_param e;
				if (is_dot_or_dotdot(name)) {
					e.attr.st_ino = d->entry->d_ino;
					e.attr.st_mode = d->entry->d_type << 12;
				} else {
					err = lo_do_lookup(req, ino, name, &e);
					if (err)
						goto error;
					entry_ino = e.ino;
				}

				entsize = fuse_add_direntry_plus(req, p, rem, name,
								&e, nextoff);
			} else {
				struct stat st = {
					.st_ino = d->entry->d_ino,
					.st_mode = static_cast<unsigned int>(d->entry->d_type << 12),
				};
				entsize = fuse_add_direntry(req, p, rem, name,
								&st, nextoff);
			}
			if (entsize > rem) {
				if (entry_ino != 0) 
					lo_forget_one(req, entry_ino, 1);
				break;
			}
			
			p += entsize;
			rem -= entsize;

			d->entry = nullptr;
			d->offset = nextoff;
		}

		err = 0;

	}
error:
    // If there's an error, we can only signal it if we haven't stored
    // any entries yet - otherwise we'd end up with wrong lookup
    // counts for the entries that are already in the buffer. So we
    // return what we've collected until that point.
    if (err && rem == size)
	    fuse_reply_err(req, err);
    else
	    fuse_reply_buf(req, buf, size - rem);
    free(buf);
}

static void lo_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
		       off_t offset, struct fuse_file_info *fi)
{
	lo_do_readdir(req, ino, size, offset, fi, 0);
}

static void lo_readdirplus(fuse_req_t req, fuse_ino_t ino, size_t size,
			   off_t offset, struct fuse_file_info *fi)
{
	lo_do_readdir(req, ino, size, offset, fi, 1);
}

static void lo_releasedir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
	struct lo_dirp *d = lo_dirp(fi);
	(void) ino;
	closedir(d->dp);
	free(d);
	fuse_reply_err(req, 0);
}

static void lo_create(fuse_req_t req, fuse_ino_t parent, const char *name,
		      mode_t mode, struct fuse_file_info *fi)
{
	int fd;
	struct lo_data *lo = lo_data(req);
	struct fuse_entry_param e;
	int err;

	if (lo_debug(req))
		fuse_log(FUSE_LOG_DEBUG, "lo_create(parent=%" PRIu64 ", name=%s)\n",
			parent, name);

	//simulate abrupt exit

	fd = openat(lo_fd(req, parent), name,
		    (fi->flags | O_CREAT) & ~O_NOFOLLOW, mode);
	if (fd == -1)
		return (void) fuse_reply_err(req, errno);

	fi->fh = fd;
	if (lo->cache == CACHE_NEVER)
		fi->direct_io = 1;
	else if (lo->cache == CACHE_ALWAYS)
		fi->keep_cache = 1;

	/* parallel_direct_writes feature depends on direct_io features.
	   To make parallel_direct_writes valid, need set fi->direct_io
	   in current function. */
	fi->parallel_direct_writes = 1;

	err = lo_do_lookup(req, parent, name, &e);
	if (err)
		fuse_reply_err(req, err);
	else
		//simulate delay
		fuse_reply_create(req, &e, fi);
}

static void lo_fsyncdir(fuse_req_t req, fuse_ino_t ino, int datasync,
			struct fuse_file_info *fi)
{
	int res;
	int fd = dirfd(lo_dirp(fi)->dp);
	(void) ino;
	if (datasync)
		res = fdatasync(fd);
	else
		res = fsync(fd);
	fuse_reply_err(req, res == -1 ? errno : 0);
}

static void lo_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
	int fd;
	char buf[64];
	struct lo_data *lo = lo_data(req);

	if (rand() % FFAILRATE == 0) {  // Example: 10% probability of error
		log_error("lo_open: An unexpected failure occurred", ERRLOGFILE, ino); 
        //otel span 
		auto span = traceAndSpan("faulty_lo_open");
		span->SetAttribute("Operation", "file.open");
		span->SetAttribute("inode_number", (ino));
		span->AddEvent("Abrupt Exit Simulated", {{"Timestamp", getCurrentTime()}, {"error_type", "ENOENT"}});
		span->End();
		file_handle->flush();
		fuse_reply_err(req, ENOENT);//simulates space was too full to flush
		return; // Exit the function early after sending the error
    }else{
		if (lo_debug(req))
			fuse_log(FUSE_LOG_DEBUG, "lo_open(ino=%" PRIu64 ", flags=%d)\n",
				ino, fi->flags);

		//simulate abrupt exit

		/* With writeback cache, kernel may send read requests even
		when userspace opened write-only */
		if (lo->writeback && (fi->flags & O_ACCMODE) == O_WRONLY) {
			fi->flags &= ~O_ACCMODE;
			fi->flags |= O_RDWR;
		}

		/* With writeback cache, O_APPEND is handled by the kernel.
		This breaks atomicity (since the file may change in the
		underlying filesystem, so that the kernel's idea of the
		end of the file isn't accurate anymore). In this example,
		we just accept that. A more rigorous filesystem may want
		to return an error here */
		if (lo->writeback && (fi->flags & O_APPEND))
			fi->flags &= ~O_APPEND;

		sprintf(buf, "/proc/self/fd/%i", lo_fd(req, ino));
		fd = open(buf, fi->flags & ~O_NOFOLLOW);
		if (fd == -1)
			return (void) fuse_reply_err(req, errno);

		fi->fh = fd;
		
		if (lo->cache == CACHE_NEVER)
			fi->direct_io = 1;
		else if (lo->cache == CACHE_ALWAYS)
			fi->keep_cache = 1;

			/* Enable direct_io when open has flags O_DIRECT to enjoy the feature
			parallel_direct_writes (i.e., to get a shared lock, not exclusive lock,
		for writes to the same file in the kernel). */
		if (fi->flags & O_DIRECT)
			fi->direct_io = 1;

		/* parallel_direct_writes feature depends on direct_io features.
		To make parallel_direct_writes valid, need set fi->direct_io
		in current function. */
		fi->parallel_direct_writes = 1;
		
		//simulate delayed io
		if (rand() % FFAILRATE == 0) {  // Example: 10% probability of delay
			sleep(DELAYTIME); // Delay for DELAYTIME seconds
			log_error("lo_open: An unexpected delay occurred", ERRLOGFILE, ino);
			auto span = traceAndSpan("faulty_lo_open");
			span->SetAttribute("Operation", "file.open");
			span->SetAttribute("inode_number", (ino));
			span->AddEvent("Delayed Open Simulated", {{"Timestamp", getCurrentTime()}, {"delay_time", DELAYTIME}});
			span->End();
			file_handle->flush();//flush ostream_out.txt buffer
		}

		fuse_reply_open(req, fi);
	}
}

static void lo_release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
	//abrupt exit
	close(fi->fh);
	//delay
	fuse_reply_err(req, 0);
}

static void lo_flush(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi)
{
	if (rand() % FFAILRATE == 0) {  // Example: 10% probability of error
		log_error("lo_flush: An unexpected failure occurred", ERRLOGFILE, ino); 
        //otel span 
		auto span = traceAndSpan("faulty_lo_flush");
		span->SetAttribute("Operation", "file.flush");
		span->SetAttribute("inode_number", (ino));
		span->AddEvent("Abrupt Exit Simulated", {{"Timestamp", getCurrentTime()}, {"error_type", "ENOSPC"}});
		span->End();
		file_handle->flush();
		fuse_reply_err(req, ENOSPC);//simulates space was too full to flush
		return; // Exit the function early after sending the error
    }else{
		int res;
		res = close(dup(fi->fh));
		//simulate delayed io
		if (rand() % FFAILRATE == 0) {  // Example: 10% probability of delay
			sleep(DELAYTIME); // Delay for DELAYTIME seconds
			log_error("lo_flush: An unexpected delay occurred", ERRLOGFILE, ino);
			auto span = traceAndSpan("faulty_lo_flush");
			span->SetAttribute("Operation", "file.flush");
			span->SetAttribute("inode_number", (ino));
			span->AddEvent("Delayed Flush Simulated", {{"Timestamp", getCurrentTime()}, {"delay_time", DELAYTIME}});
			span->End();
			file_handle->flush();//flush ostream_out.txt buffer
		}
		fuse_reply_err(req, res == -1 ? errno : 0);
	}
}

static void lo_fsync(fuse_req_t req, fuse_ino_t ino, int datasync,
		     struct fuse_file_info *fi)
{
	int res;
	(void) ino;
	if (datasync){
		res = fdatasync(fi->fh);
	}else{
		res = fsync(fi->fh);
	}
	fuse_reply_err(req, res == -1 ? errno : 0);
}

static void lo_read(fuse_req_t req, fuse_ino_t ino, size_t size,
		    off_t offset, struct fuse_file_info *fi)
{
	//init_random_seed();
	int is_faulty = 0;

	//simulate failed io
	if (rand() % FFAILRATE == 0) {  // Example: 10% probability of error
		log_error("lo_read: An unexpected failure occurred", ERRLOGFILE, ino); 
        //otel span 
		auto span = traceAndSpan("faulty_lo_read");
		span->SetAttribute("Operation", "file.read");
		span->SetAttribute("File_offset", (offset));
		span->SetAttribute("inode_number", (ino));
		span->AddEvent("Abrupt Exit Simulated", {{"Timestamp", getCurrentTime()}, {"error_type", "EIO"}});
		span->End();
		file_handle->flush();
		fuse_reply_err(req, EIO);
		return; // Exit the function early after sending the error
    }else{
		struct fuse_bufvec buf = FUSE_BUFVEC_INIT(size);

		if (lo_debug(req)){
			fuse_log(FUSE_LOG_DEBUG, "lo_read(ino=%" PRIu64 ", size=%zd, "
				"off=%lu)\n", ino, size, (unsigned long) offset);
		}

		//simulate delayed io
		if (rand() % FFAILRATE == 0) {  // Example: 10% probability of delay
			sleep(DELAYTIME); // Delay for DELAYTIME seconds
			log_error("lo_read: An unexpected delay occurred", ERRLOGFILE, ino);
			is_faulty += 1;
		}

		//simulate truncated read
		if (rand() % FFAILRATE == 0) { // 10% probability of a short read
			buf.buf[0].size = rand() % 10 + 5;//truncated size between 5-15
			buf.buf[0].flags = static_cast<fuse_buf_flags>(FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK);
			buf.buf[0].fd = fi->fh;
			buf.buf[0].pos = offset + (rand()%10);//move the offset up a bit to simulate truncation
			log_error("lo_read: Truncated read occurred", ERRLOGFILE, ino);
			is_faulty += 2;
		}else{
			buf.buf[0].flags = static_cast<fuse_buf_flags>(FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK);
			buf.buf[0].fd = fi->fh;
			buf.buf[0].pos = offset;
		}
		
		//if faults occured log them accordingly
		if(is_faulty){
			auto span = traceAndSpan("faulty_lo_read");
			span->SetAttribute("Operation", "file.read");
			span->SetAttribute("File_offset", (offset));
			span->SetAttribute("inode_number", (ino));
			if(is_faulty % 2){//if %2 it means first fault happened
				span->AddEvent("Delayed Read Simulated", {{"Timestamp", getCurrentTime()}, {"delay_time", DELAYTIME}});
				is_faulty -=1;
			}
			if(is_faulty % 4){
				span->AddEvent("Truncated Read Simulated", {{"Timestamp", getCurrentTime()}, {"size", static_cast<int>(buf.buf[0].size)}});
				//span->AddEvent("Delay Simulated", {{"Timestamp", getCurrentTime()}, {"size", buf.buf[0].size}});
				is_faulty -= 2;
			}
			span->End();
			file_handle->flush();
		}
		fuse_reply_data(req, &buf, FUSE_BUF_SPLICE_MOVE);
	}
}

static void lo_write_buf(fuse_req_t req, fuse_ino_t ino,
			 struct fuse_bufvec *in_buf, off_t off,
			 struct fuse_file_info *fi)
{
	ssize_t res;

	//init_random_seed();
	int is_faulty = 0;

	//simulate IO error
	if (rand() % FFAILRATE == 0) {  // Example: 10% probability of error
		log_error("lo_write_buf: An unexpected failure occurred", ERRLOGFILE, ino);
        //otel span
		auto span = traceAndSpan("faulty_lo_write");
		span->SetAttribute("Operation", "file.write");
		span->SetAttribute("File_offset", (off));
		span->SetAttribute("inode_number", (ino));
		span->AddEvent("Abrupt Exit Simulated", {{"Timestamp", getCurrentTime()}, {"error_type", "EIO"}});
		span->End();
		file_handle->flush();
		fuse_reply_err(req, EIO); 
		return; // Exit the function early after sending the error
    }else{
		struct fuse_bufvec out_buf;
		out_buf = FUSE_BUFVEC_INIT(fuse_buf_size(in_buf));

		out_buf.buf[0].flags = static_cast<fuse_buf_flags>(FUSE_BUF_IS_FD | FUSE_BUF_FD_SEEK);
		out_buf.buf[0].fd = fi->fh;
		out_buf.buf[0].pos = off;

		if (lo_debug(req)){
			fuse_log(FUSE_LOG_DEBUG, "lo_write(ino=%" PRIu64 ", size=%zd, off=%lu)\n",
				ino, out_buf.buf[0].size, (unsigned long) off);
		}
		res = fuse_buf_copy(&out_buf, in_buf, static_cast<fuse_buf_copy_flags>(0));
		if(res < 0)
			fuse_reply_err(req, -res);
		else{
			//simulate delayed io
			if (rand() % FFAILRATE == 0) {  // Example: 10% probability of delay
				sleep(DELAYTIME); // Delay for 5 seconds
				log_error("lo_write_buf: An unexpected delay occurred", ERRLOGFILE, ino);
				is_faulty += 1;
			}

			//simulate truncated write
			if (rand() % FFAILRATE == 0) {
				res = res / 2;	//buf size cut in half
				log_error("lo_write_buf: Truncated write occurred", ERRLOGFILE, ino);
				is_faulty += 2;
			}

			if(is_faulty){
				auto span = traceAndSpan("faulty_lo_write");
				span->SetAttribute("Operation", "file.write");
				span->SetAttribute("File_offset", (off));
				span->SetAttribute("inode_number", (ino));
				if(is_faulty % 2){//if %2 it means first fault happened
					span->AddEvent("Delayed Write Simulated", {{"Timestamp", getCurrentTime()}, {"delay_time", DELAYTIME}});
					is_faulty -=1;
				}
				if(is_faulty % 4){
					span->AddEvent("Truncated Write Simulated", {{"Timestamp", getCurrentTime()}, {"size", static_cast<int>(res)}});
					is_faulty -= 2;
				}
				span->End();
				file_handle->flush();
			}
			fuse_reply_write(req, (size_t) res);
		}
	}
}

static void lo_statfs(fuse_req_t req, fuse_ino_t ino)
{
	int res;
	struct statvfs stbuf;

	res = fstatvfs(lo_fd(req, ino), &stbuf);
	if (res == -1)
		fuse_reply_err(req, errno);
	else
		fuse_reply_statfs(req, &stbuf);
}

static void lo_fallocate(fuse_req_t req, fuse_ino_t ino, int mode,
			 off_t offset, off_t length, struct fuse_file_info *fi)
{
	int err = EOPNOTSUPP;
	(void) ino;

#ifdef HAVE_FALLOCATE
	err = fallocate(fi->fh, mode, offset, length);
	if (err < 0)
		err = errno;

#elif defined(HAVE_POSIX_FALLOCATE)
	if (mode) {
		fuse_reply_err(req, EOPNOTSUPP);
		return;
	}

	err = posix_fallocate(fi->fh, offset, length);
#endif

	fuse_reply_err(req, err);
}

static void lo_flock(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi,
		     int op)
{
	int res;
	(void) ino;

	res = flock(fi->fh, op);

	fuse_reply_err(req, res == -1 ? errno : 0);
}

static void lo_getxattr(fuse_req_t req, fuse_ino_t ino, const char *name,
			size_t size)
{
	char *value = nullptr;
	char procname[64];
	struct lo_inode *inode = lo_inode(req, ino);
	ssize_t ret;
	int saverr;

	saverr = ENOSYS;
	if (!lo_data(req)->xattr)
		goto out;

	if (lo_debug(req)) {
		fuse_log(FUSE_LOG_DEBUG, "lo_getxattr(ino=%" PRIu64 ", name=%s size=%zd)\n",
			ino, name, size);
	}

	sprintf(procname, "/proc/self/fd/%i", inode->fd);

	if (size) {
		value = static_cast<char *>(malloc(size));
		
		if (!value)
			goto out_err;

		ret = getxattr(procname, name, value, size);
		if (ret == -1)
			goto out_err;
		saverr = 0;
		if (ret == 0)
			goto out;

		fuse_reply_buf(req, value, ret);
	} else {
		ret = getxattr(procname, name, nullptr, 0);
		if (ret == -1)
			goto out_err;

		fuse_reply_xattr(req, ret);
	}
out_free:
	free(value);
	return;

out_err:
	saverr = errno;
out:
	fuse_reply_err(req, saverr);
	goto out_free;
}

static void lo_listxattr(fuse_req_t req, fuse_ino_t ino, size_t size)
{
	char *value = nullptr;
	char procname[64];
	struct lo_inode *inode = lo_inode(req, ino);
	ssize_t ret;
	int saverr;

	saverr = ENOSYS;
	if (!lo_data(req)->xattr)
		goto out;

	if (lo_debug(req)) {
		fuse_log(FUSE_LOG_DEBUG, "lo_listxattr(ino=%" PRIu64 ", size=%zd)\n",
			ino, size);
	}

	sprintf(procname, "/proc/self/fd/%i", inode->fd);

	if (size) {
		value = static_cast<char *>(malloc(size));
		if (!value)
			goto out_err;

		ret = listxattr(procname, value, size);
		if (ret == -1)
			goto out_err;
		saverr = 0;
		if (ret == 0)
			goto out;

		fuse_reply_buf(req, value, ret);
	} else {
		ret = listxattr(procname, nullptr, 0);
		if (ret == -1)
			goto out_err;

		fuse_reply_xattr(req, ret);
	}
out_free:
	free(value);
	return;

out_err:
	saverr = errno;
out:
	fuse_reply_err(req, saverr);
	goto out_free;
}

static void lo_setxattr(fuse_req_t req, fuse_ino_t ino, const char *name,
			const char *value, size_t size, int flags)
{
	char procname[64];
	struct lo_inode *inode = lo_inode(req, ino);
	ssize_t ret;
	int saverr;

	saverr = ENOSYS;
	if (!lo_data(req)->xattr)
		goto out;

	if (lo_debug(req)) {
		fuse_log(FUSE_LOG_DEBUG, "lo_setxattr(ino=%" PRIu64 ", name=%s value=%s size=%zd)\n",
			ino, name, value, size);
	}

	sprintf(procname, "/proc/self/fd/%i", inode->fd);

	ret = setxattr(procname, name, value, size, flags);
	saverr = ret == -1 ? errno : 0;

out:
	fuse_reply_err(req, saverr);
}

static void lo_removexattr(fuse_req_t req, fuse_ino_t ino, const char *name)
{
	char procname[64];
	struct lo_inode *inode = lo_inode(req, ino);
	ssize_t ret;
	int saverr;

	saverr = ENOSYS;
	if (!lo_data(req)->xattr)
		goto out;

	if (lo_debug(req)) {
		fuse_log(FUSE_LOG_DEBUG, "lo_removexattr(ino=%" PRIu64 ", name=%s)\n",
			ino, name);
	}

	sprintf(procname, "/proc/self/fd/%i", inode->fd);

	ret = removexattr(procname, name);
	saverr = ret == -1 ? errno : 0;

out:
	fuse_reply_err(req, saverr);
}

#ifdef HAVE_COPY_FILE_RANGE
static void lo_copy_file_range(fuse_req_t req, fuse_ino_t ino_in, off_t off_in,
			       struct fuse_file_info *fi_in,
			       fuse_ino_t ino_out, off_t off_out,
			       struct fuse_file_info *fi_out, size_t len,
			       int flags)
{
	ssize_t res;

	if (lo_debug(req))
		fuse_log(FUSE_LOG_DEBUG, "lo_copy_file_range(ino=%" PRIu64 "/fd=%lu, "
				"off=%lu, ino=%" PRIu64 "/fd=%lu, "
				"off=%lu, size=%zd, flags=0x%x)\n",
			ino_in, fi_in->fh, off_in, ino_out, fi_out->fh, off_out,
			len, flags);

	res = copy_file_range(fi_in->fh, &off_in, fi_out->fh, &off_out, len,
			      flags);
	if (res < 0)
		fuse_reply_err(req, errno);
	else
		fuse_reply_write(req, res);
}
#endif

static void lo_lseek(fuse_req_t req, fuse_ino_t ino, off_t off, int whence,
		     struct fuse_file_info *fi)
{
	off_t res;

	(void)ino;
	res = lseek(fi->fh, off, whence);
	if (res != -1)
		fuse_reply_lseek(req, res);
	else
		fuse_reply_err(req, errno);
}

static const struct fuse_lowlevel_ops lo_oper = {
	.init		= lo_init,
	.destroy	= lo_destroy,
	.lookup		= lo_lookup,
	.forget		= lo_forget,
	.getattr	= lo_getattr,
	.setattr	= lo_setattr,
	.readlink	= lo_readlink,
	.mknod		= lo_mknod,
	.mkdir		= lo_mkdir,
	.unlink		= lo_unlink,
	.rmdir		= lo_rmdir,
	.symlink	= lo_symlink,
	.rename		= lo_rename,
	.link		= lo_link,
	.open		= lo_open,
	.read		= lo_read,
	//.write?
	.flush		= lo_flush,
	.release	= lo_release,
	.fsync		= lo_fsync,
	.opendir	= lo_opendir,
	.readdir	= lo_readdir,
	.releasedir	= lo_releasedir,
	.fsyncdir	= lo_fsyncdir,
	.statfs		= lo_statfs,
	.setxattr	= lo_setxattr,
	.getxattr	= lo_getxattr,
	.listxattr	= lo_listxattr,
	.removexattr	= lo_removexattr,
	//.access?
	.create		= lo_create,
	//.getlk?
	//.setlk?
	//.bmap?
	//.ioctl?
	//.poll?
	.write_buf      = lo_write_buf,
	//.retrieve_reply?
	.forget_multi	= lo_forget_multi,
	.flock		= lo_flock,
	.fallocate	= lo_fallocate,
	//.reserved00
	//.reserved01
	//.reserved02
	//.renamex
	//.setvolname
	//.exchange
	//.getxtimes
	//.setattr_x
	.readdirplus	= lo_readdirplus,
#ifdef HAVE_COPY_FILE_RANGE
	.copy_file_range = lo_copy_file_range,
#endif
	.lseek		= lo_lseek,
};

void config_faulty(std::string config_path) {
    // Configs
    json config = getConfig(config_path);

    if (config.contains("faultyIO")) {
        auto configFaulty = config["faultyIO"];
		if(configFaulty.contains("local_log_path")) ERRLOGFILE = configFaulty["local_log_path"];
		if(configFaulty.contains("file_fail_rate")) FFAILRATE = configFaulty["file_fail_rate"];
		if(configFaulty.contains("directory_fail_rate")) DFAILRATE = configFaulty["directory_fail_rate"];
		if(configFaulty.contains("use_seednum")) SEEDNUM = configFaulty["use_seednum"];
		if(configFaulty.contains("seed")) SEEDNUM = configFaulty["seed"];
		if(configFaulty.contains("delay_time")) DELAYTIME = configFaulty["delay_time"];
    }
	return;
}

fuse_lowlevel_ops faulty_operations(fuse_lowlevel_ops &next) {
	faulty_next = &next;

	fuse_lowlevel_ops curr = next;
	curr.read = lo_read;
	curr.write_buf = lo_write_buf;
	curr.flush = lo_flush;
	curr.readdir = lo_readdir;
	curr.readdirplus = lo_readdirplus;
	curr.open = lo_open;
	curr.opendir = lo_opendir;

	return curr;
}

int main(int argc, char *argv[])
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct fuse_session *se;
	struct fuse_cmdline_opts opts;
	struct fuse_loop_config *config;
	struct lo_data lo = { .debug = 0,
	                      .writeback = 0 };
	int ret = -1;

	/* Don't mask creation mode, kernel already did that */
	umask(0);

	pthread_mutex_init(&lo.mutex, nullptr);
	lo.root.next = lo.root.prev = &lo.root;
	lo.root.fd = -1;
	lo.cache = CACHE_NORMAL;

	config_faulty("./config/config.json");
	init_random_seed();

	if (fuse_parse_cmdline(&args, &opts) != 0)
		return 1;
	if (opts.show_help) {
		printf("usage: %s [options] <mountpoint>\n\n", argv[0]);
		fuse_cmdline_help();
		fuse_lowlevel_help();
		passthrough_ll_help();
		ret = 0;
		goto err_out1;
	} else if (opts.show_version) {
		printf("FUSE library version %s\n", fuse_pkgversion());
		fuse_lowlevel_version();
		ret = 0;
		goto err_out1;
	}

	if(opts.mountpoint == nullptr) {
		printf("usage: %s [options] <mountpoint>\n", argv[0]);
		printf("       %s --help\n", argv[0]);
		ret = 1;
		goto err_out1;
	}

	if (fuse_opt_parse(&args, &lo, lo_opts, nullptr)== -1)
		return 1;

	lo.debug = opts.debug;
	lo.root.refcount = 2;
	if (lo.source) {
		struct stat stat;
		int res;

		res = lstat(lo.source, &stat);
		if (res == -1) {
			fuse_log(FUSE_LOG_ERR, "failed to stat source (\"%s\"): %m\n",
				 lo.source);
			exit(1);
		}
		if (!S_ISDIR(stat.st_mode)) {
			fuse_log(FUSE_LOG_ERR, "source is not a directory\n");
			exit(1);
		}

	} else {
		lo.source = strdup("/");
		if(!lo.source) {
			fuse_log(FUSE_LOG_ERR, "fuse: memory allocation failed\n");
			exit(1);
		}
	}
	if (!lo.timeout_set) {
		switch (lo.cache) {
		case CACHE_NEVER:
			lo.timeout = 0.0;
			break;

		case CACHE_NORMAL:
			lo.timeout = 1.0;
			break;

		case CACHE_ALWAYS:
			lo.timeout = 86400.0;
			break;
		}
	} else if (lo.timeout < 0) {
		fuse_log(FUSE_LOG_ERR, "timeout is negative (%lf)\n",
			 lo.timeout);
		exit(1);
	}

	lo.root.fd = open(lo.source, O_PATH);
	if (lo.root.fd == -1) {
		fuse_log(FUSE_LOG_ERR, "open(\"%s\", O_PATH): %m\n",
			 lo.source);
		exit(1);
	}

	se = fuse_session_new(&args, &lo_oper, sizeof(lo_oper), &lo);
	if (se == nullptr)
	    goto err_out1;

	if (fuse_set_signal_handlers(se) != 0)
	    goto err_out2;

	if (fuse_session_mount(se, opts.mountpoint) != 0)
	    goto err_out3;

	fuse_daemonize(opts.foreground);

	/* Block until ctrl+c or fusermount -u */
	if (opts.singlethread)
		ret = fuse_session_loop(se);
	else {
		config = fuse_loop_cfg_create();
		fuse_loop_cfg_set_clone_fd(config, opts.clone_fd);
		fuse_loop_cfg_set_max_threads(config, opts.max_threads);
		ret = fuse_session_loop_mt(se, config);
		fuse_loop_cfg_destroy(config);
		config = nullptr;
	}

	fuse_session_unmount(se);
err_out3:
	fuse_remove_signal_handlers(se);
err_out2:
	fuse_session_destroy(se);
err_out1:
	free(opts.mountpoint);
	fuse_opt_free_args(&args);

	if (lo.root.fd >= 0)
		close(lo.root.fd);

	free(lo.source);
	return ret ? 1 : 0;
}
