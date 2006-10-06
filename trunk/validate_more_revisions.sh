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

for i in `seq $from $to`; do
  echo "Validating $i...";
  ./validate_revision.sh $i > $logdir/$i.log 2>&1
  tail -1 $logdir/$i.log | sed 's/^/   /'
done
echo "Finished validating"
