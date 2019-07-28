import subprocess
import time
from base_repeat_test import BaseRepeatTest

class PIDCheckTest(BaseRepeatTest):
    """
    Test for A2 fork and getpid system calls
    Test is proc_syscalls.c, function sys_fork and sys_getpid
    """
    def __init__(self):
        super(PIDCheckTest, self).__init__(
            test_name="Fork and getpid system call test",
            test_cmds="sys161 kernel 'p uw-testbin/pidcheck;q'"
        )

    def check_output(self, output, **kwargs):
        super(PIDCheckTest, self).check_output(output, **kwargs)
        for line in output.splitlines():
            if line.startswith("C: "):
                child_pid_seen_by_child = int(line[len("C: ")].strip())
                assert child_pid_seen_by_child == 2
            elif line.startswith("PC: "):
                child_pid_seen_by_parent = int(line[len("PC: ")].strip())
                assert child_pid_seen_by_parent == 2
            elif line.startswith("PP: "):
                parent_pid_seen_by_parent = int(line[len("PP: ")].strip())
                assert parent_pid_seen_by_parent == 1

def main():
    pidcheck_test = PIDCheckTest()
    pidcheck_test.run_test()

if __name__== "__main__":
    main()
