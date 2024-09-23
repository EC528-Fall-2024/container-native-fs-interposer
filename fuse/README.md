## Utility FUSE File Systems

The FUSE (Filesystem in Userspace) library provides a mechanism for implementing and mounting file systems in user space (as opposed to the kernel space). One application of FUSE is to easily develop utility file systems that act as a wrapper around and extends the functionalities of an underlying file system.  

This folder contains implementations of utility FUSE file systems with the following functionalities:  
1) Workload tracing  
2) Workload metric collection  
3) Faulty I/O   
4) Throttle I/O  
5) Fake I/O

### Install and set up fuse

1) Install dependencies  
sudo apt update  
sudo apt install fuse libfuse-dev  

Depending on your environment, you may need to install these packages:  
sudo apt install build-essential pkg-config kmod  

2) Load FUSE kernel module    
modprobe fuse  

3) Compile passthrough example  
gcc -Wall passthrough.c -o passthrough -D_FILE_OFFSET_BITS=64 -I/usr/include/fuse -lfuse -pthread

