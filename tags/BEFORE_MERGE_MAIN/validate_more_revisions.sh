#!/bin/bash

from=$1
to=$2

logdir=./revision_status_log

if [ "$from" == "" ] || [ "$to" == "" ]; then
  cat <<KONEC
./validate_more_revisions.sh <fromrev> <torev>
  will run ./validate_revision.sh for a sequence of revisions and
  collect logfiles to "$logdir"
KONEC
  exit 1
fi


mkdir -p $logdir

tmpdir=/tmp/validate-more-revisions-tmp
if [ -e $tmpdir ]; then
  echo "$0 seems to be already running!"
  echo "If this is a false alarm, remove our temp directory:"
  echo "   rm -rf $tmpdir"
  exit 1;
fi

mkdir -p $tmpdir

# need to save a copy of the helper script validate_revision.sh, 
# because previous releases might have missed it
cp ./validate_revision.sh $tmpdir/

for i in `seq $from $to`; do
  echo "Validating $i...";
  $tmpdir/validate_revision.sh $i > $logdir/$i.log 2>&1
  tail -1 $logdir/$i.log | sed 's/^/   /'
done
rm -rf $tmpdir
echo "Finished validating, now at revision $to"
