# Moses speedtesting framework 

### Description

This is an automatic test framework that is designed to test the day to day performance changes in Moses.

### Set up

#### Set up a Moses repo
Set up a Moses repo and build it with the desired configuration.
```bash
git clone https://github.com/moses-smt/mosesdecoder.git
cd mosesdecoder
./bjam -j10 --with-cmph=/usr/include/
```
You need to build Moses first, so that the testsuite knows what command you want it to use when rebuilding against newer revisions.

#### Create a parent directory.
Create a parent directory where the **runtests.py** and related scripts and configuration file should reside.
This should also be the location of the TEST_DIR and TEST_LOG_DIR as explained in the next section.

#### Set up a global configuration file.
You need a configuration file for the testsuite. A sample configuration file is provided in **testsuite\_config**
<pre>
MOSES_REPO_PATH: /home/moses-speedtest/moses-standard/mosesdecoder
DROP_CACHES_COMM: sys_drop_caches 3
TEST_DIR: /home/moses-speedtest/phrase_tables/tests
TEST_LOG_DIR: /home/moses-speedtest/phrase_tables/testlogs
BASEBRANCH: RELEASE-2.1.1
MOSES_PROFILER_REPO: /home/moses-speedtest/moses-standard/mosesdecoder-variant-prof
MOSES_GOOGLE_PROFILER_REPO: /home/moses-speedtest/moses-standard/mosesdecoder-variant-gperftools
</pre>

The _MOSES\_REPO\_PATH_ is the place where you have set up and built moses.
The _DROP\_CACHES\_COMM_ is the command that would be used to drop caches. It should run without needing root access.
_TEST\_DIR_ is the directory where all the tests will reside.
_TEST\_LOG\_DIR_ is the directory where the performance logs will be gathered. It should be created before running the testsuite for the first time.
_BASEBRANCH_ is the branch against which all new tests will be compared. It should normally be set to be the latest Moses stable release.
_MOSES\_PROFILER\_REPO_ is a path to a moses repository set up and built with profiling enabled. Optional if you want to produce profiling results.
_MOSES\_GOOGLE\_PROFILER\_REPO is a path to moses repository set up with full tcmalloc and profiler, as well as shared link for use with gperftools.
### Creating tests

In order to create a test one should go into the TEST_DIR and create a new folder. That folder will be used for the name of the test.
Inside that folder one should place a configuration file named **config**. The naming is mandatory.
An example such configuration file is **test\_config**

<pre>
Command: moses -f ... -i fff #Looks for the command in the /bin directory of the repo specified in the testsuite_config
LDPRE: ldpreloads #Comma separated LD_LIBRARY_PATH:/, 
Variants: vanilla, cached, ldpre, profile, google-profiler #Can't have cached without ldpre or vanilla
</pre>

The _Command:_ line specifies the executable (which is looked up in the /bin directory of the repo.) and any arguments necessary. Before running the test, the script cds to the current test directory so you can use relative paths.
The _LDPRE:_ specifies if tests should be run with any LD\_PRELOAD flags.
The _Variants:_ line specifies what type of tests should we run. This particular line will run the following tests:
1. A Vanilla test meaning just the command after _Command_ will be issued.
2. A vanilla cached test meaning that after the vanilla test, the test will be run again without dropping caches in order to benchmark performance on cached filesystem.
3. A test with LD_PRELOAD ldpreloads moses -f command. For each available LDPRELOAD comma separated library to preload.
4. A cached version of all LD_PRELOAD tests.
5. A profile variant is only available if you have setup the profiler repository. It produces gprof outputs for all of the above in a subdirectory inside the _TEST\_LOG\_DIR.

#### Produce profiler results.
If you want to produce profiler results together in some tests you need to specify the _MOSES\_PROFILER\_REPO_ in the config
```bash
git clone https://github.com/moses-smt/mosesdecoder.git mosesdecoder-profile
cd mosesdecoder-profile
./bjam -j10 --with-cmph=/usr/include/ variant=profile
```

Afterwards for testcases which contain the **profile** keyword in **Variants** you will see a directory inside _TEST\_LOG\_DIR which contains the **gprof** output from every run (files ending in **\_profile**).

#### Produce google profiler results.
If you want to produce profiler results together in some tests you need to specify the _MOSES\_GOOGLE\_PROFILER\_REPO in the config
```bash
git clone https://github.com/moses-smt/mosesdecoder.git mosesdecoder-google-profile
cd mosesdecoder
./bjam link=shared -j10 --full-tcmalloc --with-cmph=/usr/include/
```

Afterwards for testcases which contain the **google-profiler** keyword in **Variants** you will see a directory inside _TEST\_LOG\_DIR which contains the **google-profiler** output from every run (files prefixed with **pprof**). To analyze the output you need to use [pprof](http://google-perftools.googlecode.com/svn/trunk/doc/cpuprofile.html).

### Running tests.
Running the tests is done through the **runtests.py** script.

#### Running all tests.
To run all tests, with the base branch and the latests revision (and generate new basebranch test data if such is missing) do a:
```bash
python3 runtests.py -c testsuite_config
```

#### Running specific tests.
The script allows the user to manually run a particular test or to test against a specific branch or revision:
<pre>
moses-speedtest@crom:~/phrase_tables$ python3 runtests.py --help
usage: runtests.py [-h] -c CONFIGFILE [-s SINGLETESTDIR] [-r REVISION]
                   [-b BRANCH]

A python based speedtest suite for moses.

optional arguments:
  -h, --help            show this help message and exit
  -c CONFIGFILE, --configfile CONFIGFILE
                        Specify test config file
  -s SINGLETESTDIR, --singletest SINGLETESTDIR
                        Single test name directory. Specify directory name,
                        not full path!
  -r REVISION, --revision REVISION
                        Specify a specific revison for the test.
  -b BRANCH, --branch BRANCH
                        Specify a branch for the test.
</pre>

### Generating HTML report.
To generate a summary of the test results use the **html\_gen.py** script. It places a file named *index.html* in the current script directory.
```bash
python3 html_gen.py testsuite_config
```
You should use the generated file with the **style.css** file provided in the html directory.

### Command line regression testing.
Alternatively you could check for regressions from the command line using the **check\_fo\r_regression.py** script:
```bash
python3 check_for_regression.py TESTLOGS_DIRECTORY
```

Alternatively the results of all tests are logged inside the the specified TESTLOGS directory so you can manually check them for additional information such as date, time, revision, branch, etc...

### Create a cron job:
Create a cron job to run the tests daily and generate an html report. An example *cronjob* is available.
```bash
#!/bin/sh
cd /home/moses-speedtest/phrase_tables

python3 runtests.py -c testsuite_config #Run the tests.
python3 html_gen.py testsuite_config #Generate html

cp index.html /fs/thor4/html/www/speed-test/ #Update the html
```

Place the script in _/etc/cron.daily_ for dayly testing

###### Author
Nikolay Bogoychev, 2014

###### License
This software is licensed under the LGPL.