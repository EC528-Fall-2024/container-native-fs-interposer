#ifndef METRIC_COLLECTION_HPP_INCLUDED
#define METRIC_COLLECTION_HPP_INCLUDED

#include <fuse_lowlevel.h>
#include "passthrough_hp.hpp"
#include "otel.hpp"

fuse_lowlevel_ops metric_operations(fuse_lowlevel_ops &next);

#endif // METRIC_COLLECTION_HPP_INCLUDED