import subprocess
import time

class BaseRepeatTest(object):
    """
    Base class for all tests that rely on repeatedly running an
    OS test many times because results are non-deterministic. The
    OS tests run with this class have the form "sys161 kernel <test_cmd>"
    for each test_cmd in test_cmds, where test_cmds is an argument to
    BaseRepeatTest constructor. The checking of the output is left to
    the child class implementation in check_output.
    """

    def __init__(self, test_name, test_cmds, num_iterations=500):
        self.test_name = test_name
        self.num_iterations = num_iterations
        # allow passing of single command string for convenience
        if isinstance(test_cmds, list) or isinstance(test_cmds, tuple):
            self.test_cmds = test_cmds
        else:
            self.test_cmds = [test_cmds]

    def run_test(self):
        print("Running test %s %d times each for the following commands: %s"
        % (self.test_name, self.num_iterations, str(self.test_cmds)))
        start_time = time.time()
        for test_cmd in self.test_cmds:
            for i in range(self.num_iterations):
                output = subprocess.check_output(test_cmd, shell=True, stderr=subprocess.STDOUT)
                try:
                    self.check_output(output)
                except Exception as e:
                    print("Test %d failed!" % (i+1))
                    raise e
                print("%s iteration %d passed of command \"%s\"" %
                      (self.test_name, i+1, test_cmd))
            if len(self.test_cmds) > 1:
                print("All %d iterations of command %s have been completed" %
                      (self.num_iterations, test_cmd))

        self._print_success(start_time, time.time())

    def check_output(self, output, **kwargs):
        """
        Check the output and raise
        an exception if the output indicates that the test failed.
        Child class should call super class to get these basic checks.
        """
        assert "panic" not in output
        assert "error" not in output
        assert "failed" not in output
        assert "Fail" not in output
        assert "FAIL" not in output
        assert "KASSERT" not in output

    def _print_success(self, start_time, end_time):
        time_taken_seconds = float(end_time - start_time)
        time_taken_minutes = time_taken_seconds / 60
        if time_taken_minutes > 1:
            print("All tests for %s passed in %f minutes!" %
                  (self.test_name, time_taken_minutes))
        else:
            print("All tests for %s passed in %f seconds!" %
                   (self.test_name, time_taken_seconds))
