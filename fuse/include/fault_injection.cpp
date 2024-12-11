#define FUSE_USE_VERSION 31

#include <string.h>
#include <chrono>
#include "fault_injection.hpp"
#include <unordered_set>
#include "./config_parser.hpp"

#include <chrono>
#include <iomanip>
#include <ctime>
#include <sstream>

std::string ERRLOGFILE = "usr/src/myapp/testmount/error_log.txt"; //for function calls to the error log
int FFAILRATE = 0; //likelihood of file failure = 1/failrate
int DFAILRATE = 0; //likelihood of directory failure
bool CONFIGSEED = 0; //user can set configseed to one and it will use default value 0 or they can choose their own seed by setting seednum
int SEEDNUM = 0; //default 0 user can change
int DELAYTIME = 3;

static fuse_lowlevel_ops *fault_next;

// Low-level file operations

static void fault_init(void *userdata, struct fuse_conn_info *conn)
{
	fault_next->init(userdata, conn);
}

static void fault_write_buf(fuse_req_t req, fuse_ino_t ino,
                         struct fuse_bufvec *in_buf, off_t off,
                         struct fuse_file_info *fi)
{
        ssize_t res;

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
                //simulate delayed io
                if (rand() % FFAILRATE == 0) {  // Example: 10% probability of delay
                        sleep(DELAYTIME); // Delay for 5 seconds
                        log_error("lo_write_buf: An unexpected delay occurred", ERRLOGFILE, ino);
                        is_faulty += 1;
                }

                //simulate truncated write
                if (rand() % FFAILRATE == 0) {
                        res = res / 2;  //buf size cut in half // FIXME
                        log_error("lo_write_buf: Truncated write occurred", ERRLOGFILE, ino);
                        is_faulty += 2;
                }

                fault_next->write_buf(req, ino, in_buf, off, fi);
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
}

fuse_lowlevel_ops fault_operations(fuse_lowlevel_ops &next) {
  // Configs
  json config = getConfig("./config/config.json");
  if (config.contains("faultyIO")) {
      auto configFaulty = config["faultyIO"];
      if(configFaulty.contains("local_log_path")) ERRLOGFILE = configFaulty["local_log_path"];
      if(configFaulty.contains("file_fail_rate")) FFAILRATE = configFaulty["file_fail_rate"];
      if(configFaulty.contains("directory_fail_rate")) DFAILRATE = configFaulty["directory_fail_rate"];
      if(configFaulty.contains("use_seednum")) SEEDNUM = configFaulty["use_seednum"];
      if(configFaulty.contains("seed")) SEEDNUM = configFaulty["seed"];
      if(configFaulty.contains("delay_time")) DELAYTIME = configFaulty["delay_time"];
  }


	fault_next = &next;

	fuse_lowlevel_ops curr = next;
	
	curr.init = fault_init;
  // curr.destroy = metrics_destroy;
	// 
	// curr.read = metrics_read;
	// curr.write_buf = metrics_write_buf;
	// 
	// curr.mkdir = metrics_mkdir;
	// curr.rmdir = metrics_rmdir;

	return curr;
}
