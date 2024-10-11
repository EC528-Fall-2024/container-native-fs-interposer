#include "nop.h"

static struct fuse_lowlevel_ops *do_next;

static void do_init(void *userdata, struct fuse_conn_info *conn) {
  do_next->init(userdata, conn);
};

static void do_destroy(void *userdata) { do_next->destroy(userdata); };

static void do_lookup(fuse_req_t req, fuse_ino_t parent, const char *name) {
  do_next->lookup(req, parent, name);
};

static void do_forget(fuse_req_t req, fuse_ino_t ino, uint64_t nlookup) {
  do_next->forget(req, ino, nlookup);
};

static void do_getattr(fuse_req_t req, fuse_ino_t ino,
                       struct fuse_file_info *fi) {
  do_next->getattr(req, ino, fi);
};

static void do_setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr,
                       int to_set, struct fuse_file_info *fi) {
  do_next->setattr(req, ino, attr, to_set, fi);
};

static void do_readlink(fuse_req_t req, fuse_ino_t ino) {
  do_next->readlink(req, ino);
};

static void do_mknod(fuse_req_t req, fuse_ino_t parent, const char *name,
                     mode_t mode, dev_t rdev) {
  do_next->mknod(req, parent, name, mode, rdev);
};

static void do_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name,
                     mode_t mode) {
  do_next->mkdir(req, parent, name, mode);
};

static void do_unlink(fuse_req_t req, fuse_ino_t parent, const char *name) {
  do_next->unlink(req, parent, name);
};

static void do_rmdir(fuse_req_t req, fuse_ino_t parent, const char *name) {
  do_next->rmdir(req, parent, name);
};

static void do_symlink(fuse_req_t req, const char *link, fuse_ino_t parent,
                       const char *name) {
  do_next->symlink(req, link, parent, name);
};

static void do_rename(fuse_req_t req, fuse_ino_t parent, const char *name,
                      fuse_ino_t newparent, const char *newname,
                      unsigned int flags) {
  do_next->rename(req, parent, name, newparent, newname, flags);
};

static void do_link(fuse_req_t req, fuse_ino_t ino, fuse_ino_t newparent,
                    const char *newname) {
  do_next->link(req, ino, newparent, newname);
};

static void do_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi) {
  do_next->open(req, ino, fi);
};

static void do_read(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                    struct fuse_file_info *fi) {
  do_next->read(req, ino, size, off, fi);
};

static void do_write(fuse_req_t req, fuse_ino_t ino, const char *buf,
                     size_t size, off_t off, struct fuse_file_info *fi) {
  do_next->write(req, ino, buf, size, off, fi);
};

static void do_flush(fuse_req_t req, fuse_ino_t ino,
                     struct fuse_file_info *fi) {
  do_next->flush(req, ino, fi);
};

static void do_release(fuse_req_t req, fuse_ino_t ino,
                       struct fuse_file_info *fi) {
  do_next->release(req, ino, fi);
};

static void do_fsync(fuse_req_t req, fuse_ino_t ino, int datasync,
                     struct fuse_file_info *fi) {
  do_next->fsync(req, ino, datasync, fi);
};

static void do_opendir(fuse_req_t req, fuse_ino_t ino,
                       struct fuse_file_info *fi) {
  do_next->opendir(req, ino, fi);
};

static void do_readdir(fuse_req_t req, fuse_ino_t ino, size_t size, off_t off,
                       struct fuse_file_info *fi) {
  do_next->readdir(req, ino, size, off, fi);
};

static void do_releasedir(fuse_req_t req, fuse_ino_t ino,
                          struct fuse_file_info *fi) {
  do_next->releasedir(req, ino, fi);
};

static void do_fsyncdir(fuse_req_t req, fuse_ino_t ino, int datasync,
                        struct fuse_file_info *fi) {
  do_next->fsyncdir(req, ino, datasync, fi);
};

static void do_statfs(fuse_req_t req, fuse_ino_t ino) {
  do_next->statfs(req, ino);
};

static void do_setxattr(fuse_req_t req, fuse_ino_t ino, const char *name,
                        const char *value, size_t size, int flags) {
  do_next->setxattr(req, ino, name, value, size, flags);
};

static void do_getxattr(fuse_req_t req, fuse_ino_t ino, const char *name,
                        size_t size) {
  do_next->getxattr(req, ino, name, size);
};

static void do_listxattr(fuse_req_t req, fuse_ino_t ino, size_t size) {
  do_next->listxattr(req, ino, size);
};

static void do_removexattr(fuse_req_t req, fuse_ino_t ino, const char *name) {
  do_next->removexattr(req, ino, name);
};

