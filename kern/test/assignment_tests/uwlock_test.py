import subprocess
import time
from base_repeat_test import BaseRepeatTest
 
class LockTest(BaseRepeatTest):
    """
    Test for A1 lock
    Test is uw-tests.c, function uwlocktest1
    """
    def __init__(self):
        super(LockTest, self).__init__(
            test_name="UW Lock test",
            test_cmd="sys161 kernel 'uw1;q'",
            success_msg="TEST SUCCEEDED",
            fail_msg="TEST FAILED"
        )

def main():
    uw_lock_test = LockTest()
    uw_lock_test.run_test()

if __name__== "__main__":
    main()