#define _GNU_SOURCE
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

static const struct fuse_opt fuse_attr[] = {
	{ "writeback",
	  offsetof(struct lo_data, writeback), 1 },
	{ "no_writeback",
	  offsetof(struct lo_data, writeback), 0 },
	{ "source=%s",
	  offsetof(struct lo_data, source), 0 },
	{ "flock",
	  offsetof(struct lo_data, flock), 1 },
	{ "no_flock",
	  offsetof(struct lo_data, flock), 0 },
	{ "xattr",
	  offsetof(struct lo_data, xattr), 1 },
	{ "no_xattr",
	  offsetof(struct lo_data, xattr), 0 },
	{ "timeout=%lf",
	  offsetof(struct lo_data, timeout), 0 },
	{ "timeout=",
	  offsetof(struct lo_data, timeout_set), 1 },
	{ "cache=never",
	  offsetof(struct lo_data, cache), CACHE_NEVER },
	{ "cache=auto",
	  offsetof(struct lo_data, cache), CACHE_NORMAL },
	{ "cache=always",
	  offsetof(struct lo_data, cache), CACHE_ALWAYS },

	FUSE_OPT_END
};

/* 
 *  following two data structures are not needed for now
 *  to learn more go to example code provided by libfuse
 * */
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


static void simple_init(void *userdata,
                        struct fuse_conn_info *conn)
{
    
}

/*
 *  this is where all the function this fuse going to support
 * */
static const struct fuse_lowlevel_ops operations = {
    
};     //tobe implemented

/*
 * only support one parameter [mountpoint]. which is the cloned_fd
 * all other configurations will be ignored
 * fuse config will be hard coded other than field[cloned_fd]. Therefore field[max_idle_threads] will be 1
 * */
int main(int argc, char *argv[])
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    struct fuse_session *se;      
    struct fuse_cmdline_opts opts;
    struct fuse_loop_config config;
    struct lo_data usr_data;
    int ret = -1;

    //pre initializing usr_data
    pthread_mutex_init(&usr_data.mutex, NULL);         //not gonna support multiple write for now
    usr_data.root.next = usr_data.root.prev = &usr_data.root;
    usr_data.root.fd = -1;
    usr_data.cache = CACHE_NORMAL;

    config.max_idle_threads = 1;

    if(fuse_parse_cmdline(&args, &opts) != 0)
        return -1;

    fuse_opt_parse(&args, &usr_data, fuse_attr, NULL);


    //clone the fd
    config.clone_fd = opts.clone_fd;
    //initializing usr_data after parsed cmdline
    usr_data.debug = 0;
    usr_data.root.refcount = 2;
    struct stat stat;
    int res = lstat(usr_data.source, &stat);
    if ( res == -1 ) {
        printf("cannot mount directory \n");
        exit(1);
    }
    usr_data.root.fd = open(usr_data.source, O_PATH);


    if(opts.show_help) {
        printf("Usage: %s [options] <mountpoint>\n\n", argv[0]);
        fuse_cmdline_help();
        fuse_lowlevel_help();
        ret = 0;
        return ret;
    } else if (opts.show_version) {
        printf("FUSE library version %s\n", fuse_pkgversion());
        fuse_lowlevel_version();
        ret = 0;
        return ret;    
    }

    if(opts.mountpoint == NULL) {
        printf("Usage: %s [options] <mountpoint>\n\n", argv[0]);
        printf("       %s --help\n", argv[0]);
        ret = 0;
        return ret;
    }
    
    printf("STEP 1: opts.mountpoint is not NULL : %s\n", opts.mountpoint);
    se = fuse_session_new(&args, &operations, sizeof(operations), &usr_data);
    
    if( se == NULL ) {
        printf("fuse_session_new() failed \n");
        ret = -1;
        return ret;
    }
    
    printf("STEP 2: fuse_session_new() success, fuse_session (%p) \n", se);
    if( fuse_set_signal_handlers(se) != 0 ) {
        printf("failed to setup signal_handler for se \n");
        ret = -1;
        return ret;
    }
    
    printf("STEP 3: Setup signal handler OK \n");

    if( fuse_session_mount(se, opts.mountpoint) != 0 ) {
        printf("se failed to mount \n");
        ret = -1;
        return ret;
    }

    printf("STEP 4: fuse_ssession_mount() success \n");
    
    fuse_daemonize(opts.foreground);           //for testing purpose, want to see cmdline output
    
    ret = fuse_session_loop_mt(se, &config);               //block untill ctl+c or fusemount -u (unmount). This is single thread, 
                                               //if want to support multi-thread, should use fuse
    
    fuse_session_unmount(se);
    printf("STEP 5: fuse_session_unmount() \n");

    fuse_remove_signal_handlers(se);
    printf("STEP 6: fuse_remove_signal_handlers(se) \n");

    fuse_session_destroy(se);
    printf("STEP 7: fuse_session_destroy(se) \n");

    fuse_opt_free_args(&args);
    ret = 0;
    return ret;
}
