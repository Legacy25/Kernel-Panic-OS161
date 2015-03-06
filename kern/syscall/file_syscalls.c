/*
 * filedefs.c
 *
 *  Created on: Feb 28, 2015
 *      Author: trinity
 */
#include <types.h>
#include <thread.h>
#include <kern/errno.h>
#include <kern/seek.h>
#include <file_syscalls.h>
#include <vnode.h>
#include <uio.h>
#include <kern/iovec.h>
#include <kern/unistd.h>
#include <kern/fcntl.h>
#include <kern/stat.h>
#include <lib.h>
#include <vfs.h>
#include <copyinout.h>
#include <synch.h>
#include <current.h>

#define NO_ERROR 0;
#define ERROR -1;

int sys_open(char* filename, int flags, mode_t mode, int32_t *retval) {
	int i = 0, fopen_res = -1;
	int res_vopstat;
	struct stat *file_info;
	file_info = (struct stat*)kmalloc(sizeof(struct stat*));
	struct vnode *vd;
	for (i=0;((curthread->fd_table[i] != NULL) && (i<OPEN_MAX));i++);

	if (i == (OPEN_MAX - 1)) {
		//Max file limit reached
		kfree(file_info);
		*retval = ERROR;
		return EMFILE;
	}

	if (filename == NULL) {
		kfree(file_info);
		*retval = ERROR;
		return EFAULT;
	}

	if ((flags != O_RDONLY) || (flags != O_WRONLY) || (flags != O_RDWR) || (flags != O_CREAT) || (flags != O_EXCL) || (flags != O_TRUNC) || (flags != O_APPEND)) {
		kfree(file_info);
		*retval = ERROR;
		return EINVAL;
	}
	curthread->fd_table[i] = (struct file_desc*)kmalloc(sizeof(struct file_desc*));
	void * kern_fname;
	size_t str_len;
	int res_copyinstr = 0;
	kern_fname = (char *)kmalloc(sizeof(char *)*PATH_MAX);
	res_copyinstr = copyinstr((const_userptr_t)filename, kern_fname, PATH_MAX, &str_len);
	if (res_copyinstr) {
		//error in allocating kernel space memory
		kfree(kern_fname);
		kfree(file_info);
		kfree(curthread->fd_table[i]);
		curthread->fd_table[i] = NULL;
		*retval = ERROR;
		return res_copyinstr;
	}

	fopen_res = vfs_open(kern_fname, flags, mode, &vd);
	if (fopen_res) {
		//Error on vfs_open
		kfree(curthread->fd_table[i]);
		kfree(file_info);
		kfree(kern_fname);
		curthread->fd_table[i] = NULL;
		*retval = ERROR;
		return fopen_res;
	}

	if (flags == O_APPEND) { //get file size if append
		res_vopstat = VOP_STAT(curthread->fd_table[fopen_res]->vd, file_info);
		if (res_vopstat) {
			kfree(curthread->fd_table[i]);
			kfree(file_info);
			kfree(kern_fname);
			curthread->fd_table[i] = NULL;
			*retval = ERROR;
			return res_vopstat;
		}
	}
	//Set parameters of filehandle in thread's file descriptor table
	curthread->fd_table[i]->vd = vd;
	curthread->fd_table[i]->flags = flags;
	curthread->fd_table[i]->offset = (flags == O_APPEND) ? file_info->st_size : (off_t)0;
	curthread->fd_table[i]->ref_count = 1;
	curthread->fd_table[i]->fl_lock = lock_create(kern_fname); //this is for ease of debugging, no actual function
	kfree(kern_fname);
	kfree(file_info);
	*retval = fopen_res;
	return NO_ERROR;
}

int sys_close(int fd, int32_t *retval) {
	if ((fd < 0) || (fd > OPEN_MAX) || (curthread->fd_table[fd] == NULL)) {
		*retval = ERROR;
		return EBADF;
	}
	if (curthread->fd_table[fd]->ref_count == 1) {
		curthread->fd_table[fd]->ref_count--;
		vfs_close(curthread->fd_table[fd]->vd);
		*retval = NO_ERROR;
		return NO_ERROR;
	} else {
		curthread->fd_table[fd]->ref_count--;
		*retval = NO_ERROR;
		return NO_ERROR;
	}
	return NO_ERROR;
}

