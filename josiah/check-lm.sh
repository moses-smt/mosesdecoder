#!/bin/sh

#
# Process a log file created by running josiah with the -p option
# and extract josiah+srilm lm scores.
#

if [ $# -ne 2 ]; then
    echo "Usage: `basename $0` <input-file> <out-stem>"
    exit 1
fi

input=$1
out=$2

ngram=/home/bhaddow/statmt/srilm/bin/i686/ngram
lm=/afs/inf.ed.ac.uk/group/bhaddow/models/de-en-tuning-mert/europarl.lm.2
#ngram=/Users/abhishekarun/PhD/srilm/bin/macosx/ngram
#lm=/Users/abhishekarun/PhD/mt-marathon-2009/models/europarl.lm.gz

if [ `basename $input .gz` == `basename $input` ]; then
    catcmd=cat
else
    catcmd=zcat
fi



# Extract sentences and gibbler lm scores
sentence_out=$out.sentences
josiah_out=$out.probs.josiah
echo "Writing sentences to $sentence_out"
echo "Writing josiah probabilities to  $josiah_out"
$catcmd $1 | perl -e "
open SOUT, \">$sentence_out\" || die \"Could not open sentence file\";
open JOUT, \">$josiah_out\" || die \"Could not open josiah probs file\";
\$lmtotal=0;
while(<>) {
    chomp;
    if (/^MaxDecode.*Target: <<(.*?)>> Feature values: <<(.*?)>>/) {
        \$sentence = \$1;
        \$scores = \$2;
        \$sentence =~ s/\[\d+..\d+\]//g;
        \$unknowns = (\$sentence =~ s/\|UNK\|UNK\|UNK//g);
        @fields = split /, /, \$scores;
        \$lmscore = \$fields[3];
        \$lmscore += 100*\$unknowns;
        \$lmtotal += \$lmscore;
        print SOUT \"\$sentence\n\"; 
        print JOUT \"\$lmscore\n\";
    }
}
print JOUT \"\$lmtotal\n\";
close SOUT;
close JOUT;
"

# Score with srilm
srilm_out=$out.probs.srilm
echo "Writing srilm probabilities to  $srilm_out"
$ngram -lm $lm -ppl $sentence_out -debug 1 | perl -e "
    while (<>) {
        if (/logprob/) {
            @fields = split;
            \$score = \$fields[3]*log(10);
            print \"\$score\n\";
        }
    }" > $srilm_out

echo "TOTAL" >> $sentence_out
nl $josiah_out > $josiah_out.nl
nl $srilm_out > $srilm_out.nl
nl $sentence_out > $sentence_out.nl
echo "Writing combined probabilities to $out.probs"
join $josiah_out.nl $srilm_out.nl | join -  $sentence_out.nl > $out.probs
