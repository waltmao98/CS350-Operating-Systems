import subprocess
import time
from base_repeat_test import BaseRepeatTest

class VMData1Test(BaseRepeatTest):
    """
    Test for TLB replacement
    """
    def __init__(self):
        super(VMData1Test, self).__init__(
            test_name="VM-data 1 test",
            test_cmds="sys161 kernel 'p uw-testbin/vm-data1;q'"
        )

def main():
    vm_data1_test = VMData1Test()
    vm_data1_test.run_test()

if __name__== "__main__":
    main()
