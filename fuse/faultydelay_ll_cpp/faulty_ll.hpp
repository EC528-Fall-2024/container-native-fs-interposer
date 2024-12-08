#ifndef FAULTY_FS_HPP_INCLUDED
#define FAULTY_FS_HPP_INCLUDED

#include <fuse_lowlevel.h>

fuse_lowlevel_ops faulty_operations(fuse_lowlevel_ops &next);

#endif