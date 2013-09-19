#!/bin/bash
# Hackish summarization of weight-scan.pl results, heavily relies on tools by
# Ondrej Bojar (bojar@ufal.mff.cuni.cz), some of which need Mercury; beware.

function die() { echo "$@" >&2; exit 1; }
set -o pipefail  # safer pipes

refs="$1"
dir="$2"

[ -d "$dir" ] && [ -e "$refs" ] \
  || die "usage: $0 ref-file weight-scan-working-dir"

testbleu=$HOME/tools/src/obotools/testbleu
projectbleu=$HOME/tools/src/obotools/projectbleu

[ -x "$testbleu" ] || die "Can't run $testbleu"
[ -x "$projectbleu" ] || die "Can't run $projectbleu"

# create exact bleus and put them to bleu.*
for f in $dir/out.*; do
  bleuf=${f//out./bleu.}
  [ -e "$bleuf" ] \
    || $testbleu $refs < $f | pickre --re='BLEU...([0-9.]*)' > $bleuf \
    || die "Failed to construct $bleuf"
done

# create bleu projections from each best* and put them to corresponding pbleu*
# first collect all weights
lcat $dir/weights.* \
| tr ' ' , \
| pickre --re='weights.([-0-9.]*)' \
| cut -f 1,3 \
| numsort 1 \
> $dir/allweights
allwparam=$(cut -f2 $dir/allweights | prefix -- '-w ' | tr '\n' ' ')
for f in $dir/best*.*; do
  pbleuf=$(echo $f | sed 's/best[0-9]*/pbleu/')
  if [ ! -e "$pbleuf" ] || [ `wc -l < $pbleuf` -ne `wc -l < $dir/allweights` ]; then
    # need to regenerate the projection
    $projectbleu $refs $allwparam < $f \
    | paste $dir/allweights - \
    | cut -f1,3 \
    > $pbleuf \
    || die "Failed to construct $pbleuf"
  fi
done

# summarize bleu projections
echo "goal	proj/real	from	was" > $dir/graph.data
for f in $dir/bleu.*; do
  obs=$(echo $f | sed 's/^.*bleu\.//')
  cat $dir/pbleu.$obs \
  | pickre --re='F: ([0-9.]*)' \
  | recut 2,1 \
  | prefix --tab -- "$obs\tproj" \
  >> $dir/graph.data
  lcat $dir/bleu.$obs \
  | pickre --re='bleu\.([-0-9.]*)' \
  | prefix --tab -- "$obs\treal" \
  | recut 1,2,3,5 \
  >> $dir/graph.data
done


exit 0

## COMMANDS TO PLOT IT:
# plot 'walkable' graph of projections at various points
g=weight-scan-tm_2/graph.data; cat $g | skip 1 | grep real | cut -f2- | numsort 2 | sed 's/real/all/' > cliprealall; skip 1 < $g | numsort 1,3 | split_at_colchange 1 | blockwise "(prefix --tab x cliprealall; cat -) | labelledxychart --data=3,4,0,'',linespoints --blockpivot=2" > clip

# plot a combination of projections along with the individual projections and
# the real scores
cat best100.-0.100000 best100.-0.500000 best100.-0.300000 best100.-0.200000 | /home/obo/tools/src/obotools/projectbleu ../tune.ref $allwparam | paste allweights - > comb.-0.5_-0.3_-0.2_-0.1
(lcat pbleu.-0.100000 pbleu.-0.500000 pbleu.-0.300000 pbleu.-0.200000 comb.-0.5_-0.3_-0.2_-0.1 | pickre --re='F: ([0-9.]*)' | recut 2,3,1 ; cat graph.data | skip 1 | grep real | cut -f2- | numsort 2 ) | tee delme | labelledxychart --blockpivot=1 --data=2,3,0,'',linespoints | gpsandbox
