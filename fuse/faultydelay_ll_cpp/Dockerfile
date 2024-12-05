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
    libudev-dev libevdev-dev libsystemd-dev libglib2.0-dev libjson-glib-dev libunistring-dev check valgrind swig libgtest-dev libbenchmark-dev protobuf-compiler libprotobuf-dev autoconf libtool libgmock-dev nlohmann-json3-dev\
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install -y \
    libfuse2 pkg-config build-essential meson ninja-build signify-openbsd python3 python3-pip nano bash bash-completion git\
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install pytest

#use bash shell (for autocompletion)
SHELL ["/bin/bash", "-c"]
RUN echo "source /etc/profile.d/bash_completion.sh" >> /etc/profile

# Copy your FUSE application code to the container
COPY . /usr/src/myapp

# Change working directory
WORKDIR /usr/src/myapp

#Gtest install (currently building otel without gtest cause of other bugs, so potentially dont need)

# RUN cd /usr/src/googletest/googlemock \
#     && cmake . \
#     && make \
#     && mv lib/*.a /usr/lib/

# RUN cd /usr/src/gtest \
#     && cmake -DBUILD_GMOCK=ON . \
#     && make \
#     && cp lib/*.a /usr/lib

# #build grpc
# RUN cd grpc \
#     && mkdir -p cmake/build \
#     && cd cmake/build \
#     && cmake ../.. \
#     && make -j4 \
#     && make install

# #build abseil dependency for grpc
# RUN cd abseil-cpp \
#     && mkdir build \
#     && cd build \
#     && cmake .. -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
#     && make -j4 \
#     && make install

#Otel install lines
RUN cd opentelemetry-cpp \
    && mkdir build \
    && cd build \
    && cmake -DOPENTELEMETRY_INSTALL=ON -DWITH_EXPORTER_OSTREAM=ON -DWITH_TRACE=ON -DWITH_RESOURCE=ON -DBUILD_TESTING=OFF .. \
    && cmake --build . \
    && make install

#-DWITH_EXPORTER_OSTREAM=ON -DWITH_TRACE=ON -DWITH_RESOURCE=ON -DBUILD_TESTING=OFF 

#git clone --recurse-submodules https://github.com/open-telemetry/opentelemetry-cpp
#cd opentelemetry-cpp
#mkdir build && cd build && cmake ..
#cmake -DWITH_OTLP_GRPC=ON -DWITH_LOGS_PREVIEW=ON -DOPENTELEMETRY_INSTALL=ON -DGMOCK_LIB=/usr/local/lib/libgmock.a -DGMOCK_MAIN_LIB=/usr/local/lib/libgmock_main.a -DBUILD_TESTING=OFF .. \
#cmake --build . --target all
#ctest
#cmake --install . --prefix /<install-root>/ (optional)

#Verify tarball w signify
RUN signify-openbsd -V -m libfuse/fuse-3.16.2.tar.gz -p libfuse/fuse-3.16.pub

RUN tar xzf libfuse/fuse-3.16.2.tar.gz 

#unpack tarball and setu libfuse build with meson
RUN cd fuse-3.16.2 \
    && meson setup build \
    && ninja -C build install

RUN mkdir mountpoint \
    && g++ -Wall -D_FILE_OFFSET_BITS=64 faulty_ll.cpp include/config_parser.cpp -I/usr/local/include -I./include -L/usr/local/lib -lopentelemetry_trace -lopentelemetry_resources -lopentelemetry_exporter_ostream_span -lopentelemetry_common `pkg-config fuse3 --cflags --libs` -o faulty_ll \
    && ldconfig \
    && mkdir testmount

#in the shell all i need to do is mount it to mountpoint

#name executable and run it 
# Build the FUSE application (if applicable)
# RUN g++ -o my_fuse_app -I//usr/local/include/fuse3 test.cpp 
#g++ -Wall -D_FILE_OFFSET_BITSl=64 faulty_ll.cpp -I//usr/local/include/fuse3 `pkg-config fuse3 --cflags --libs` -o faulty_ll
#[-I/include/path]

# Run the shell when the container starts
CMD ["bash"]

