#!/bin/bash

PATH="/opt/local/bin:/opt/local/sbin:/opt/local/bin:/opt/local/sbin:/opt/local/bin:/opt/local/sbin:/opt/local/bin:/opt/local/sbin:/usr/bin:/bin:/usr/sbin:/sbin:/usr/local/bin:/usr/X11/bin:/usr/texbin"
cd /Users/hieuhoang/unison/workspace/data/europarl/exp/fr-en
echo 'starting at '`date`' on '`hostname`
mkdir -p /Users/hieuhoang/unison/workspace/data/europarl/exp/fr-en/tuning

/Users/hieuhoang/workspace/bin/current/scripts/training/mert-moses.pl /Users/hieuhoang/unison/workspace/data/europarl/exp/fr-en/tuning/input.lc.2 /Users/hieuhoang/unison/workspace/data/europarl/exp/fr-en/tuning/reference.lc.1 /Users/hieuhoang/workspace/bin/current/dist/bin/moses_chart /Users/hieuhoang/unison/workspace/data/europarl/exp/fr-en/tuning/sa/moses.ini.3 --nbest 100 --working-dir /Users/hieuhoang/unison/workspace/data/europarl/exp/fr-en/tuning/tmp.3  --decoder-flags "-threads 2 -cube-pruning-pop-limit 100 -s 30 -v 0 " --rootdir /Users/hieuhoang/workspace/bin/current/scripts -mertdir /Users/hieuhoang/workspace/bin/current/dist/bin --no-filter-phrase-table --continue

mkdir -p /Users/hieuhoang/unison/workspace/data/europarl/exp/fr-en/tuning
cp /Users/hieuhoang/unison/workspace/data/europarl/exp/fr-en/tuning/tmp.3/moses.ini /Users/hieuhoang/unison/workspace/data/europarl/exp/fr-en/tuning/moses.ini.3

echo 'finished at '`date`
touch /Users/hieuhoang/unison/workspace/data/europarl/exp/fr-en/steps/3/TUNING_tune.3.DONE