int sys_read(int fd, void *buf, size_t buflen, int32_t *retval) {
	//Error Handling
	if ((fd < 0) || (fd > OPEN_MAX) || (curthread->fd_table[fd] == NULL)) {
		*retval = ERROR;
		return EBADF;
	}
	struct iovec iv;
	struct uio uv;
	lock_acquire(curthread->fd_table[fd]->fl_lock);

	off_t bytes_read = curthread->fd_table[fd]->offset;
	int result;

	iv.iov_ubase = (userptr_t)buf;
	iv.iov_len = buflen;

	uv.uio_iov = &iv;		/* Data blocks */
	uv.uio_iovcnt = 1;			/* Number of iovecs */
	uv.uio_offset = curthread->fd_table[fd]->offset;	/* Desired offset into object */
	uv.uio_resid = buflen;	/* Remaining amt of data to xfer */
	uv.uio_segflg = UIO_USERSPACE;	/* What kind of pointer we have */
	uv.uio_rw = UIO_READ;	/* Whether op is a read or write */
	uv.uio_space = curthread->t_addrspace;

	result = VOP_READ(curthread->fd_table[fd]->vd, &uv);
	if (result) {
		//Failure handle
		*retval = ERROR;
		return result;
	}

	curthread->fd_table[fd]->offset = uv.uio_offset;
	bytes_read = curthread->fd_table[fd]->offset - bytes_read;		//can we use uv.uio_iov->iov_len??
	*retval = (uint32_t)bytes_read;
	lock_release(curthread->fd_table[fd]->fl_lock);
	return NO_ERROR;
}

int sys_write(int fd, const void *buf, size_t nbytes, int32_t *retval) {
	//Error Handling
	if ((fd < 0) || (fd > OPEN_MAX) || (curthread->fd_table[fd] == NULL)) {
		*retval = ERROR;
		return EBADF;
	}
	struct iovec iv;
	struct uio uv;
	lock_acquire(curthread->fd_table[fd]->fl_lock);

	off_t bytes_written = curthread->fd_table[fd]->offset;
	int result;

	iv.iov_ubase = (userptr_t)buf;
	iv.iov_len = nbytes;

	uv.uio_iov = &iv;		/* Data blocks */
	uv.uio_iovcnt = 1;		/* Number of iovecs */
	uv.uio_offset = curthread->fd_table[fd]->offset;	/* Desired offset into object */
	uv.uio_resid = nbytes;	/* Remaining amt of data to xfer */
	uv.uio_segflg = UIO_USERSPACE;	/* What kind of pointer we have */
	uv.uio_rw = UIO_WRITE;	/* Whether op is a read or write */
	uv.uio_space = curthread->t_addrspace;

	result = VOP_WRITE(curthread->fd_table[fd]->vd, &uv);
	if (result) {
		//Failure handle
		*retval = ERROR;
		return result;
	}
	curthread->fd_table[fd]->offset = uv.uio_offset;
	bytes_written = curthread->fd_table[fd]->offset - bytes_written; //can we use uv.uio_iov->iov_len??
	*retval = (uint32_t)bytes_written;
	lock_release(curthread->fd_table[fd]->fl_lock);
	return NO_ERROR;
}

int sys_chdir(const char* pathname, int32_t *retval) {
	int result = 0;
	char *kern_pathname;
	size_t str_len;
	kern_pathname = (char *)kmalloc(sizeof(char)*PATH_MAX);
	result = copyinstr((const_userptr_t)pathname, kern_pathname, PATH_MAX, &str_len);
	if (result) {
		//error in allocating kernel space memory
		*retval = ERROR;
		kfree(kern_pathname);
		return result;
	}
	result = vfs_chdir(kern_pathname);
	if (result) {
		*retval = ERROR;
		kfree(kern_pathname);
		return result;
	}
	*retval = NO_ERROR;
	return NO_ERROR;
}