static void do_access(fuse_req_t req, fuse_ino_t ino, int mask) {
  do_next->access(req, ino, mask);
};

static void do_create(fuse_req_t req, fuse_ino_t parent, const char *name,
                      mode_t mode, struct fuse_file_info *fi) {
  do_next->create(req, parent, name, mode, fi);
};

static void do_getlk(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi,
                     struct flock *lock) {
  do_next->getlk(req, ino, fi, lock);
};

static void do_setlk(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi,
                     struct flock *lock, int sleep) {
  do_next->setlk(req, ino, fi, lock, sleep);
};

static void do_bmap(fuse_req_t req, fuse_ino_t ino, size_t blocksize,
                    uint64_t idx) {
  do_next->bmap(req, ino, blocksize, idx);
};

static void do_ioctl(fuse_req_t req, fuse_ino_t ino, unsigned int cmd,
                     void *arg, struct fuse_file_info *fi, unsigned flags,
                     const void *in_buf, size_t in_bufsz, size_t out_bufsz) {
  do_next->ioctl(req, ino, cmd, arg, fi, flags, in_buf, in_bufsz, out_bufsz);
};

static void do_poll(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi,
                    struct fuse_pollhandle *ph) {
  do_next->poll(req, ino, fi, ph);
};

static void do_write_buf(fuse_req_t req, fuse_ino_t ino,
                         struct fuse_bufvec *bufv, off_t off,
                         struct fuse_file_info *fi) {
  do_next->write_buf(req, ino, bufv, off, fi);
};

static void do_retrieve_reply(fuse_req_t req, void *cookie, fuse_ino_t ino,
                              off_t offset, struct fuse_bufvec *bufv) {
  do_next->retrieve_reply(req, cookie, ino, offset, bufv);
};

static void do_forget_multi(fuse_req_t req, size_t count,
                            struct fuse_forget_data *forgets) {
  do_next->forget_multi(req, count, forgets);
};

static void do_flock(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fi,
                     int op) {
  do_next->flock(req, ino, fi, op);
};

static void do_fallocate(fuse_req_t req, fuse_ino_t ino, int mode, off_t offset,
                         off_t length, struct fuse_file_info *fi) {
  do_next->fallocate(req, ino, mode, offset, length, fi);
};

static void do_readdirplus(fuse_req_t req, fuse_ino_t ino, size_t size,
                           off_t off, struct fuse_file_info *fi) {
  do_next->readdirplus(req, ino, size, off, fi);
};

static void do_copy_file_range(fuse_req_t req, fuse_ino_t ino_in, off_t off_in,
                               struct fuse_file_info *fi_in, fuse_ino_t ino_out,
                               off_t off_out, struct fuse_file_info *fi_out,
                               size_t len, int flags) {
  do_next->copy_file_range(req, ino_in, off_in, fi_in, ino_out, off_out, fi_out,
                           len, flags);
};

static void do_lseek(fuse_req_t req, fuse_ino_t ino, off_t off, int whence,
                     struct fuse_file_info *fi) {
  do_next->lseek(req, ino, off, whence, fi);
};

struct fuse_lowlevel_ops nop_operations(struct fuse_lowlevel_ops &next) {
  do_next = &next;

  struct fuse_lowlevel_ops do_ops = {
    .init = do_init, .destroy = do_destroy, .lookup = do_lookup,
    .forget = do_forget, .getattr = do_getattr, .setattr = do_setattr,
    .readlink = do_readlink, .mknod = do_mknod, .mkdir = do_mkdir,
    .unlink = do_unlink, .rmdir = do_rmdir, .symlink = do_symlink,
    .rename = do_rename, .link = do_link, .open = do_open, .read = do_read,
    .write = do_write, .flush = do_flush, .release = do_release,
    .fsync = do_fsync, .opendir = do_opendir, .readdir = do_readdir,
    .releasedir = do_releasedir, .fsyncdir = do_fsyncdir, .statfs = do_statfs,
    .setxattr = do_setxattr, .getxattr = do_getxattr, .listxattr = do_listxattr,
    .removexattr = do_removexattr, .access = do_access, .create = do_create,
    .getlk = do_getlk, .setlk = do_setlk, .bmap = do_bmap, .ioctl = do_ioctl,
    .poll = do_poll, .write_buf = do_write_buf,
    .retrieve_reply = do_retrieve_reply, .forget_multi = do_forget_multi,
    .flock = do_flock, .fallocate = do_fallocate, .readdirplus = do_readdirplus,
    .copy_file_range = do_copy_file_range, .lseek = do_lseek,
  };

  return do_ops;
}
