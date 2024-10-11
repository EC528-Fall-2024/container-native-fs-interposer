#define FUSE_USE_VERSION FUSE_MAKE_VERSION(3, 12)
#include <fuse_lowlevel.h>

struct fuse_lowlevel_ops nop_operations(struct fuse_lowlevel_ops &_next);
