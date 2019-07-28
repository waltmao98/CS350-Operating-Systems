#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/wait.h>
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <copyinout.h>
#include <mips/trapframe.h>
#include <synch.h>
#include <vfs.h>
#include <kern/fcntl.h>
#include "opt-A2.h"

int setup_address_space(struct proc *child);
void setup_parent_child(struct proc *child);
struct trapframe *get_trapframe_copy(struct trapframe *tf);

int sys_execv(userptr_t progname, userptr_t args) {

	struct addrspace *as;
  struct vnode *v;
	vaddr_t entrypoint, stackptr;
  int result;


  // copy args
  char **user_args = (char **) args;
  size_t num_args = 0; // number of args
  while (user_args[num_args]) {
    num_args += 1;
  }

  char **args_copy = kmalloc(sizeof(char *) * num_args);
  for (size_t i = 0; i < num_args; ++i) {
    args_copy[i] = kmalloc(sizeof(char) * (strlen(user_args[i]) + 1));
    result = copyinstr((userptr_t)user_args[i], args_copy[i], strlen(user_args[i]) + 1, NULL);
    if (result) {
      return result;
    }
  }


	/* Open the file. */
  char *program_name_temp = kstrdup((char *)progname); // make copy of program path
  result = vfs_open(program_name_temp, O_RDONLY, 0, &v);
  kfree(program_name_temp);
  if (result) {
    return result;
  }

  /* Create a new address space. */
	as = as_create();
	if (as ==NULL) {
		vfs_close(v);
    for (size_t i = 0; i < num_args; ++i) kfree(args_copy[i]);
    kfree(args_copy);
		return ENOMEM;
	}

  /* Switch to it and activate it. */
  struct addrspace *old_as = curproc_setas(as);
	as_activate();

  /* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
    for (size_t i = 0; i < num_args; ++i) kfree(args_copy[i]);
    kfree(args_copy);
		vfs_close(v);
		return result;
	}

	vfs_close(v); /* Done with the file now. */
  as_destroy(old_as); // Destroy the old address space

  /* Define the user stack in the address space */
	result = as_define_stack(as, &stackptr, args_copy, num_args);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
    for (size_t i = 0; i < num_args; ++i) kfree(args_copy[i]);
    kfree(args_copy);
		return result;
	}

  // free copy of args
  for (size_t i = 0; i < num_args; ++i) kfree(args_copy[i]);
  kfree(args_copy);

  /* Warp to user mode. */
	enter_new_process(
    num_args /*argc*/, 
    (userptr_t) stackptr /*userspace addr of argv*/,
		stackptr, 
    entrypoint
  );

  return -1; // execv failed
}

void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;

  DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

  KASSERT(curproc->p_addrspace != NULL);
  as_deactivate();
  /*
   * clear p_addrspace before calling as_destroy. Otherwise if
   * as_destroy sleeps (which is quite possible) when we
   * come back we'll be calling as_activate on a
   * half-destroyed address space. This tends to be
   * messily fatal.
   */
  as = curproc_setas(NULL);
  as_destroy(as);

  /* detach this thread from its process */
  /* note: curproc cannot be used after this call */
  proc_remthread(curthread);
  
  lock_acquire(p->p_lock);
  p->p_alive = false;
  p->p_exit_code = exitcode;

  // Go through every child
  // If child is dead, then we can destroy it
  // If child is not dead, we set its parent pointer to NULL
	while (array_num(p->p_children) > 0) {
		struct proc *child = (struct proc *) array_get(p->p_children, 0);
    lock_acquire(child->p_lock);
    array_remove(p->p_children, 0);
    child->p_parent = NULL;
    if (!child->p_alive) {
      lock_release(child->p_lock);
      proc_destroy(child);
    } else {
      lock_release(child->p_lock);
    }
	}

  // If this process's parent is dead
  if (!p->p_parent) {
    lock_release(p->p_lock);
    /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */
    proc_destroy(p);
  } else {
    cv_signal(p->p_wait_pid_cv, p->p_lock);
    lock_release(p->p_lock);
  }
  
  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");
}


/* getpid() system call                */
int
sys_getpid(pid_t *retval)
{
#if OPT_A2
  *retval = curproc->p_pid;
#else
  *retval = 1;
#endif /* OPT_A2 */

  return 0;
}

/* sys_waitpid ////////////////////////// */

int
sys_waitpid(pid_t pid,
	    userptr_t status,
	    int options,
	    pid_t *retval)
{
  if (options != 0) {
    *retval = -1;
    return EINVAL;
  }

  if (!status) {
    *retval = -1;
    return EFAULT;
  }

  KASSERT(pid > 0);
  if (!is_child_process_of_curproc(pid)) {
    *retval = -1;
    return ECHILD;
  }

  int exitstatus;
  int result;

  struct proc *child = proc_get_child(pid);
  KASSERT(child);

  // If child is already dead, get the pid and return
  lock_acquire(child->p_lock);
  if (child->p_alive) {
    // Go to sleep until child calls exit()
    cv_wait(child->p_wait_pid_cv, child->p_lock);
    // Child has called exit()
  }
  exitstatus = _MKWAIT_EXIT(child->p_exit_code);
  lock_release(child->p_lock);

  proc_remove_child(child);
  proc_destroy(child);
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    *retval = -1;
    return result;
  }
  *retval = pid;
  return 0;
}

/* sys_fork ////////////////////////// */

int setup_address_space(struct proc *child) {
  spinlock_acquire(&child->p_spinlock);
  struct addrspace **child_addrspace = kmalloc(sizeof(struct addrspace **));
  int err = as_copy(curproc_getas(), child_addrspace);
  if (err != 0) {
    return err;
  }
  KASSERT(*child_addrspace);
  child->p_addrspace = *child_addrspace;
  kfree(child_addrspace);
  spinlock_release(&child->p_spinlock);
  return 0;
}

struct trapframe *get_trapframe_copy(struct trapframe *tf) {
  KASSERT(tf);
  struct trapframe *copy = kmalloc(sizeof(struct trapframe));
  memcpy(copy, tf, sizeof(struct trapframe));
  KASSERT(copy);
  return copy;
}

int sys_fork(struct trapframe *tf, pid_t *retval) {

  struct proc *child = proc_create_runprogram(curproc->p_name);
  if (!child) {
    return ENOMEM;
  }

  // Create address space and copy code, data, and stack over
  int err = setup_address_space(child);
  if (err != 0) {
    return err; // Probably ENOMEM
  }

  // Estabilish parent-child relationship
  proc_add_child(child);

  // Make deep copy of the trapframe
  // It's the responsibility of enter_forked_process to free it
  struct trapframe *tf_copy = get_trapframe_copy(tf);

  pid_t child_pid = child->p_pid;

  // Start new thread in child process
  thread_fork(
    "new process thread",
    child,
    enter_forked_process,
    tf_copy,
    (unsigned long) child->p_pid
  );

  *retval = (pid_t) child_pid;
  return 0;
}
