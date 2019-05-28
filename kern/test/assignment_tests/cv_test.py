import subprocess
import time
from base_repeat_test import BaseRepeatTest
 
class CVTest(BaseRepeatTest):
    """
    Test for A1 conditional variable (CV)
    Test is synchtest.c, function cvtestthread
    """
    def __init__(self):
        super(CVTest, self).__init__(
            test_name="Conditional variable test",
            test_cmd="sys161 kernel 'sy3;q'"
        )
    
    def error_check_output(self, output):
        # Output should be "Thread 31 ... Thread 1" 5 times
        curr_thread_num = 32
        for line in output.splitlines():
            if line.startswith("Thread"):
                next_thread_num = self._get_thread_num(line)
                assert(next_thread_num < curr_thread_num or 
                       (curr_thread_num == 0 and next_thread_num == 31))

    def _get_thread_num(self, line):
        return int(line[len("Thread "):].strip())

def main():
    cv_test = CVTest()
    cv_test.run_test()

if __name__== "__main__":
    main()