int sys___getcwd(char *buf, size_t buflen, int32_t *retval) {
	if (buf == NULL) {	//buf does not point to a memory location
		*retval = ERROR;
		return EFAULT;
	}
	struct iovec iv;
	struct uio uv;
	int res_getcwd = 0;
	iv.iov_ubase = (userptr_t)buf;
	iv.iov_len = buflen;

	uv.uio_iov = &iv;
	uv.uio_iovcnt = 1;
	uv.uio_offset = (off_t)0;
	uv.uio_resid = buflen - 1;
	uv.uio_rw = UIO_USERSPACE;
	uv.uio_segflg = UIO_READ;
	uv.uio_space = curthread->t_addrspace;
	res_getcwd = vfs_getcwd(&uv);
	if (res_getcwd) {
		*retval = ERROR;
		return res_getcwd;
	}
	buf[sizeof(buf)] = '\0';
	*retval = uv.uio_iov->iov_len;
	return NO_ERROR;
}

int sys_dup2(int oldfd, int newfd, int32_t *retval) {
	if((oldfd == newfd) || (oldfd < 0) || (oldfd > OPEN_MAX) || (newfd < 0) || (newfd > OPEN_MAX)) {
		*retval = ERROR;
		return EBADF;
	}
	int i = 0;
	for (i=0;((curthread->fd_table[i] != NULL) || (i <= OPEN_MAX));i++);
	if (i == OPEN_MAX) {
		*retval = ERROR;
		return EMFILE;
	}

	if (curthread->fd_table[newfd] != NULL) {
		int res_sysclose = 0;
		int32_t *retval;
		res_sysclose = sys_close(newfd, retval);
		if (res_sysclose) {
			*retval = ERROR;
			return EBADF;
		}
	} else {
		if (!lock_do_i_hold(curthread->fd_table[oldfd]->fl_lock))
		lock_acquire(curthread->fd_table[oldfd]->fl_lock);

		curthread->fd_table[newfd] = (PFD)kmalloc(sizeof(FD));
		curthread->fd_table[newfd] = curthread->fd_table[oldfd];
		lock_release(curthread->fd_table[oldfd]->fl_lock);
		*retval = newfd;
	}
	return NO_ERROR;
}

off_t sys_lseek(int fd, off_t pos, int whence, off_t *retval) {

	if((curthread->fd_table[fd] == NULL) || (fd < 0) || (fd > OPEN_MAX) || (fd == STDIN_FILENO) || (fd == STDOUT_FILENO) || (fd == STDERR_FILENO)) {
		*retval = (off_t)ERROR;
		return EBADF;
	}
	if ((whence != SEEK_SET) || (whence != SEEK_END) || (whence != SEEK_CUR)) {
		*retval = (off_t)ERROR;
		return EINVAL;
	}
	int res_vopstat, res_voptryseek;
	struct stat file_info;
	off_t new_offset;

	lock_acquire(curthread->fd_table[fd]->fl_lock);

	res_vopstat = VOP_STAT(curthread->fd_table[fd]->vd, &file_info);
	if (res_vopstat) {
		lock_release(curthread->fd_table[fd]->fl_lock);
		*retval = (off_t)ERROR;
		return res_vopstat;
	}

	if (whence == SEEK_SET) {
		if (pos < 0) {
			lock_release(curthread->fd_table[fd]->fl_lock);
			*retval = (off_t)ERROR;
			return EINVAL;
		}
		new_offset = pos;
	} else if (whence == SEEK_CUR) {
		if (new_offset < (off_t)0) {
			lock_release(curthread->fd_table[fd]->fl_lock);
			*retval = (off_t)ERROR;
			return EINVAL;
		}
		new_offset = curthread->fd_table[fd]->offset + pos;
	} else if (whence == SEEK_END) {
		new_offset = file_info.st_size + pos;
	}

	res_voptryseek = VOP_TRYSEEK(curthread->fd_table[fd]->vd, new_offset);
	if (res_voptryseek) {
		lock_release(curthread->fd_table[fd]->fl_lock);
		*retval = *retval = (off_t)ERROR;
		return res_voptryseek;
	}

	curthread->fd_table[fd]->offset = new_offset;
	*retval = new_offset;
	return NO_ERROR;
}
