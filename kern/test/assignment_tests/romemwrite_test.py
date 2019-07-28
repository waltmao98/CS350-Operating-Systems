import subprocess
import time
from base_repeat_test import BaseRepeatTest

class Romemwrite(BaseRepeatTest):
    """
    Test for enforcement of read-only memory
    """
    def __init__(self):
        super(Romemwrite, self).__init__(
            test_name="romemwrite test",
            test_cmds="sys161 kernel 'p uw-testbin/romemwrite;q'"
        )

def main():
    romemwrite_test = Romemwrite()
    romemwrite_test.run_test()

if __name__== "__main__":
    main()
