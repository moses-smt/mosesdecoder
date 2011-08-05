#!/usr/bin/env python

#
# Mert test suite.
# Created by Barry Haddow
#
# This script downloads data from www.statmt.org, and runs tests of mert,
# comparing weights against expected and producing timing  information.
#

import ConfigParser
import logging
import optparse
import os
import os.path
import re
import string
import subprocess
import sys
import time
import urllib
import warnings

warnings.filterwarnings(action="ignore",message="tmpnam")

log = logging.getLogger("testmert")
dataurl = "http://www.statmt.org/moses/reg-testing/mert/"

def getMertDirectory():
    scriptdir = os.path.dirname(__file__)
    if not os.path.isabs(scriptdir):
        scriptdir = os.path.join(os.getcwd(),scriptdir)
        scriptdir = os.path.normpath(scriptdir)
    return os.path.dirname(scriptdir)

class Mert:
    """Controls operation of mert loop"""
    def __init__(self,weightfile,reffile,scorertype="BLEU",retries="20"):
        self.reffile = reffile
        self.scorertype = scorertype
        self.workingdir = os.tmpnam()
        os.mkdir(self.workingdir)
        self.mertdir = getMertDirectory()
        self.iteration = 1 # iteration number of inner loop
        self.retries = retries
        self.extractortimes = []
        self.merttimes = []
        os.system("cp %s %s" % \
            (weightfile,self.getFileName("weights",self.iteration-1)))
        # calculate dimension from weight file
        weightfh = open(weightfile)
        line = weightfh.readline()
        self.dimension = repr(len(line.split()))
        weightfh.close()

    def innerLoop(self, nbestfile):
        """Perform iteration of the inner loop. Returns location of 
        weights file"""
        log.debug("Inner loop: %d" % self.iteration)
        # run extractor
        scorefile = self.getFileName("scores",self.iteration)
        featurefile = self.getFileName("features",self.iteration)
        weightinfile = self.getFileName("weights",self.iteration-1)
        cmd = [os.path.join(self.mertdir,"extractor"),"--reference", 
          self.reffile, "--nbest",nbestfile,"--sctype",self.scorertype,\
          "--scfile", scorefile,"--ffile",featurefile]
        if self.iteration > 1:
            prevscorefile = self.getFileName("scores",self.iteration-1)
            prevfeaturefile = self.getFileName("features",self.iteration-1)
            cmd = cmd + ["--prev-scfile",prevscorefile , "--prev-ffile", prevfeaturefile ]
        log.debug("Running: " + string.join(cmd))
        start = time.time()
        ret = subprocess.call(cmd)
        self.extractortimes.append(time.time()-start)
        if ret != 0:
            raise RuntimeError("Failed to execute extractor: return code %d" % ret)

        # run mert
        cmd = [os.path.join(self.mertdir,"mert"),"--sctype",\
         self.scorertype, "--scfile", scorefile, "--ffile", featurefile,\
         "--ifile",weightinfile, "-d", self.dimension,"-n",self.retries]
        log.debug("Running: " + string.join(cmd))
        start = time.time()
        ret = subprocess.call(cmd, cwd=self.workingdir)
        self.merttimes.append(time.time()-start)
        if ret != 0:
            raise RuntimeError("Failed to execute mert: return code %d" % ret)
        weightoutfile = self.getFileName("weights",self.iteration)
        os.system("mv %s %s" % (os.path.join(self.workingdir,\
            "weights.txt"), weightoutfile))
        self.iteration = self.iteration + 1
        return weightoutfile

    def getFileName(self,stem,iteration):
        return os.path.join(self.workingdir,stem+"."+repr(iteration))

    def cleanup(self):
        os.system("rm -rf %s" % self.workingdir)

