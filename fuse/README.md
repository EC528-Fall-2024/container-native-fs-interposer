# Utility FUSE File Systems

The FUSE (Filesystem in Userspace) library provides a mechanism for implementing and mounting file systems in user space (as opposed to the kernel space). One application of FUSE is to easily develop utility file systems that act as a wrapper around and extends the functionalities of an underlying file system.  

This folder contains implementations of utility FUSE file systems with the following functionalities:  
1) Workload tracing  
2) Workload metric collection  
3) Faulty I/O   
4) Throttle I/O  

## Installation

1) Install [libfuse](https://github.com/libfuse/libfuse)  
2) Install OpenTelemetry  
3) cd container-native-fs-interposer/fuse/  
4) Set up build directory: meson setup build  
5) Build the project: meson compile -C build

## Running file system  

1) cd container-native-fs-interposer/fuse  
2) mkdir /tmp/test-fuse-fs   
2) Run the file system: ./build/interposer /tmp/test-fuse-fs  
4) Use mounted file system: cd /tmp/test-fuse-fs  
3) Unmount file system: fusermount -u /tmp/test-fuse-fs  
