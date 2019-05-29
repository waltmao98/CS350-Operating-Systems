import subprocess
import time
from base_string_in_ouput_test import BaseStringInOuptutTest
 
class UWLockTest(BaseStringInOuptutTest):
    """
    Test for A1 lock
    Test is uw-tests.c, function uwlocktest1
    """
    def __init__(self):
        super(UWLockTest, self).__init__(
            test_name="UW Lock test",
            test_cmds="sys161 kernel 'uw1;q'",
            success_msg="TEST SUCCEEDED",
            fail_msg="TEST FAILED"
        )

def main():
    uw_lock_test = UWLockTest()
    uw_lock_test.run_test()

if __name__== "__main__":
    main()