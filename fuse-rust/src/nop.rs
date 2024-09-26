use crate::fuse::{
    dev_t, flock, fuse_bufvec, fuse_config, fuse_conn_info, fuse_file_info, fuse_fill_dir_t,
    fuse_operations, fuse_pollhandle, fuse_readdir_flags, gid_t, mode_t, off_t, stat, statvfs,
    timespec, uid_t,
};
use std::{
    ffi::{c_char, c_int, c_uint, c_void},
    mem::MaybeUninit,
};

static mut NEXT: MaybeUninit<fuse_operations> = MaybeUninit::uninit();

unsafe extern "C" fn getattr(
    arg1: *const c_char,
    arg2: *mut stat,
    fi: *mut fuse_file_info,
) -> c_int {
    NEXT.assume_init_ref().getattr.unwrap()(arg1, arg2, fi)
}

unsafe extern "C" fn readlink(arg1: *const c_char, arg2: *mut c_char, arg3: usize) -> c_int {
    NEXT.assume_init_ref().readlink.unwrap()(arg1, arg2, arg3)
}

unsafe extern "C" fn mknod(arg1: *const c_char, arg2: mode_t, arg3: dev_t) -> c_int {
    NEXT.assume_init_ref().mknod.unwrap()(arg1, arg2, arg3)
}

unsafe extern "C" fn mkdir(arg1: *const c_char, arg2: mode_t) -> c_int {
    NEXT.assume_init_ref().mkdir.unwrap()(arg1, arg2)
}

unsafe extern "C" fn unlink(arg1: *const c_char) -> c_int {
    NEXT.assume_init_ref().unlink.unwrap()(arg1)
}

unsafe extern "C" fn rmdir(arg1: *const c_char) -> c_int {
    NEXT.assume_init_ref().rmdir.unwrap()(arg1)
}

unsafe extern "C" fn symlink(arg1: *const c_char, arg2: *const c_char) -> c_int {
    NEXT.assume_init_ref().symlink.unwrap()(arg1, arg2)
}

unsafe extern "C" fn rename(arg1: *const c_char, arg2: *const c_char, flags: c_uint) -> c_int {
    NEXT.assume_init_ref().rename.unwrap()(arg1, arg2, flags)
}

unsafe extern "C" fn link(arg1: *const c_char, arg2: *const c_char) -> c_int {
    NEXT.assume_init_ref().link.unwrap()(arg1, arg2)
}

unsafe extern "C" fn chmod(arg1: *const c_char, arg2: mode_t, fi: *mut fuse_file_info) -> c_int {
    NEXT.assume_init_ref().chmod.unwrap()(arg1, arg2, fi)
}

unsafe extern "C" fn chown(
    arg1: *const c_char,
    arg2: uid_t,
    arg3: gid_t,
    fi: *mut fuse_file_info,
) -> c_int {
    NEXT.assume_init_ref().chown.unwrap()(arg1, arg2, arg3, fi)
}

unsafe extern "C" fn truncate(arg1: *const c_char, arg2: off_t, fi: *mut fuse_file_info) -> c_int {
    NEXT.assume_init_ref().truncate.unwrap()(arg1, arg2, fi)
}

unsafe extern "C" fn open(arg1: *const c_char, arg2: *mut fuse_file_info) -> c_int {
    NEXT.assume_init_ref().open.unwrap()(arg1, arg2)
}

unsafe extern "C" fn read(
    arg1: *const c_char,
    arg2: *mut c_char,
    arg3: usize,
    arg4: off_t,
    arg5: *mut fuse_file_info,
) -> c_int {
    NEXT.assume_init_ref().read.unwrap()(arg1, arg2, arg3, arg4, arg5)
}

unsafe extern "C" fn write(
    arg1: *const c_char,
    arg2: *const c_char,
    arg3: usize,
    arg4: off_t,
    arg5: *mut fuse_file_info,
) -> c_int {
    NEXT.assume_init_ref().write.unwrap()(arg1, arg2, arg3, arg4, arg5)
}

unsafe extern "C" fn statfs(arg1: *const c_char, arg2: *mut statvfs) -> c_int {
    NEXT.assume_init_ref().statfs.unwrap()(arg1, arg2)
}

unsafe extern "C" fn flush(arg1: *const c_char, arg2: *mut fuse_file_info) -> c_int {
    NEXT.assume_init_ref().flush.unwrap()(arg1, arg2)
}

unsafe extern "C" fn release(arg1: *const c_char, arg2: *mut fuse_file_info) -> c_int {
    NEXT.assume_init_ref().release.unwrap()(arg1, arg2)
}

