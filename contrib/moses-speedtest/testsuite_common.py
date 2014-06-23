"""Common functions of the testsuitce"""
import os
#Clour constants
class bcolors:
    PURPLE = '\033[95m'
    BLUE = '\033[94m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    ENDC = '\033[0m'

class LogLine:
    """A class to contain logfile line"""
    def __init__(self, date, time, revision, testname, real, user, system, branch):
        self.date = date
        self.time = time
        self.revision = revision
        self.testname = testname
        self.real = real
        self.system = system
        self.user = user
        self.branch = branch

class Result:
    """A class to contain results of benchmarking"""
    def __init__(self, testname, previous, current, revision, branch, prevrev, prevbranch):
        self.testname = testname
        self.previous = previous
        self.current = current
        self.change = previous - current
        self.revision = revision
        self.branch = branch
        self.prevbranch = prevbranch
        self.prevrev = prevrev
        #Produce a percentage with fewer digits
        self.percentage = float(format(1 - current/previous, '.4f'))

def processLogLine(logline):
    """Parses the log line into a nice datastructure"""
    logline = logline.split()
    log = LogLine(logline[0], logline[1], logline[2], logline[4],\
        float(logline[6]), float(logline[8]), float(logline[10]), logline[12])
    return log

def getLastTwoLines(filename, logdir):
    """Just a call to tail to get the diff between the last two runs"""
    try:
        line1, line2 = os.popen("tail -n2 " + logdir + '/' + filename)
    except ValueError: #Check for new tests
        tempfile = open(logdir + '/' + filename)
        line1 = tempfile.readline()
        tempfile.close()
        return (line1, '\n')
    return (line1, line2)
