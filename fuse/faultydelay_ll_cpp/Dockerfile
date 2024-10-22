# Use an Ubuntu base image
FROM ubuntu:20.04

# Install required packages including libfuse
RUN apt-get update && apt-get install -y \
    gcc gpp make fuse3 libfuse-dev\
    && rm -rf /var/lib/apt/lists/*


RUN apt-get update && apt-get install -y \
    libfuse2 build-essential meson ninja-build signify-openbsd python3 python3-pip\
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install pytest
# Copy your FUSE application code to the container
COPY . /usr/src/myapp

# Change working directory
WORKDIR /usr/src/myapp

#Verify tarball w signify
RUN signify-openbsd -V -m libfuse/fuse-3.16.2.tar.gz -p libfuse/fuse-3.16.pub

#unpack tarball and setu libfuse build with meson
RUN tar xzf libfuse/fuse-3.16.2.tar.gz
RUN cd fuse-3.16.2

# RUN meson setup build

# #RUN mkdir build 
# RUN cd build
# #RUN meson setup ..

# #RUN meson setup build
# # RUN cd build
# RUN meson compile
# RUN cd ..
# RUN ninja -C build install

# #Build libfuse, test it, and install it
# RUN meson compile
# #RUN sudo chown root:root util/fusermount3
# #RUN sudo chmod 4755 util/fusermount3
# RUN pwd
# #RUN python3 -m pytest ~/libfuse/fuse-3.16.2/test/
# RUN ninja install

#name executable and run it 
# Build the FUSE application (if applicable)
RUN g++ -o my_fuse_app test.cpp 
#[-I/include/path]

# Run the FUSE application when the container starts
CMD ["./my_fuse_app"] 

