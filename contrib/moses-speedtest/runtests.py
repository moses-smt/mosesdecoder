"""Given a config file, runs tests"""
import os
import subprocess
import time
import shutil
from argparse import ArgumentParser
from testsuite_common import processLogLine

def parse_cmd():
    """Parse the command line arguments"""
    description = "A python based speedtest suite for moses."
    parser = ArgumentParser(description=description)
    parser.add_argument("-c", "--configfile", action="store",\
                dest="configfile", required=True,\
                help="Specify test config file")
    parser.add_argument("-s", "--singletest", action="store",\
                dest="singletestdir", default=None,\
                help="Single test name directory. Specify directory name,\
                not full path!")
    parser.add_argument("-r", "--revision", action="store",\
                dest="revision", default=None,\
                help="Specify a specific revison for the test.")
    parser.add_argument("-b", "--branch", action="store",\
                dest="branch", default=None,\
                help="Specify a branch for the test.")

    arguments = parser.parse_args()
    return arguments

def repoinit(testconfig, profiler=None):
    """Determines revision and sets up the repo. If given the profiler optional
    argument, wil init the profiler repo instead of the default one."""
    revision = ''
    #Update the repo
    if profiler == "gnu-profiler":
        if testconfig.repo_prof is not None:
            os.chdir(testconfig.repo_prof)
        else:
            raise ValueError('Profiling repo is not defined')
    elif profiler == "google-profiler":
        if testconfig.repo_gprof is not None:
            os.chdir(testconfig.repo_gprof)
        else:
            raise ValueError('Profiling repo is not defined')
    else:
        os.chdir(testconfig.repo)
    #Checkout specific branch, else maintain main branch
    if testconfig.branch != 'master':
        subprocess.call(['git', 'checkout', testconfig.branch])
        rev, _ = subprocess.Popen(['git', 'rev-parse', 'HEAD'],\
            stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()
        revision = str(rev).replace("\\n'", '').replace("b'", '')
    else:
        subprocess.call(['git checkout master'], shell=True)

    #Check a specific revision. Else checkout master.
    if testconfig.revision:
        subprocess.call(['git', 'checkout', testconfig.revision])
        revision = testconfig.revision
    elif testconfig.branch == 'master':
        subprocess.call(['git pull'], shell=True)
        rev, _ = subprocess.Popen(['git rev-parse HEAD'], stdout=subprocess.PIPE,\
            stderr=subprocess.PIPE, shell=True).communicate()
        revision = str(rev).replace("\\n'", '').replace("b'", '')

    return revision

class Configuration:
    """A simple class to hold all of the configuration constatns"""
    def __init__(self, repo, drop_caches, tests, testlogs, basebranch, baserev, repo_prof=None, repo_gprof=None):
        self.repo = repo
        self.repo_prof = repo_prof
        self.repo_gprof = repo_gprof
        self.drop_caches = drop_caches
        self.tests = tests
        self.testlogs = testlogs
        self.basebranch = basebranch
        self.baserev = baserev
        self.singletest = None
        self.revision = None
        self.branch = 'master' # Default branch

    def additional_args(self, singletest, revision, branch):
        """Additional configuration from command line arguments"""
        self.singletest = singletest
        if revision is not None:
            self.revision = revision
        if branch is not None:
            self.branch = branch

    def set_revision(self, revision):
        """Sets the current revision that is being tested"""
        self.revision = revision


class Test:
    """A simple class to contain all information about tests"""
    def __init__(self, name, command, ldopts, permutations, prof_command=None, gprof_command=None):
        self.name = name
        self.command = command
        self.prof_command = prof_command
        self.gprof_command = gprof_command
        self.ldopts = ldopts.replace(' ', '').split(',') #Not tested yet
        self.permutations = permutations

def parse_configfile(conffile, testdir, moses_repo, moses_prof_repo=None, moses_gprof_repo=None):
    """Parses the config file"""
    command, ldopts, prof_command, gprof_command = '', '', None, None
    permutations = []
    fileopen = open(conffile, 'r')
    for line in fileopen:
        line = line.split('#')[0] # Discard comments
        if line == '' or line == '\n':
            continue # Discard lines with comments only and empty lines
        opt, args = line.split(' ', 1) # Get arguments

        if opt == 'Command:':
            command = args.replace('\n', '')
            if moses_prof_repo is not None:  # Get optional command for profiling
                prof_command = moses_prof_repo + '/bin/' + command
            if moses_gprof_repo is not None: # Get optional command for google-perftools
                gprof_command = moses_gprof_repo + '/bin/' + command
            command = moses_repo + '/bin/' + command
        elif opt == 'LDPRE:':
            ldopts = args.replace('\n', '')
        elif opt == 'Variants:':
            permutations = args.replace('\n', '').replace(' ', '').split(',')
        else:
            raise ValueError('Unrecognized option ' + opt)
    #We use the testdir as the name.
    testcase = Test(testdir, command, ldopts, permutations, prof_command, gprof_command)
    fileopen.close()
    return testcase

def parse_testconfig(conffile):
    """Parses the config file for the whole testsuite."""
    repo_path, drop_caches, tests_dir, testlog_dir = '', '', '', ''
    basebranch, baserev, repo_prof_path, repo_gprof_path = '', '', None, None
    fileopen = open(conffile, 'r')
    for line in fileopen:
        line = line.split('#')[0] # Discard comments
        if line == '' or line == '\n':
            continue # Discard lines with comments only and empty lines
        opt, args = line.split(' ', 1) # Get arguments
        if opt == 'MOSES_REPO_PATH:':
            repo_path = args.replace('\n', '')
        elif opt == 'DROP_CACHES_COMM:':
            drop_caches = args.replace('\n', '')
        elif opt == 'TEST_DIR:':
            tests_dir = args.replace('\n', '')
        elif opt == 'TEST_LOG_DIR:':
            testlog_dir = args.replace('\n', '')
        elif opt == 'BASEBRANCH:':
            basebranch = args.replace('\n', '')
        elif opt == 'BASEREV:':
            baserev = args.replace('\n', '')
        elif opt == 'MOSES_PROFILER_REPO:':  # Optional
            repo_prof_path = args.replace('\n', '')
        elif opt == 'MOSES_GOOGLE_PROFILER_REPO:':  # Optional
            repo_gprof_path = args.replace('\n', '')
        else:
            raise ValueError('Unrecognized option ' + opt)
    config = Configuration(repo_path, drop_caches, tests_dir, testlog_dir,\
    basebranch, baserev, repo_prof_path, repo_gprof_path)
    fileopen.close()
    return config

def get_config():
    """Builds the config object with all necessary attributes"""
    args = parse_cmd()
    config = parse_testconfig(args.configfile)
    config.additional_args(args.singletestdir, args.revision, args.branch)
    revision = repoinit(config)
    if config.repo_prof is not None:
        repoinit(config, "gnu-profiler")
    if config.repo_gprof is not None:
        repoinit(config, "google-profiler")
    config.set_revision(revision)
    return config

def check_for_basever(testlogfile, basebranch):
    """Checks if the base revision is present in the testlogs"""
    filetoopen = open(testlogfile, 'r')
    for line in filetoopen:
        templine = processLogLine(line)
        if templine.branch == basebranch:
            return True
    return False

def split_time(filename):
    """Splits the output of the time function into seperate parts.
    We will write time to file, because many programs output to
    stderr which makes it difficult to get only the exact results we need."""
    timefile = open(filename, 'r')
    realtime = float(timefile.readline().replace('\n', '').split()[1])
    usertime = float(timefile.readline().replace('\n', '').split()[1])
    systime = float(timefile.readline().replace('\n', '').split()[1])
    timefile.close()

    return (realtime, usertime, systime)


def write_log(time_file, logname, config):
    """Writes to a logfile"""
    log_write = open(config.testlogs + '/' + logname, 'a') # Open logfile
    date_run = time.strftime("%d.%m.%Y %H:%M:%S") # Get the time of the test
    realtime, usertime, systime = split_time(time_file) # Get the times in a nice form

    # Append everything to a log file.
    writestr = date_run + " " + config.revision + " Testname: " + logname +\
    " RealTime: " + str(realtime) + " UserTime: " + str(usertime) +\
    " SystemTime: " + str(systime) + " Branch: " + config.branch +'\n'
    log_write.write(writestr)
    log_write.close()

def write_gprof(command, name, variant, config):
    """Produces a gprof report from a gmon file"""
    #Check if we have a directory for the profiling of this testcase:
    output_dir = config.testlogs + '/' + name
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    outputfile = output_dir + '/' + time.strftime("%d.%m.%Y_%H:%M:%S") + '_' + name + '_' + variant

    #Compile a gprof command and output the file in the directory we just created
    gmon_path = os.getcwd() + '/gmon.out'  # Path to the profiling file
    executable_path = command.split(' ')[0]  # Path to the moses binary
    gprof_command = 'gprof ' + executable_path + ' ' + gmon_path + ' > ' + outputfile
    subprocess.call([gprof_command], shell=True)
    os.remove(gmon_path)  # After we are done discard the gmon file

def write_pprof(name, variant, config):
    """Copies the google-perftools profiler output to the corresponding test directory"""
    output_dir = config.testlogs + '/' + name
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    outputfile = output_dir + '/pprof_' + time.strftime("%d.%m.%Y_%H:%M:%S") + '_' + name + '_' + variant
    shutil.move("/tmp/moses.prof", outputfile)


def execute_test(command, path, name, variant, config, profile=None):
    """Executes a testcase given a whole command, path to the test file output,
    name of the test and variant tested. Config is the global configuration"""
    subprocess.Popen([command], stdout=None, stderr=subprocess.PIPE, shell=True).communicate()
    if profile is None:
        write_log(path, name + '_' + variant, config)
    elif profile == "gnu-profiler":  # Basically produce a gmon output
        write_gprof(command, name, variant, config)
    elif profile == "google-profiler":
        write_pprof(name, variant, config)        


def execute_tests(testcase, cur_directory, config):
    """Executes timed tests based on the config file"""
    #Several global commands related to the time wrapper
    time_command = ' time -p -o /tmp/time_moses_tests '
    time_path = '/tmp/time_moses_tests'

    #Figure out the order of which tests must be executed.
    #Change to the current test directory
    os.chdir(config.tests + '/' + cur_directory)
    #Clear caches
    subprocess.call(['sync'], shell=True)
    subprocess.call([config.drop_caches], shell=True)
    #Perform vanilla test and if a cached test exists - as well
    print(testcase.name)
    if 'vanilla' in testcase.permutations:
        #Create the command for executing moses
        whole_command = time_command + testcase.command

        #test normal and cached
        execute_test(whole_command, time_path, testcase.name, 'vanilla', config)
        if 'cached' in testcase.permutations:
            execute_test(whole_command, time_path, testcase.name, 'vanilla_cached', config)

    #Now perform LD_PRELOAD tests
    if 'ldpre' in testcase.permutations:
        for opt in testcase.ldopts:
            #Clear caches
            subprocess.call(['sync'], shell=True)
            subprocess.call([config.drop_caches], shell=True)

            #Create the command for executing moses:
            whole_command = 'LD_PRELOAD=' + opt + time_command + testcase.command
            variant = 'ldpre_' + opt

            #test normal and cached
            execute_test(whole_command, time_path, testcase.name, variant, config)
            if 'cached' in testcase.permutations:
                execute_test(whole_command, time_path, testcase.name, variant + '_cached', config)

    #Perform profiling test. Mostly same as the above lines but necessary duplication.
    #All actual code is inside execute_test so those lines shouldn't need modifying
    if 'profile' in testcase.permutations:
        subprocess.call(['sync'], shell=True)  # Drop caches first
        subprocess.call([config.drop_caches], shell=True)

        if 'vanilla' in testcase.permutations:
            whole_command = testcase.prof_command
            execute_test(whole_command, time_path, testcase.name, 'profile', config, "gnu-profiler")
            if 'cached' in testcase.permutations:
                execute_test(whole_command, time_path, testcase.name, 'profile_cached', config, "gnu-profiler")

        if 'ldpre' in testcase.permutations:
            for opt in testcase.ldopts:
                #Clear caches
                subprocess.call(['sync'], shell=True)
                subprocess.call([config.drop_caches], shell=True)

                #Create the command for executing moses:
                whole_command = 'LD_PRELOAD=' + opt + " " + testcase.prof_command
                variant = 'profile_ldpre_' + opt

                #test normal and cached
                execute_test(whole_command, time_path, testcase.name, variant, config, "gnu-profiler")
                if 'cached' in testcase.permutations:
                    execute_test(whole_command, time_path, testcase.name, variant + '_cached', config, "gnu-profiler")

    #Google-perftools profiler
    if 'google-profiler' in testcase.permutations:
        subprocess.call(['sync'], shell=True)  # Drop caches first
        subprocess.call([config.drop_caches], shell=True)

        #Create the command for executing moses
        whole_command = "CPUPROFILE=/tmp/moses.prof " + testcase.gprof_command

        #test normal and cached
        execute_test(whole_command, time_path, testcase.name, 'vanilla', config, 'google-profiler')
        if 'cached' in testcase.permutations:
            execute_test(whole_command, time_path, testcase.name, 'vanilla_cached', config, 'google-profiler')

    #Now perform LD_PRELOAD tests
    if 'ldpre' in testcase.permutations:
        for opt in testcase.ldopts:
            #Clear caches
            subprocess.call(['sync'], shell=True)
            subprocess.call([config.drop_caches], shell=True)

            #Create the command for executing moses:
            whole_command = 'LD_PRELOAD=' + opt + " " + whole_command
            variant = 'ldpre_' + opt

            #test normal and cached
            execute_test(whole_command, time_path, testcase.name, variant, config, 'google-profiler')
            if 'cached' in testcase.permutations:
                execute_test(whole_command, time_path, testcase.name, variant + '_cached', config, 'google-profiler')


# Go through all the test directories and executes tests
if __name__ == '__main__':
    CONFIG = get_config()
    ALL_DIR = os.listdir(CONFIG.tests)

    #We should first check if any of the tests is run for the first time.
    #If some of them are run for the first time we should first get their
    #time with the base version (usually the previous release)
    FIRSTTIME = []
    TESTLOGS = []
    #Strip filenames of test underscores
    for listline in os.listdir(CONFIG.testlogs):
        listline = listline.replace('_vanilla', '')
        listline = listline.replace('_cached', '')
        listline = listline.replace('_ldpre', '')
        TESTLOGS.append(listline)
    for directory in ALL_DIR:
        if directory not in TESTLOGS:
            FIRSTTIME.append(directory)

    #Sometimes even though we have the log files, we will need to rerun them
    #Against a base version, because we require a different baseversion (for
    #example when a new version of Moses is released.) Therefore we should
    #Check if the version of Moses that we have as a base version is in all
    #of the log files.

    for logfile in os.listdir(CONFIG.testlogs):
        logfile_name = CONFIG.testlogs + '/' + logfile
        if os.path.isfile(logfile_name) and not check_for_basever(logfile_name, CONFIG.basebranch):
            logfile = logfile.replace('_vanilla', '')
            logfile = logfile.replace('_cached', '')
            logfile = logfile.replace('_ldpre', '')
            FIRSTTIME.append(logfile)
    FIRSTTIME = list(set(FIRSTTIME)) #Deduplicate

    if FIRSTTIME != []:
        #Create a new configuration for base version tests:
        BASECONFIG = Configuration(CONFIG.repo, CONFIG.drop_caches,\
            CONFIG.tests, CONFIG.testlogs, CONFIG.basebranch,\
            CONFIG.baserev, CONFIG.repo_prof, CONFIG.repo_gprof)
        BASECONFIG.additional_args(None, CONFIG.baserev, CONFIG.basebranch)
        #Set up the repository and get its revision:
        REVISION = repoinit(BASECONFIG)
        BASECONFIG.set_revision(REVISION)
        #Build
        os.chdir(BASECONFIG.repo)
        subprocess.call(['./previous.sh'], shell=True)
        #If profiler configuration exists also init it
        if BASECONFIG.repo_prof is not None:
            repoinit(BASECONFIG, "gnu-profiler")
            os.chdir(BASECONFIG.repo_prof)
            subprocess.call(['./previous.sh'], shell=True)

        if BASECONFIG.repo_gprof is not None:
            repoinit(BASECONFIG, "google-profiler")
            os.chdir(BASECONFIG.repo_gprof)
            subprocess.call(['./previous.sh'], shell=True)

        #Perform tests
        for directory in FIRSTTIME:
            cur_testcase = parse_configfile(BASECONFIG.tests + '/' + directory +\
            '/config', directory, BASECONFIG.repo, BASECONFIG.repo_prof, BASECONFIG.repo_gprof)
            execute_tests(cur_testcase, directory, BASECONFIG)

        #Reset back the repository to the normal configuration
        repoinit(CONFIG)
        if BASECONFIG.repo_prof is not None:
            repoinit(CONFIG, "gnu-profiler")

        if BASECONFIG.repo_gprof is not None:
            repoinit(CONFIG, "google-profiler")

    #Builds moses
    os.chdir(CONFIG.repo)
    subprocess.call(['./previous.sh'], shell=True)
    if CONFIG.repo_prof is not None:
        os.chdir(CONFIG.repo_prof)
        subprocess.call(['./previous.sh'], shell=True)

    if CONFIG.repo_gprof is not None:
        os.chdir(CONFIG.repo_gprof)
        subprocess.call(['./previous.sh'], shell=True)

    if CONFIG.singletest:
        TESTCASE = parse_configfile(CONFIG.tests + '/' +\
            CONFIG.singletest + '/config', CONFIG.singletest, CONFIG.repo, CONFIG.repo_prof, CONFIG.repo_gprof)
        execute_tests(TESTCASE, CONFIG.singletest, CONFIG)
    else:
        for directory in ALL_DIR:
            cur_testcase = parse_configfile(CONFIG.tests + '/' + directory +\
            '/config', directory, CONFIG.repo, CONFIG.repo_prof, CONFIG.repo_gprof)
            execute_tests(cur_testcase, directory, CONFIG)
