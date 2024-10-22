#ifndef PASSTHROUGH_HP_HPP_INCLUDED
#define PASSTHROUGH_HP_HPP_INCLUDED

// C includes
#include "passthrough_hp.hpp"
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <ftw.h>
#include <fuse3/fuse_lowlevel.h>
#include <inttypes.h>
#include <string.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <sys/xattr.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>

// C++ includes
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <list>
#include "cxxopts.hpp"
#include <mutex>
#include <fstream>
#include <thread>
#include <iomanip>

using namespace std;

#define SFS_DEFAULT_THREADS "-1" // take libfuse value as default
#define SFS_DEFAULT_CLONE_FD "0"

/* We are re-using pointers to our `struct sfs_inode` and `struct
   sfs_dirp` elements as inodes and file handles. This means that we
   must be able to store pointer a pointer in both a fuse_ino_t
   variable and a uint64_t variable (used for file handles). */
static_assert(sizeof(fuse_ino_t) >= sizeof(void*),
              "void* must fit into fuse_ino_t");
static_assert(sizeof(fuse_ino_t) >= sizeof(uint64_t),
              "fuse_ino_t must be at least 64 bits");


/* Forward declarations */
struct Inode;
Inode& get_inode(fuse_ino_t ino);
static void forget_one(fuse_ino_t ino, uint64_t n);

// Uniquely identifies a file in the source directory tree. This could
// be simplified to just ino_t since we require the source directory
// not to contain any mountpoints. This hasn't been done yet in case
// we need to reconsider this constraint (but relaxing this would have
// the drawback that we can no longer re-use inode numbers, and thus
// readdir() would need to do a full lookup() in order to report the
// right inode number).
typedef std::pair<ino_t, dev_t> SrcId;

// Define a hash function for SrcId
namespace std {
    template<>
    struct hash<SrcId> {
        size_t operator()(const SrcId& id) const {
            return hash<ino_t>{}(id.first) ^ hash<dev_t>{}(id.second);
        }
    };
}

// Maps files in the source directory tree to inodes
typedef std::unordered_map<SrcId, Inode> InodeMap;

struct Inode {
    int fd {-1};
    dev_t src_dev {0};
    ino_t src_ino {0};
    int generation {0};
    uint64_t nopen {0};
    uint64_t nlookup {0};
    std::mutex m;

    // Delete copy constructor and assignments. We could implement
    // move if we need it.
    Inode() = default;
    Inode(const Inode&) = delete;
    Inode(Inode&& inode) = delete;
    Inode& operator=(Inode&& inode) = delete;
    Inode& operator=(const Inode&) = delete;

    ~Inode() {
        if(fd > 0)
            close(fd);
    }
};

#define FUSE_BUF_COPY_FLAGS                      \
        (fs.nosplice ?                           \
            FUSE_BUF_NO_SPLICE :                 \
            static_cast<fuse_buf_copy_flags>(0))


struct Fs {
    // Must be acquired *after* any Inode.m locks.
    std::mutex mutex;
    InodeMap inodes; // protected by mutex
    Inode root;
    double timeout;
    bool debug;
    bool debug_fuse;
    bool foreground;
    std::string source;
    size_t blocksize;
    dev_t src_dev;
    bool nosplice;
    bool nocache;
    size_t num_threads;
    bool clone_fd;
    std::string fuse_mount_options;
    bool direct_io;
};
static Fs fs{};

cxxopts::ParseResult parse_options(int argc, char **argv);
void maximize_fd_limit();
void assign_operations(fuse_lowlevel_ops &sfs_oper);
int setup_fuse(int argc, char *argv[], fuse_lowlevel_ops &oper);


#endif // PASSTHROUGH_HP_HPP_INCLUDED
