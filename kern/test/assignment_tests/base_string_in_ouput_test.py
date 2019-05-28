from base_repeat_test import BaseRepeatTest
import subprocess
import time

class BaseStringInOuptutTest(BaseRepeatTest):
    """
    Base class for tests that involve simply checking if a success or fail
    message is contained in the output.
    """
    def __init__(self, test_name, test_cmd, success_msg, fail_msg, num_iterations=500):
        super(BaseStringInOuptutTest, self).__init__(
            test_name=test_name,
            test_cmd=test_cmd,
            num_iterations=num_iterations
        )
        self.success_msg = success_msg
        self.fail_msg = fail_msg
    
    def error_check_output(self, output):
        if self.success_msg not in str(output) or self.fail_msg in str(output):
            raise Exception()