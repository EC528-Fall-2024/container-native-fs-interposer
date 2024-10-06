#ifndef PASSTHROUGH_H
#define PASSTHROUGH_H

#ifdef __cplusplus
extern "C" {
#endif

#define FUSE_USE_VERSION 31

#include <fuse.h>

extern int fill_dir_plus;

void *xmp_init(struct fuse_conn_info *conn,
		        struct fuse_config *cfg);

int xmp_getattr(const char *path, struct stat *stbuf,
                struct fuse_file_info *fi);
                
int xmp_access(const char *path, int mask);

int xmp_readlink(const char *path, char *buf, size_t size);

int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                off_t offset, struct fuse_file_info *fi,
                enum fuse_readdir_flags flags);

int xmp_mknod(const char *path, mode_t mode, dev_t rdev);

int xmp_mkdir(const char *path, mode_t mode);

int xmp_unlink(const char *path);

int xmp_rmdir(const char *path);

int xmp_symlink(const char *from, const char *to);

int xmp_rename(const char *from, const char *to, unsigned int flags);

int xmp_link(const char *from, const char *to);

int xmp_chmod(const char *path, mode_t mode,
		        struct fuse_file_info *fi);

int xmp_chown(const char *path, uid_t uid, gid_t gid,
    		    struct fuse_file_info *fi);

int xmp_truncate(const char *path, off_t size,
                struct fuse_file_info *fi);

#ifdef HAVE_UTIMENSAT
int xmp_utimens(const char *path, const struct timespec ts[2],
                struct fuse_file_info *fi);
#endif

int xmp_create(const char *path, mode_t mode,
                struct fuse_file_info *fi);

int xmp_open(const char *path, struct fuse_file_info *fi);

int xmp_read(const char *path, char *buf, size_t size, off_t offset,
            struct fuse_file_info *fi);

int xmp_write(const char *path, const char *buf, size_t size,
                off_t offset, struct fuse_file_info *fi);

int xmp_statfs(const char *path, struct statvfs *stbuf);

int xmp_release(const char *path, struct fuse_file_info *fi);

int xmp_fsync(const char *path, int isdatasync,
                struct fuse_file_info *fi);

#ifdef HAVE_POSIX_FALLOCATE
int xmp_fallocate(const char *path, int mode,
                    off_t offset, off_t length, struct fuse_file_info *fi);
#endif

#ifdef HAVE_SETXATTR
int xmp_setxattr(const char *path, const char *name, const char *value,
                size_t size, int flags);
#endif

int xmp_getxattr(const char *path, const char *name, char *value,
                size_t size);

int xmp_listxattr(const char *path, char *list, size_t size);

int xmp_removexattr(const char *path, const char *name);

#ifdef HAVE_COPY_FILE_RANGE
ssize_t xmp_copy_file_range(const char *path_in,
                            struct fuse_file_info *fi_in,
                            off_t offset_in, const char *path_out,
                            struct fuse_file_info *fi_out,
                            off_t offset_out, size_t len, int flags);
#endif

off_t xmp_lseek(const char *path, off_t off, int whence, struct fuse_file_info *fi);

extern const struct fuse_operations xmp_oper;

#ifdef __cplusplus
}
#endif

#endif // PASSTHROUGH_H
