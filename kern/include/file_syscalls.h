/*
 * filedefs.h
 *
 *  Created on: Feb 28, 2015
 *      Author: trinity
 */

#ifndef FILEDEFS_H_
#define FILEDEFS_H_

typedef struct file_desc {
	struct vnode *vd;
	off_t offset;
	int flags;
	int ref_count;
	struct lock *fl_lock;
}FD;

typedef FD * PFD;

int sys_open(char* filename, int flags, mode_t mode, int32_t *retval);
int sys_close(int fd, int32_t *retval);
int sys_read(int fd, void *buf, size_t buflen, int32_t *retval);
int sys_write(int fd, const void *buf, size_t nbytes, int32_t *retval);
off_t sys_lseek(int fd, off_t pos, int whence, off_t *retval);
int sys_dup2(int oldfd, int newfd, int32_t *retval);
int sys_chdir(const char *pathname, int32_t *retval);
int sys___getcwd(char *buf, size_t buflen, int32_t *retval);
int file_desc_console_fd_init(void);



#endif /* FILEDEFS_H_ */
