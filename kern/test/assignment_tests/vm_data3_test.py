import subprocess
import time
from base_repeat_test import BaseRepeatTest

class VMData3Test(BaseRepeatTest):
    """
    Test for TLB replacement
    """
    def __init__(self):
        super(VMData3Test, self).__init__(
            test_name="VM-data 3 test",
            test_cmds="sys161 kernel 'p uw-testbin/vm-data3;q'"
        )

def main():
    vm_data3_test = VMData3Test()
    vm_data3_test.run_test()

if __name__== "__main__":
    main()
