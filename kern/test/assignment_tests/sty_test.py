import subprocess
import time
from base_repeat_test import BaseRepeatTest

class StyTest(BaseRepeatTest):
    """
    tests fork, waitpid and execv in sty.c
    Doesn't require arguement passing.
    """
    def __init__(self):
        super(StyTest, self).__init__(
            test_name="Sty test",
            test_cmds="sys161 kernel 'p testbin/sty;q'"
        )

def main():
    sty_test = StyTest()
    sty_test.run_test()

if __name__== "__main__":
    main()
