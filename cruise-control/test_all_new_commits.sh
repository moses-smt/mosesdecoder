#!/bin/bash
# given a config file runs tests on all untested commits of the scanned branches
# storing detailed logs to logs/CONFIGNAME/commit
# and extending the file brief.log
#
# A commit is assumed to be tested, if logs/CONFIGNAME/commit exists
#
# Ondrej Bojar, Ales Tamchyna, 2011

function warn() { echo "$@" >&2; }
function die() { echo "$@" >&2; exit 1; }
set -o pipefail  # safer pipes

configf="$1"
[ -e "$configf" ] || die "usage: $0 configfile"
configname=$(basename $configf | sed 's/\.config$//')

source "$configf"

[ -z "$MCC_SCAN_BRANCHES" ] \
  && die "Bad config $configf; does not define MCC_SCAN_BRANCHES"

# use the given tempdir or make subdir tmp here
USE_TEMPDIR=$MCC_TEMPDIR
[ -d "$USE_TEMPDIR" ] || USE_TEMPDIR=./tmp

LOGDIR=$MCC_LOGDIR
[ -d "$LOGDIR" ] || LOGDIR=.

# ensure full path for logdir
LOGDIR=$(readlink -f "$LOGDIR")
[ -d "$LOGDIR" ] || die "Fatal: confusing readlink for $LOGDIR"

# this is where moses is cloned into
WORKDIR=$MCC_WORKDIR
[ -d "$WORKDIR" ] || WORKDIR=$USE_TEMPDIR/workdir

MYDIR=$(pwd)

# this is where moses is taken from
GITREPO="$MCC_GITREPO"
[ -n "$GITREPO" ] || GITREPO=/home/obo/moses-at-google-code

# location of moses regression test data archive (assumes url at the moment)
REGTEST_ARCHIVE="$MCC_REGTEST_ARCHIVE"
[ -n "$REGTEST_ARCHIVE" ] \
  || REGTEST_ARCHIVE="git://github.com/moses-smt/moses-regression-tests.git"

if [ ! -d "$WORKDIR" ]; then
  mkdir "$WORKDIR" || die "Failed to create workdir $WORKDIR"
  warn "Cloning $GITREPO into $WORKDIR"
  git clone $GITREPO $WORKDIR \
    || die "Failed to git clone into workdir $WORKDIR"
else
  ( cd "$WORKDIR" && git fetch ) \
    || die "Failed to update our clone at $WORKDIR"
fi

mkdir -p $LOGDIR/logs/$configname \
  || die "Failed to create dir $LOGDIR/logs/$configname"

