
## FUSE


## Install fuse
These steps were executed on Ubuntu.

1) Install dependencies
sudo apt update
sudo apt install fuse libfuse-dev

Depending on your environment, you may need to install these packages:
sudo apt install build-essential pkg-config kmod


2) Compile passthrough example
gcc -Wall passthrough.c -o passthrough -D_FILE_OFFSET_BITS=64 -I/usr/include/fuse -lfuse -pthread

