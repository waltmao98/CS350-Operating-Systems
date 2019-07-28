import subprocess
import time
from base_repeat_test import BaseRepeatTest

class VMCrash2Test(BaseRepeatTest):
    """
    Test for enforcement of read-only memory
    """
    def __init__(self):
        super(VMCrash2Test, self).__init__(
            test_name="vm-crash2 test",
            test_cmds="sys161 kernel 'p uw-testbin/vm-crash2;q'"
        )

def main():
    vm_crash2_test = VMCrash2Test()
    vm_crash2_test.run_test()

if __name__== "__main__":
    main()
