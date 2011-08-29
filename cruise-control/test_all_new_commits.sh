#!/bin/bash
# given a config file runs tests on all untested commits of the scanned branches
# storing detailed logs to logs/CONFIGNAME/commit
# and extending the file brief.log
#
# A commit is assumed to be tested, if logs/CONFIGNAME/commit exists
#
# Ondrej Bojar, 2011

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

# this is where moses is taken from
GITREPO=$MCC_GITREPO
[ -d "$GITREPO" ] || GITREPO=/home/obo/moses-at-google-code


if [ ! -d "$WORKDIR" ]; then
  mkdir $(dirname "$WORKDIR") || die "Failed to create workdir $WORKDIR"
  warn "Cloning $GITREPO into $WORKDIR"
  git clone "$GITREPO" $WORKDIR \
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
  longlog="$LOGDIR/logs/$configname/$commit"
  if [ -e "$longlog" ]; then
    # Commit already tested
    return
  fi

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
  cat $configf >> $longlog
  echo "## Envinronment" >> $longlog
  uname -a >> $longlog
  env >> $longlog


  pushd $WORKDIR 2>/dev/null >/dev/null || die "Failed to chdir to $WORKDIR"
  git checkout --force $commit 2>/dev/null || die "Failed to checkout commit $commit"

  err=""
  echo "## regenerate-makefiles.sh" >> $longlog
  ./regenerate-makefiles.sh >> $longlog 2>&1 || err="regenerate-makefiles"

  echo "## make clean" >> $longlog
  make clean >> $longlog 2>&1 || warn "make clean failed, suspicious"

  echo "## ./configure $MCC_CONFIGURE_ARGS" >> $longlog
  [ -z "$err" ] && ./configure $MCC_CONFIGURE_ARGS  >> $longlog 2>&1 \
    || err="configure"

  echo "## make" >> $longlog
  [ -z "$err" ] && make  >> $longlog 2>&1 \
    || err="make"

  cd regression-testing
  echo "## Not running any regression tests yet." >> $longlog

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

  popd > /dev/null 2> /dev/null
}


#### Main loop over all commits
( cd "$WORKDIR" && git rev-list $MCC_SCAN_BRANCHES ) \
| while read commit; do
  run_single_test $commit || die "Testing failed, stopping the loop."
done

