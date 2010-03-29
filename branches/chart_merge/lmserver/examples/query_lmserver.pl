#!/usr/bin/perl -w
use strict;

use LMClient;
my $lmclient = new LMClient('localhost:11211');
my $lp1 = $lmclient->word_prob("wants","<s> the old man");
my $lp2 = $lmclient->word_prob("want","<s> the old man");
print "$lp1 $lp2\n";
if ($lp1 > $lp2) {
  print "Sentence 1 is more probable\n";
  } else {
    print "Sentence 2 is more probable\n";
    }
print "done\n";


