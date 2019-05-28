import subprocess
import time
from base_string_in_ouput_test import BaseStringInOuptutTest
 
class LockTest(BaseStringInOuptutTest):
    """
    Test for A1 lock
    Test is synchtest.c, function locktest
    """
    def __init__(self):
        super(LockTest, self).__init__(
            test_name="Lock test",
            test_cmd="sys161 kernel 'sy2;q'",
            success_msg="Lock test done",
            fail_msg="thread_fork failed"
        )

def main():
    lock_test = LockTest()
    lock_test.run_test()

if __name__== "__main__":
    main()