unsafe extern "C" fn fsync(arg1: *const c_char, arg2: c_int, arg3: *mut fuse_file_info) -> c_int {
    NEXT.assume_init_ref().fsync.unwrap()(arg1, arg2, arg3)
}

unsafe extern "C" fn setxattr(
    arg1: *const c_char,
    arg2: *const c_char,
    arg3: *const c_char,
    arg4: usize,
    arg5: c_int,
) -> c_int {
    NEXT.assume_init_ref().setxattr.unwrap()(arg1, arg2, arg3, arg4, arg5)
}

unsafe extern "C" fn getxattr(
    arg1: *const c_char,
    arg2: *const c_char,
    arg3: *mut c_char,
    arg4: usize,
) -> c_int {
    NEXT.assume_init_ref().getxattr.unwrap()(arg1, arg2, arg3, arg4)
}

unsafe extern "C" fn listxattr(arg1: *const c_char, arg2: *mut c_char, arg3: usize) -> c_int {
    NEXT.assume_init_ref().listxattr.unwrap()(arg1, arg2, arg3)
}

unsafe extern "C" fn removexattr(arg1: *const c_char, arg2: *const c_char) -> c_int {
    NEXT.assume_init_ref().removexattr.unwrap()(arg1, arg2)
}

unsafe extern "C" fn opendir(arg1: *const c_char, arg2: *mut fuse_file_info) -> c_int {
    NEXT.assume_init_ref().opendir.unwrap()(arg1, arg2)
}

unsafe extern "C" fn readdir(
    arg1: *const c_char,
    arg2: *mut c_void,
    arg3: fuse_fill_dir_t,
    arg4: off_t,
    arg5: *mut fuse_file_info,
    arg6: fuse_readdir_flags,
) -> c_int {
    NEXT.assume_init_ref().readdir.unwrap()(arg1, arg2, arg3, arg4, arg5, arg6)
}

unsafe extern "C" fn releasedir(arg1: *const c_char, arg2: *mut fuse_file_info) -> c_int {
    NEXT.assume_init_ref().releasedir.unwrap()(arg1, arg2)
}

unsafe extern "C" fn fsyncdir(
    arg1: *const c_char,
    arg2: c_int,
    arg3: *mut fuse_file_info,
) -> c_int {
    NEXT.assume_init_ref().fsyncdir.unwrap()(arg1, arg2, arg3)
}

unsafe extern "C" fn init(conn: *mut fuse_conn_info, cfg: *mut fuse_config) -> *mut c_void {
    NEXT.assume_init_ref().init.unwrap()(conn, cfg)
}

unsafe extern "C" fn destroy(private_data: *mut c_void) {
    NEXT.assume_init_ref().destroy.unwrap()(private_data)
}

unsafe extern "C" fn access(arg1: *const c_char, arg2: c_int) -> c_int {
    NEXT.assume_init_ref().access.unwrap()(arg1, arg2)
}

unsafe extern "C" fn create(arg1: *const c_char, arg2: mode_t, arg3: *mut fuse_file_info) -> c_int {
    NEXT.assume_init_ref().create.unwrap()(arg1, arg2, arg3)
}

unsafe extern "C" fn lock(
    arg1: *const c_char,
    arg2: *mut fuse_file_info,
    cmd: c_int,
    arg3: *mut flock,
) -> c_int {
    NEXT.assume_init_ref().lock.unwrap()(arg1, arg2, cmd, arg3)
}

unsafe extern "C" fn utimens(
    arg1: *const c_char,
    tv: *const timespec,
    fi: *mut fuse_file_info,
) -> c_int {
    NEXT.assume_init_ref().utimens.unwrap()(arg1, tv, fi)
}

unsafe extern "C" fn bmap(arg1: *const c_char, blocksize: usize, idx: *mut u64) -> c_int {
    NEXT.assume_init_ref().bmap.unwrap()(arg1, blocksize, idx)
}

unsafe extern "C" fn ioctl(
    arg1: *const c_char,
    cmd: c_int,
    arg: *mut c_void,
    arg2: *mut fuse_file_info,
    flags: c_uint,
    data: *mut c_void,
) -> c_int {
    NEXT.assume_init_ref().ioctl.unwrap()(arg1, cmd, arg, arg2, flags, data)
}

unsafe extern "C" fn poll(
    arg1: *const c_char,
    arg2: *mut fuse_file_info,
    ph: *mut fuse_pollhandle,
    reventsp: *mut c_uint,
) -> c_int {
    NEXT.assume_init_ref().poll.unwrap()(arg1, arg2, ph, reventsp)
}