class Test:
    """A mert test"""
    def __init__(self,datadir):
        self.datadir = datadir
        config = ConfigParser.ConfigParser()
        config.read(os.path.join(datadir,"config"))
        self.iterations = config.getint("test","iterations")
        log.debug("Test iterations: %d" % self.iterations)
        self.tolerance = 0.00001

    def run(self):
        """Run the test, return a boolean indicating success or failure"""
        weightfile = os.path.join(self.datadir,"weights.0")
        reffile = os.path.join(self.datadir,"reference")
        self.mert = Mert(weightfile,reffile)
        self.diffs = []
        for i in range(self.iterations):
            nbestfile = os.path.join(self.datadir,"nbest." + repr(i+1) + ".gz")
            weightfile = self.mert.innerLoop(nbestfile)
            expectedweightfile = os.path.join(self.datadir,"weights."+repr(i+1))
            expectedweights = self.getWeights(expectedweightfile)
            weights = self.getWeights(weightfile)
            log.debug("Expected weights: " + repr(expectedweights))
            log.debug("Actual weights: " + repr(weights))
            diff = False
            for j in range(len(weights)):
                if abs(weights[j]-expectedweights[j]) > self.tolerance:
                    log.debug("Weight %d does not match: " % j)
                    diff = True
                    break
            else:
                log.debug("Weights match expected")
            self.diffs.append(diff)
        self.mert.cleanup()
    
    def getWeights(self,weightfile):
        """Load a weight set from a file"""
        weightfh = open(weightfile)
        line = weightfh.readline()
        weights = [float(w) for w in line.split()]
        weightfh.close()
        return weights

    def printSummary(self):
        """Print a summary of the results"""
        print "RESULTS: ", self.datadir
        print "Weights matching expected: ",
        for diff in self.diffs:
            print not diff,
        print
        print "Extractor times: ",
        for etime in self.mert.extractortimes:
            print "%7.3f" % etime,
        print "ave: %7.3f" % (sum(self.mert.extractortimes)/self.iterations) 
        print "Optimisation times: ",
        for mtime in self.mert.merttimes:
            print "%7.3f" % mtime,
        print "ave: %7.3f" % (sum(self.mert.merttimes)/self.iterations)

def getTestList():
   listfh  = urllib.urlopen(os.path.join(dataurl,"tests.txt")) 
   tests = []
   for line in listfh:
       tests.append(line[:-1])
   listfh.close()
   return tests

def list():
    """List all available tests"""
    tests = getTestList()
    print "Available tests:"
    for test in tests:
        print test

def runAll(datadir):
    """Run all available tests"""
    for test in getTestList():
        runTest(test,datadir)

def runTest(testname,datadir):
    log.info("Test started: " + testname)
    if not os.path.isdir(datadir):
        os.mkdir(datadir)
    testdir = os.path.join(datadir,testname)
    # Check if the test exists, download if necessary
    if os.path.isdir(testdir):
        log.debug("Directory %s already exists: not downloading" % testdir)
    else:
        testurl = os.path.join(dataurl,testname + ".tgz")
        log.debug("Retrieving test data from " + testurl)
        (arname,headers) = urllib.urlretrieve(testurl)
        os.system("cd %s; tar zxf %s" % (datadir,arname))
        log.debug("Done")
        if not os.path.isdir(testdir):
            raise RuntimeError("Test %s did not unpack properly" % testname)
    test = Test(testdir)
    test.run()
    test.printSummary()
    log.info("Test ended: " + testname)

def main():
    logging.basicConfig(level = logging.DEBUG)
    parser = optparse.OptionParser("usage: %prog [options] list|run|runall [testname]")
    parser.add_option("-d", "--datadir", action="store", default="data",
        dest="datadir", help="Data directory to use", metavar="DIR")
    (options,args) = parser.parse_args() 
    if len(args) < 1:
        parser.error("Need to specify an action")
    if args[0] == "list":
        list()
    else:
        datadir = options.datadir
        if args[0] == "runall":
            runAll(datadir)
        elif args[0] == "run":
            if len(args) < 2:
                parser.error("The run action requires a test name")
            runTest(args[1],datadir)

if __name__ == "__main__":
    main()
