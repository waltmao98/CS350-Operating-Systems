import subprocess
import time
from base_repeat_test import BaseRepeatTest

class SortTest(BaseRepeatTest):
    """
    Test for memory management. Runs the testbin/sort repeatedly to check if memory
    is freed properly.
    """
    def __init__(self):
        test_cmd = ["sys161 kernel '"]
        for _ in range(100):
            test_cmd.append("p testbin/sort;")
        test_cmd.append("q'")
        test_cmd = "".join(test_cmd)
        super(SortTest, self).__init__(
            test_name="sort test with 100 iterations",
            test_cmds=test_cmd,
            num_iterations=1
        )

def main():
    sort_test = SortTest()
    sort_test.run_test()

if __name__== "__main__":
    main()