unsafe extern "C" fn write_buf(
    arg1: *const c_char,
    buf: *mut fuse_bufvec,
    off: off_t,
    arg2: *mut fuse_file_info,
) -> c_int {
    NEXT.assume_init_ref().write_buf.unwrap()(arg1, buf, off, arg2)
}

unsafe extern "C" fn read_buf(
    arg1: *const c_char,
    bufp: *mut *mut fuse_bufvec,
    size: usize,
    off: off_t,
    arg2: *mut fuse_file_info,
) -> c_int {
    NEXT.assume_init_ref().read_buf.unwrap()(arg1, bufp, size, off, arg2)
}

unsafe extern "C" fn flock(arg1: *const c_char, arg2: *mut fuse_file_info, op: c_int) -> c_int {
    NEXT.assume_init_ref().flock.unwrap()(arg1, arg2, op)
}

unsafe extern "C" fn fallocate(
    arg1: *const c_char,
    arg2: c_int,
    arg3: off_t,
    arg4: off_t,
    arg5: *mut fuse_file_info,
) -> c_int {
    NEXT.assume_init_ref().fallocate.unwrap()(arg1, arg2, arg3, arg4, arg5)
}

unsafe extern "C" fn copy_file_range(
    path_in: *const c_char,
    fi_in: *mut fuse_file_info,
    offset_in: off_t,
    path_out: *const c_char,
    fi_out: *mut fuse_file_info,
    offset_out: off_t,
    size: usize,
    flags: c_int,
) -> isize {
    NEXT.assume_init_ref().copy_file_range.unwrap()(
        path_in, fi_in, offset_in, path_out, fi_out, offset_out, size, flags,
    )
}

unsafe extern "C" fn lseek(
    arg1: *const c_char,
    off: off_t,
    whence: c_int,
    arg2: *mut fuse_file_info,
) -> off_t {
    NEXT.assume_init_ref().lseek.unwrap()(arg1, off, whence, arg2)
}

//
/// # Safety
///
/// This function must be called with a non-null next pointer
#[no_mangle]
pub unsafe extern "C" fn new_nop_layer(next: *const fuse_operations) -> *const fuse_operations {
    let next = unsafe { next.read() };
    NEXT.write(next);
    Box::into_raw(Box::new(fuse_operations {
        getattr: next.getattr.and(Some(getattr)),
        readlink: next.readlink.and(Some(readlink)),
        mknod: next.mknod.and(Some(mknod)),
        mkdir: next.mkdir.and(Some(mkdir)),
        unlink: next.unlink.and(Some(unlink)),
        rmdir: next.rmdir.and(Some(rmdir)),
        symlink: next.symlink.and(Some(symlink)),
        rename: next.rename.and(Some(rename)),
        link: next.link.and(Some(link)),
        chmod: next.chmod.and(Some(chmod)),
        chown: next.chown.and(Some(chown)),
        truncate: next.truncate.and(Some(truncate)),
        open: next.open.and(Some(open)),
        read: next.read.and(Some(read)),
        write: next.write.and(Some(write)),
        statfs: next.statfs.and(Some(statfs)),
        flush: next.flush.and(Some(flush)),
        release: next.release.and(Some(release)),
        fsync: next.fsync.and(Some(fsync)),
        setxattr: next.setxattr.and(Some(setxattr)),
        getxattr: next.getxattr.and(Some(getxattr)),
        listxattr: next.listxattr.and(Some(listxattr)),
        removexattr: next.removexattr.and(Some(removexattr)),
        opendir: next.opendir.and(Some(opendir)),
        readdir: next.readdir.and(Some(readdir)),
        releasedir: next.releasedir.and(Some(releasedir)),
        fsyncdir: next.fsyncdir.and(Some(fsyncdir)),
        init: next.init.and(Some(init)),
        destroy: next.destroy.and(Some(destroy)),
        access: next.access.and(Some(access)),
        create: next.create.and(Some(create)),
        lock: next.lock.and(Some(lock)),
        utimens: next.utimens.and(Some(utimens)),
        bmap: next.bmap.and(Some(bmap)),
        ioctl: next.ioctl.and(Some(ioctl)),
        poll: next.poll.and(Some(poll)),
        write_buf: next.write_buf.and(Some(write_buf)),
        read_buf: next.read_buf.and(Some(read_buf)),
        flock: next.flock.and(Some(flock)),
        fallocate: next.fallocate.and(Some(fallocate)),
        copy_file_range: next.copy_file_range.and(Some(copy_file_range)),
        lseek: next.lseek.and(Some(lseek)),
    }))
}
