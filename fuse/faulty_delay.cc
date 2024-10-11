#include <cstdlib>
#include <ctime>
#define FUSE_USE_VERSION FUSE_MAKE_VERSION(3, 12)

#define _GNU_SOURCE

#include <errno.h>
#include <fuse_lowlevel.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "faulty_delay.h"

static fuse_lowlevel_ops *next;

void log_error(const char *error_message) {
  // Get the current time
  time_t rawtime;
  struct tm *timeinfo;
  char time_buffer[80];           // string for formatted time
  time(&rawtime);                 // Get current time
  timeinfo = localtime(&rawtime); // Convert to local time format
  strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S",
           timeinfo); // Format the time as: "YYYY-MM-DD HH:MM:SS"
  // Write the error message with a timestamp to the file
  fprintf(stderr, "[%s] ERROR: %s\n", time_buffer, error_message);
}

static void read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                 struct fuse_file_info *fi) {
  // simulate truncated reads
  if (rand() % 10 == 0) { // 10% probability of a short read
    size = size / 2;      // Return half the requested size
    log_error("lo_read: Truncated read occured");
  }

  // simulate failed io
  if (rand() % 10 == 0) { // Example: 10% probability of error
    fuse_reply_err(req, EIO);
    log_error("lo_read: An unexpected failure occurred");
    return; // Exit the function early after sending the error
  }

  // simulate delayed io
  if (rand() % 5 == 0) { // Example: 20% probability of delay
    sleep(5);            // Delay for 5 seconds
    log_error("lo_read: An unexpected delay occurred");
  }

  next->read(req, ino, size, off, fi);
}

static void write_buf(fuse_req_t req, fuse_ino_t ino, struct fuse_bufvec *bufv,
                      off_t off, struct fuse_file_info *fi) {
  // simulate IO error
  if (rand() % 10 == 0) { // Example: 10% probability of error
    fuse_reply_err(req, EIO);
    log_error("lo_write_buf: An unexpected failure occurred");
    return; // Exit the function early after sending the error
  }

  // simulate delayed io
  if (rand() % 5 == 0) { // Example: 20% probability of delay
    sleep(5);            // Delay for 5 seconds
    log_error("lo_write_buf: An unexpected delay occurred");
  }

  next->write_buf(req, ino, bufv, off, fi);
}

static void getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
  // faulty attribute return
  if (rand() % 10 == 0) {     // Example: 10% probability
    fuse_reply_err(req, EIO); // Return a generic I/O error
    log_error("lo_getattr: No attributes returned");
    return; // Exit early
  }

  next->getattr(req, ino, fi);
}

static void setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr,
                    int valid, struct fuse_file_info *fi) {
  // simulate fault
  if (rand() % 10 == 0) {     // Example: 10% probability
    fuse_reply_err(req, EIO); // Return a generic I/O error
    log_error("lo_setattr: No attributes set");
    return; // Exit early
  }

  next->setattr(req, ino, attr, valid, fi);
}

static void fsync(fuse_req_t req, fuse_ino_t ino, int datasync,
                  struct fuse_file_info *fi) {
  // Simulate failure
  if (rand() % 10 == 0) {     // Example: 10% probability
    fuse_reply_err(req, EIO); // Return a generic I/O error
    log_error("lo_fsync: An unexpected failure occurred");
    return; // Exit early
  }

  next->fsync(req, ino, datasync, fi);
}

fuse_lowlevel_ops faulty_delay_operations(fuse_lowlevel_ops &_next) {
  srand(time(NULL));
  // Create ostream span exporter instance
  next = &_next;

  fuse_lowlevel_ops curr = _next;

  curr.read = read;
  curr.write_buf = write_buf;
  curr.getattr = getattr;
  curr.setattr = setattr;
  curr.fsync = fsync;

  return curr;
}
