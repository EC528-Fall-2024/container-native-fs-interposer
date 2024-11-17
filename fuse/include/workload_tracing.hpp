#ifndef WORKLOAD_TRACING_HPP_INCLUDED
#define WORKLOAD_TRACING_HPP_INCLUDED

#include <fuse_lowlevel.h>

fuse_lowlevel_ops tracing_operations(fuse_lowlevel_ops &next);

#endif // WORKLOAD_TRACING_HPP_INCLUDED