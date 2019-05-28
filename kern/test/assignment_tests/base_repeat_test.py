import subprocess
import time

class BaseRepeatTest(object):
    """
    Base class for all tests that rely on repeatedly running an 
    OS test many times because results are non-deterministic. The
    OS tests run with this class have the form "sys161 kernel <test_cmd>",
    where test_cmd is an argument to BaseRepeatTest constructor.
    The checking of the output is left to the child class implementation
    in error_check_output.
    """

    def __init__(self, test_name, test_cmd, num_iterations=500):
        self.test_name = test_name
        self.test_cmd = test_cmd
        self.num_iterations = num_iterations
    
    def run_test(self):
        print("Running test %s %d times" % (self.test_name, self.num_iterations))
        start_time = time.time()
        for i in range(self.num_iterations):
            output = subprocess.check_output(self.test_cmd, shell=True, stderr=subprocess.STDOUT)
            try:
                self.error_check_output(output)
            except Exception as e:
                print("Test %d failed!" % (i+1))
                raise e
            print("%s iteration %d passed" % (self.test_name, i+1))

        self._print_success(start_time, time.time())

    def error_check_output(self, output):
        """
        To be implemented by child class. Check the output and raise
        an exception if the output indicates that the test failed.
        """
        raise NotImplementedError
    
    def _print_success(self, start_time, end_time):
        time_taken_seconds = float(end_time - start_time)
        time_taken_minutes = time_taken_seconds / 60
        if time_taken_minutes > 1:
            print("All tests for %s passed in %f minutes!" % (self.test_name, time_taken_minutes))
        else:
            print("All tests for %s passed in %f seconds!" % (self.test_name, time_taken_seconds))