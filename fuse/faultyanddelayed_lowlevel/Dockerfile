# # Use a multi-arch Ubuntu base image (supports ARM and x86)
# FROM --platform=linux/arm64/v8 ubuntu:20.04

# # Install necessary dependencies for building libfuse and running your app
# ARG DEBIAN_FRONTEND=noninteractive
# RUN apt-get update && apt-get install -y \
#     build-essential \
#     pkg-config \
#     git \
#     libtool \
#     autotools-dev \
#     automake \
#     autoconf \
#     fuse \
#     wget \
#     python3 \
#     meson \
#     ninja-build \
#     sudo \
#     && rm -rf /var/lib/apt/lists/*

# # Download and install libfuse from source (multi-arch compatible)
# #RUN apt-get update && apt-get install -y libudev-dev//install udev
# RUN wget https://github.com/libfuse/libfuse/releases/download/fuse-3.13.0/fuse-3.13.0.tar.xz \
#     && tar -xf fuse-3.13.0.tar.xz \
#     && cd fuse-3.13.0 \
#     && mkdir build \
#     && cd build \
#     && meson .. \
#     && ninja \
#     && sudo ninja install

# # Copy your FUSE application code to the container
# COPY . /usr/src/myapp

# # Change working directory
# WORKDIR /usr/src/myapp

# # Compile FUSE application
# RUN gcc -o my_fuse_app faulty_ll.c -lfuse3

# # Define the default command to run your FUSE application
# CMD ["./my_fuse_app"]

# For non arm architecture (I think)

# Use an Ubuntu base image
FROM ubuntu:20.04

# Install required packages including libfuse
RUN apt-get update && apt-get install -y \
    fuse libfuse2 libfuse-dev gcc make \
    && rm -rf /var/lib/apt/lists/*

# Copy your FUSE application code to the container
COPY . /usr/src/myapp

# Change working directory
WORKDIR /usr/src/myapp

# Build the FUSE application (if applicable)
RUN gcc -o my_fuse_app faulty_ll.c -lfuse

# Run the FUSE application when the container starts
CMD ["./my_fuse_app"]

