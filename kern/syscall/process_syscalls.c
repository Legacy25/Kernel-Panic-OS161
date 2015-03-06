

#include <types.h>
#include <syscall.h>
#include <thread.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <current.h>
#include <mips/trapframe.h>




int sys_fork(struct trapframe *tf, int32_t *retval) {

	struct trapframe *tf_copy;
	struct thread *ret;
	struct addrspace *child_addrspace;
	int result;

	/*
	 * Copy parent's trap frame
	 */
	tf_copy = kmalloc(sizeof(struct trapframe));
	memcpy(tf_copy, tf, sizeof(struct trapframe));

	/*
	 *  Copy parent's address space
	 */
	result = as_copy(curthread->t_addrspace, &child_addrspace);
	if(result) {
		return result;
	}

	/*
	 * Fork a new thread
	 */
	tf_copy->tf_a0 = (int32_t) child_addrspace;
	result = thread_fork(curthread->t_name, enter_forked_process,
			(void *) tf_copy, (unsigned long) NULL, &ret);
	if(result) {
		return result;
	}

	/*
	 * Set child parent process id to own pid
	 */
	ret->t_ppid = curthread->t_pid;


	/*
	 * Set return value to child pid
	 */
	*retval = ret->t_pid;

	return 0;

}


int sys_getpid(int32_t *retval) {
	*retval = curthread->t_pid;
	return 0;
}

int sys_exit(int _exitcode) {
	curthread->t_exited = true;
	curthread->t_exitcode = _exitcode;
	thread_exit();
	return 0;
}


