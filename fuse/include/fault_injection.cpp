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
