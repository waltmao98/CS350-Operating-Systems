import subprocess
import time
from base_repeat_test import BaseRepeatTest

class HogPartyTest(BaseRepeatTest):
    """
    Test for tests fork and execv in hogparty.c.
    Doesn't require arguement passing.
    """
    def __init__(self):
        super(HogPartyTest, self).__init__(
            test_name="Hog party test",
            test_cmds="sys161 kernel 'p uw-testbin/hogparty;q'"
        )

def main():
    hog_party_test = HogPartyTest()
    hog_party_test.run_test()

if __name__== "__main__":
    main()