#### How is one test performed
function run_single_test () {
  commit=$1
  first_char=$(echo $commit | grep -o '^.')
  longlog="$LOGDIR/logs/$configname/$first_char/$commit.log"
  warn "Testing commit $commit"

  # Get the version of this script
  ccversion=$(svnversion 2>/dev/null)
  [ ! -z "$ccversion" ] || ccversion=$(git show 2>&1 | head -n 1)
  [ ! -z "$ccversion" ] || ccversion="unknown"

  # Create log header with computer details:
  echo "#### Moses Cruise Control Log for commit $commit" > $longlog
  date >> $longlog
  echo "## Cruise Control version" >> $longlog
  echo $ccversion >> $longlog
  echo "## Parameters" >> $longlog
  cat $MYDIR/$configf >> $longlog
  echo "## Envinronment" >> $longlog
  uname -a >> $longlog
  env >> $longlog

  git checkout --force $commit 2>/dev/null || die "Failed to checkout commit $commit"

  err=""

   cd regression-testing
  #regtest_file=$(echo "$REGTEST_ARCHIVE" | sed 's/^.*\///')

  # download data for regression tests if necessary
  regtest_dir=$PWD/moses-reg-test-data
  if [ -e $regtest_dir ]; then
    (cd $regtest_dir; git pull)  &> /dev/null ||
      die "Failed to update regression testing data"
  else
    git clone $REGTEST_ARCHIVE $regtest_dir &> /dev/null ||
      die "Failed to clone regression testing data"
  fi
  #if [ ! -f $regtest_file.ok ]; then
  #  wget $REGTEST_ARCHIVE &> /dev/null \
  #    || die "Failed to download data for regression tests"
  #  tar xzf $regtest_file
  #  touch $regtest_file.ok
  #fi
  #regtest_dir=$PWD/$(basename $regtest_file .tgz)
  cd ..


  echo "## ./bjam clean" >> $longlog
  ./bjam clean $MCC_CONFIGURE_ARGS --with-regtest=$regtest_dir >> $longlog 2>&1 || warn "bjam clean failed, suspicious"

  echo "## ./bjam $MCC_CONFIGURE_ARGS" >> $longlog
  if [ -z "$err" ]; then
    ./bjam $MCC_CONFIGURE_ARGS >> $longlog 2>&1 || err="bjam"
  fi
  
  echo "## regression tests" >> $longlog
  if [ -z "$err" ]; then
    ./bjam $MCC_CONFIGURE_ARGS --with-regtest=$regtest_dir >> $longlog 2>&1 || err="regression tests"
  fi

  if [ -z "$err" ] && [ "$MCC_RUN_EMS" = "yes" ]; then
    echo "## EMS" >> $longlog
    if [ ! -f "giza-pp.ok" ]; then # fetch & compile Giza++
      svn checkout http://giza-pp.googlecode.com/svn/trunk/ giza-pp \
      || die "Failed to fetch Giza++"
      cd giza-pp && make || die "Failed to compile Giza++"
      mkdir -p bin
      ln -s ../GIZA++-v2/GIZA++ ../GIZA++-v2/snt2cooc.out ../mkcls-v2/mkcls bin/
      cd ..
      touch giza-pp.ok
    fi
    ./bjam $MCC_CONFIGURE_ARGS --with-giza="$(pwd)/giza-pp/bin" || err="bjam with-giza"
    srilm_dir=$(echo $MCC_CONFIGURE_ARGS | sed -r 's/.*--with-srilm=([^ ]+) .*/\1/')
    mach_type=$($srilm_dir/sbin/machine-type)
    mkdir -p "$WORKDIR/ems_workdir"
    rm -rf "$WORKDIR/ems_workdir/"* # clean any previous experiments
    cat $MYDIR/config.ems \
    | sed \
      -e "s#WORKDIR#$WORKDIR#" \
      -e "s#SRILMDIR#$srilm_dir#" \
      -e "s#MACHINE_TYPE#$mach_type#" \
    > ./config.ems
    scripts/ems/experiment.perl \
      -no-graph -exec -config $(pwd)/config.ems &>> $longlog \
      || die "Running EMS failed"
    [ -f $WORKDIR/ems_workdir/steps/1/REPORTING_report.1.DONE ] || err="ems"
  fi

  echo "## Finished" >> $longlog
  date >> $longlog

  if [ -z "$err" ]; then
    status="OK"
  else
    status="FAIL:$err"
  fi
  echo "## Status: $status" >> $longlog
  
  nicedate=$(date +"%Y%m%d-%H%M%S")
  echo "$commit	$status	$configname	$ccversion	$nicedate" \
    >> "$LOGDIR/brief.log"

  if [ -z "$err" ]; then
    touch "$LOGDIR/logs/$configname/$first_char/$commit.OK"
  else
    return 1;
  fi
}

cd $WORKDIR || die "Failed to chdir to $WORKDIR"

# update the revision lists for all watched branches
for i in $MCC_SCAN_BRANCHES; do
  git rev-list $i > "$LOGDIR/logs/$configname/$(echo -n $i | sed 's/^.*\///').revlist"
done

# create info files for new commits
for i in $(git rev-list $MCC_SCAN_BRANCHES); do
  first_char=$(echo $i | grep -o '^.')
  mkdir -p "$LOGDIR/logs/$configname/$first_char" 
  [ -f "$LOGDIR/logs/$configname/$first_char/$i.info" ] && break;
  git show $i | $MYDIR/shorten_info.pl > "$LOGDIR/logs/$configname/$first_char/$i.info"
done

#### Main loop over all commits
for i in $MCC_SCAN_BRANCHES; do
  warn "On brach $i"
  git rev-list $i \
  | while read commit; do
    first_char=$(echo $commit | grep -o '^.')
    test_done="$LOGDIR/logs/$configname/$first_char/$commit.log"
    if [ ! -e "$test_done" ]; then
      run_single_test $commit && warn "Commit $commit test ok, stopping" && break
      warn "Commit $commit test failed, continuing"
    else
      warn "Reached a previously tested commit ($commit), stopping"
      break
    fi
  done
done
