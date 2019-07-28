import subprocess
import time
from base_repeat_test import BaseRepeatTest

class ForkTestTest(BaseRepeatTest):
    """
    Test for widefork in widefork.c
    """
    def __init__(self):
        super(ForkTestTest, self).__init__(
            test_name="Fork test test",
            test_cmds="sys161 kernel 'p testbin/forktest;q'"
        )

def main():
    fork_test = ForkTestTest()
    fork_test.run_test()

if __name__== "__main__":
    main()
