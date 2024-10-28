# Use an Ubuntu base image
FROM ubuntu:20.04

# Install required packages including libfuse
RUN apt-get update && apt-get install -y \
    gcc g++ make fuse3 libfuse-dev \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install -y \
    cmake\
    && echo America && echo New_York\
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install -y \
    php\
    && echo America && echo New_York\
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install -y \
    libudev-dev libevdev-dev libsystemd-dev libglib2.0-dev libjson-glib-dev libunistring-dev check valgrind swig\
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install -y \
    libfuse2 pkg-config build-essential meson ninja-build signify-openbsd python3 python3-pip\
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install pytest
# Copy your FUSE application code to the container
COPY . /usr/src/myapp

# Change working directory
WORKDIR /usr/src/myapp

#Verify tarball w signify
RUN signify-openbsd -V -m libfuse/fuse-3.16.2.tar.gz -p libfuse/fuse-3.16.pub

RUN tar xzf libfuse/fuse-3.16.2.tar.gz 

#unpack tarball and setu libfuse build with meson
RUN cd fuse-3.16.2 \
    && meson setup build \
    && ninja -C build install

RUN mkdir mountpoint \
    && g++ -Wall -D_FILE_OFFSET_BITSl=64 faulty_ll.cpp -I//usr/local/include/fuse3 `pkg-config fuse3 --cflags --libs` -o faulty_ll \
    && ldconfig \
    && mkdir testmount

#in the shell all i need to do is mount it to mountpoint

#name executable and run it 
# Build the FUSE application (if applicable)
RUN g++ -o my_fuse_app -I//usr/local/include/fuse3 test.cpp 
#g++ -Wall -D_FILE_OFFSET_BITSl=64 faulty_ll.cpp -I//usr/local/include/fuse3 `pkg-config fuse3 --cflags --libs` -o faulty_ll
#[-I/include/path]

# Run the FUSE application when the container starts
CMD ["./my_fuse_app"] 

