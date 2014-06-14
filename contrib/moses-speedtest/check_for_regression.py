"""Checks if any of the latests tests has performed considerably different than
 the previous ones. Takes the log directory as an argument."""
import os
import sys
from testsuite_common import Result, processLogLine, bcolors, getLastTwoLines

LOGDIR = sys.argv[1] #Get the log directory as an argument
PERCENTAGE = 5 #Default value for how much a test shoudl change
if len(sys.argv) == 3:
    PERCENTAGE = float(sys.argv[2]) #Default is 5%, but we can specify more
    #line parameter

def printResults(regressed, better, unchanged, firsttime):
    """Pretty print the results in different colours"""
    if regressed != []:
        for item in regressed:
            print(bcolors.RED + "REGRESSION! " + item.testname + " Was: "\
            + str(item.previous) + " Is: " + str(item.current) + " Change: "\
            + str(abs(item.percentage)) + "%. Revision: " + item.revision\
            + bcolors.ENDC)
    print('\n')
    if unchanged != []:
        for item in unchanged:
            print(bcolors.BLUE + "UNCHANGED: " + item.testname + " Revision: " +\
                item.revision + bcolors.ENDC)
    print('\n')
    if better != []:
        for item in better:
            print(bcolors.GREEN + "IMPROVEMENT! " + item.testname + " Was: "\
            + str(item.previous) + " Is: " + str(item.current) + " Change: "\
            + str(abs(item.percentage)) + "%. Revision: " + item.revision\
            + bcolors.ENDC)
    if firsttime != []:
        for item in firsttime:
            print(bcolors.PURPLE + "First time test! " + item.testname +\
            " Took: " + str(item.real) +  " seconds. Revision: " +\
            item.revision + bcolors.ENDC)


all_files = os.listdir(LOGDIR)
regressed = []
better = []
unchanged = []
firsttime = []

#Go through all log files and find which tests have performed better.
for logfile in all_files:
    (line1, line2) = getLastTwoLines(logfile, LOGDIR)
    log1 = processLogLine(line1)
    if line2 == '\n': # Empty line, only one test ever run
        firsttime.append(log1)
        continue
    log2 = processLogLine(line2)
    res = Result(log1.testname, log1.real, log2.real, log2.revision,\
    log2.branch, log1.revision, log1.branch)
    if res.percentage < -PERCENTAGE:
        regressed.append(res)
    elif res.change > PERCENTAGE:
        better.append(res)
    else:
        unchanged.append(res)

printResults(regressed, better, unchanged, firsttime)
