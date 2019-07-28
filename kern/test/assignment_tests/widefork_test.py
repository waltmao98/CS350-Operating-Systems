import subprocess
import time
from base_repeat_test import BaseRepeatTest

class WideForkTest(BaseRepeatTest):
    """
    Test for widefork in widefork.c
    """
    def __init__(self):
        super(WideForkTest, self).__init__(
            test_name="Wide fork test",
            test_cmds="sys161 kernel 'p uw-testbin/widefork;q'"
        )

def main():
    wide_fork_test = WideForkTest()
    wide_fork_test.run_test()

if __name__== "__main__":
    main()
