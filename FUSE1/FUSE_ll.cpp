#define FUSE_USE_VERSION 34

#include <fuse3/fuse_lowlevel.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <sys/file.h>
#include <sys/xattr.h>

/* We are re-using pointers to our `struct lo_inode` and `struct
   lo_dirp` elements as inodes. This means that we must be able to
   store uintptr_t values in a fuse_ino_t variable. The following
   incantation checks this condition at compile time. */
#if defined(__GNUC__) && (__GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 6) && !defined __cplusplus
_Static_assert(sizeof(fuse_ino_t) >= sizeof(uintptr_t),
	       "fuse_ino_t too small to hold uintptr_t values!");
#else
struct _uintptr_to_must_hold_fuse_ino_t_dummy_struct \
	{ unsigned _uintptr_to_must_hold_fuse_ino_t:
			((sizeof(fuse_ino_t) >= sizeof(uintptr_t)) ? 1 : -1); };
#endif

enum {
    CACHE_NEVER,
    CACHE_NORMAL,
    CACHE_ALWAYS,
};

struct lo_inode {
    struct lo_inode* next;
    struct lo_inode* prev;
    int fd;
    ino_t ino;
    dev_t dev;
    uint64_t refcount;
};

struct lo_data {
    pthread_mutex_t mutex;
    int debug;
    int writeback;
    int flock;
    int xattr;
    char *source;
    double timeout;
    int cache;
    int timeout_set;
    struct lo_inode root;   /* protected my lo->mutex */
};

static const struct fuse_lowlevel_ops operations = { };     //tobe implemented

int main(int argc, char *argv[])
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    struct fuse_session *se;      
    struct fuse_cmdline_opts opts;
    struct fuse_loop_config config;
    struct lo_data lo = { .debug = 0,
                          .writeback = 0};
    
    int ret = -1;       //return code

    umask(0);

    pthread_mutex_init(&lo.mutex, NULL);         //not gonna support multiple write for now
    lo.root.next = lo.root.prev = &lo.root;
    lo.root.fd = -1;
    lo.cache = CACHE_NORMAL;

    if(fuse_parse_cmdline(&args, &opts) != 0)
        return -1;

    if(opts.show_help) {
        printf("Usage: %s [options] <mountpoint>\n\n", argv[0]);
        fuse_cmdline_help();
        fuse_lowlevel_help();
        ret = 0;
        goto err_out1;
    } else if (opts.show_version) {
        printf("FUSE library version %s\n", fuse_pkgversion());
        fuse_lowlevel_version();
        ret = 0;
        goto err_out1;
    }

    if(opts.mountpoint == NULL) {
        printf("Usage: %s [options] <mountpoint>\n\n", argv[0]);
        printf("       %s --help\n", argv[0]);
        ret = 1;
        goto err_out1;
    }
    
    printf("STEP 1: opts.mountpoint is not NULL : %s\n", opts.mountpoint);
    se = fuse_session_new(&args, &operations, sizeof(operations), NULL);
    
    if( se == NULL ) {
        printf("fuse_session_new() failed \n");
        ret = -1;
        return -1;
    }
    
    printf("STEP 2: fuse_session_new() success, fuse_session (%p) \n", se);


err_out1:
    free(opts.mountpoint);
    fuse_opt_free_args(&args);

    if(lo.root.fd >= 0)
        close(lo.root.fd);

    free(lo.source);
    return ret ? 1 : 0; 
}
