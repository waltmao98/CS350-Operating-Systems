import subprocess
import time
from base_repeat_test import BaseRepeatTest

class VMData1Repeat(BaseRepeatTest):
    """
    Test for memory management. Runs the uw-testbin/vm-data1 repeatedly to check if memory
    is freed properly.
    """
    def __init__(self):
        test_cmd = ["sys161 kernel '"]
        for _ in range(100):
            test_cmd.append("p uw-testbin/vm-data1;")
        test_cmd.append("q'")
        test_cmd = "".join(test_cmd)
        super(VMData1Repeat, self).__init__(
            test_name="vm-data1 test with 100 iterations",
            test_cmds=test_cmd,
            num_iterations=1
        )

def main():
    vm_data1_repeat_test = VMData1Repeat()
    vm_data1_repeat_test.run_test()

if __name__== "__main__":
    main()
