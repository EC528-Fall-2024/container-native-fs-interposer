#ifndef FAULT_INJECTION_HPP_INCLUDED
#define FAULT_INJECTION_HPP_INCLUDED

#include <fuse_lowlevel.h>
#include "passthrough_hp.hpp"
#include "otel.hpp"

fuse_lowlevel_ops fault_operations(fuse_lowlevel_ops &next);

#endif // FAULT_INJECTION_HPP_INCLUDED
