import subprocess
import time
from base_repeat_test import BaseRepeatTest

class MatMultTest(BaseRepeatTest):
    """
    Test for memory management. Runs the testbin/matmult repeatedly to check if memory
    is freed properly.
    """
    def __init__(self):
        num_iterations = 1000
        test_cmd = ["sys161 kernel '"]
        for _ in range(num_iterations):
            test_cmd.append("p testbin/matmult;")
        test_cmd.append("q'")
        test_cmd = "".join(test_cmd)
        super(MatMultTest, self).__init__(
            test_name="mat_mult test with %d iterations" % (num_iterations),
            test_cmds=test_cmd,
            num_iterations=1
        )

def main():
    mat_mult_test = MatMultTest()
    mat_mult_test.run_test()

if __name__== "__main__":
    main()
