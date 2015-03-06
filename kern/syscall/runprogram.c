/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <thread.h>
#include <current.h>
#include <addrspace.h>
#include <vm.h>
#include <vfs.h>
#include <syscall.h>
#include <test.h>
#include <kern/unistd.h>
#include <file_syscalls.h>
#include <synch.h>

int file_desc_console_fd_init(void) {
	struct vnode *vd_stdin, *vd_stdout, *vd_stderr;
	char *path_name_stdin = NULL, *path_name_stdout = NULL, *path_name_stderr = NULL;
	path_name_stdin = kstrdup("con:");

	int res_stdin = 0, res_stdout = 0, res_stderr = 0;

	//////////////////STDIN//////////////////
	res_stdin = vfs_open(path_name_stdin, O_RDONLY, 0, &vd_stdin);
	if (res_stdin) {
		kfree(path_name_stdin);
		return res_stdin;
	}
	curthread->fd_table[STDIN_FILENO] = (PFD)kmalloc(sizeof(PFD));
	curthread->fd_table[STDIN_FILENO]->vd = vd_stdin;
	curthread->fd_table[STDIN_FILENO]->flags = O_RDONLY;
	curthread->fd_table[STDIN_FILENO]->offset = 0;
	curthread->fd_table[STDIN_FILENO]->ref_count = 1;
	curthread->fd_table[STDIN_FILENO]->fl_lock = lock_create("con:");

	//////////////////STDOUT//////////////////
	path_name_stdout = kstrdup("con:");
	res_stdout = vfs_open(path_name_stdout, O_WRONLY, 0, &vd_stdout);
	if (res_stdout) {
		kfree(path_name_stdin);
		kfree(path_name_stdout);
		return res_stdout;
	}
	curthread->fd_table[STDOUT_FILENO] = (PFD)kmalloc(sizeof(PFD));
	curthread->fd_table[STDOUT_FILENO]->vd = vd_stdout;
	curthread->fd_table[STDOUT_FILENO]->flags = O_WRONLY;
	curthread->fd_table[STDOUT_FILENO]->offset = 0;
	curthread->fd_table[STDOUT_FILENO]->ref_count = 1;
	curthread->fd_table[STDOUT_FILENO]->fl_lock = lock_create("con:");

	//////////////////STDERR//////////////////
	path_name_stderr = kstrdup("con:");
	res_stderr = vfs_open(path_name_stderr, O_WRONLY, 0, &vd_stderr);
	if (res_stderr) {
		kfree(path_name_stdin);
		kfree(path_name_stdout);
		kfree(path_name_stderr);
		return res_stderr;
	}
	curthread->fd_table[STDERR_FILENO] = (PFD)kmalloc(sizeof(PFD));
	curthread->fd_table[STDERR_FILENO]->vd = vd_stderr;
	curthread->fd_table[STDERR_FILENO]->flags = O_WRONLY;
	curthread->fd_table[STDERR_FILENO]->offset = 0;
	curthread->fd_table[STDERR_FILENO]->ref_count = 1;
	curthread->fd_table[STDERR_FILENO]->fl_lock = lock_create("con:");

	kfree(path_name_stdin);
	kfree(path_name_stdout);
	kfree(path_name_stderr);

	return 0;
}

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
int
runprogram(char *progname)
{
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, 0, &v);
	if (result) {
		return result;
	}

	/* We should be a new thread. */
	KASSERT(curthread->t_addrspace == NULL);

	/* Create a new address space. */
	curthread->t_addrspace = as_create();
	if (curthread->t_addrspace==NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Initialize STDIN, STDOUT and STDERR File Handles in the thread's file descriptor table*/
	// result = file_desc_console_fd_init();
	if (result) {
		return result;
	}

	/* Activate it. */
	as_activate(curthread->t_addrspace);

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* thread_exit destroys curthread->t_addrspace */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(curthread->t_addrspace, &stackptr);
	if (result) {
		/* thread_exit destroys curthread->t_addrspace */
		return result;
	}

	/* Warp to user mode. */
	enter_new_process(0 /*argc*/, NULL /*userspace addr of argv*/,
			  stackptr, entrypoint);
	
	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	return EINVAL;
}
