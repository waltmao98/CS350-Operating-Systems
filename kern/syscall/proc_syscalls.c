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
#include "opt-A2.h"

int setup_address_space(struct proc *child);
void setup_parent_child(struct proc *child);
struct trapframe *get_trapframe_copy(struct trapframe *tf);

  /* this implementation of sys__exit does not do anything with the exit code */
  /* this needs to be fixed to get exit() and waitpid() working properly */

void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;
  /* for now, just include this to keep the compiler from complaining about
     an unused variable */
  (void)exitcode;

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

  /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */
  proc_destroy(p);
  
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

/* stub handler for waitpid() system call                */

int
sys_waitpid(pid_t pid,
	    userptr_t status,
	    int options,
	    pid_t *retval)
{
  

  int exitstatus;
  int result;

  /* this is just a stub implementation that always reports an
     exit status of 0, regardless of the actual exit status of
     the specified process.   
     In fact, this will return 0 even if the specified process
     is still running, and even if it never existed in the first place.

     Fix this!
  */

  if (options != 0) {
    return(EINVAL);
  }
  /* for now, just pretend the exitstatus is 0 */
  exitstatus = 0;
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return(0);
}

int setup_address_space(struct proc *child) {
  spinlock_acquire(&child->p_lock);
  struct addrspace **child_addrspace = kmalloc(sizeof(struct addrspace **));
  int err = as_copy(curproc_getas(), child_addrspace);
  if (err != 0) {
    return err;
  }
  KASSERT(*child_addrspace);
  child->p_addrspace = *child_addrspace;
  kfree(child_addrspace);
  spinlock_release(&child->p_lock);
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
