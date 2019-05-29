import subprocess
import time
from base_repeat_test import BaseRepeatTest
 
class TrafficSynchTest(BaseRepeatTest):
    """
    Test for A1 Traffic intersection synch problem
    Runs traffic_synch.c and checks the output
    """
    def __init__(self):
        super(TrafficSynchTest, self).__init__(
            test_name="Traffic intersection synch problem test",
            test_cmds=["sys161 kernel 'sp3 1 150 1 1 0;q'",
                       "sys161 kernel 'sp3 5 100 10 1 0;q'",
                       "sys161 kernel 'sp3 2 300 0 1 1;q'",
                       "sys161 kernel 'sp3 10 80 0 1 0;q'"],
            num_iterations=250
        )
    
    def check_output(self, output, **kwargs):
        # no error checking, just make sure of correctness (no crash/freeze)
        print(output)

def main():
    traffic_synch_test = TrafficSynchTest()
    traffic_synch_test.run_test()

if __name__== "__main__":
    main()