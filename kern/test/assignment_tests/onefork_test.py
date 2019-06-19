import subprocess
import time
from base_repeat_test import BaseRepeatTest

class OneForkTest(BaseRepeatTest):
    """
    Test for A2 fork system call
    Test is proc_syscalls.c, function sys_fork
    """
    def __init__(self):
        super(OneForkTest, self).__init__(
            test_name="Fork system call test",
            test_cmds="sys161 kernel 'p uw-testbin/onefork;q'"
        )

    def check_output(self, output, **kwargs):
        p_count, c_count = 0, 0
        for line in output.splitlines():
            if len(line) == 1:
                if line[0] == 'P':
                    p_count += 1
                if line[0] == 'C':
                    c_count += 1
        assert p_count == 1 and c_count == 1

def main():
    onefork_test = OneForkTest()
    onefork_test.run_test()

if __name__== "__main__":
    main()
