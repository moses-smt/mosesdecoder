#!/usr/bin/perl -w

use strict;
use FindBin qw($RealBin);

my ($in,$out,$score,$source_lang,$target_lang,$proportion,$domain,$alignment);

use Getopt::Long;
GetOptions('in=s' => \$in,
           'out=s' => \$out,
           'score=s' => \$score,
           'domain=s' => \$domain,
           'alignment=s' => \$alignment,
           'input-extension=s' => \$source_lang,
           'output-extension=s' => \$target_lang,
           'proportion=f' => \$proportion
    ) or exit(1);

die("ERROR: input corpus stem not specified (-in FILESTEM)") unless defined($in);
die("ERROR: output corpus stem not specified (-out FILESTEM)") unless defined($out);
die("ERROR: score file not specified (-score FILE)") unless defined($score);
die("ERROR: domain file not specified (-domain FILE)") unless defined($domain);
die("ERROR: input extension not specified (-input-extension STRING)") unless defined($source_lang);
die("ERROR: output extension not specified (-output-extension STRING)") unless defined($target_lang);
die("ERROR: proportion not specified (-proportion RATIO)") unless defined($proportion);

open(CONFIG,">$out.ini");
print CONFIG "[general]
strategy = Score
source_language = $source_lang
target_language = $target_lang
input_stem = $in
".(defined($alignment) ? "alignment_stem = $alignment\n" : "").
"output_stem = $out
domain_file = $domain
domain_file_out = $out

[score]
score_file = $score
proportion = $proportion\n";
close(CONFIG);

my $cmd = "$RealBin/mml-filter.py $out.ini";
print STDERR "$cmd\n";
print STDERR `$cmd`;

