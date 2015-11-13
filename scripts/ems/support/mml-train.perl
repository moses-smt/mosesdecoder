#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;

my ($indomain_source,,$indomain_target,$outdomain_source,$outdomain_target,$lm_training,$lm_binarizer,$order,$lm_settings,$line_count,$model);

use Getopt::Long;
GetOptions('in-source=s' => \$indomain_source,
           'in-target=s' => \$indomain_target,
           'out-source=s' => \$outdomain_source,
           'out-target=s' => \$outdomain_target,
           'model=s' => \$model,
           'lm-training=s' => \$lm_training,
           'lm-binarizer=s' => \$lm_binarizer,
           'order=s' => \$order,
           'lm-settings=s' => \$lm_settings,
           'line-count=i' => \$line_count
    ) or exit(1);

die("ERROR: in-domain source file not specified (-in-source FILE)") unless defined($indomain_source);
die("ERROR: in-domain target file not specified (-in-target FILE)") unless defined($indomain_target);
die("ERROR: out-of-domain source file not specified (-out-source FILE)") unless defined($outdomain_source);
die("ERROR: out-of-domain target file not specified (-out-target FILE)") unless defined($outdomain_target);

die("ERROR: in-domain source file '$indomain_source' not found") unless -e $indomain_source || -e $indomain_source.".gz";
die("ERROR: in-domain target file '$indomain_target' not found") unless -e $indomain_target || -e $indomain_target.".gz";
die("ERROR: out-of-domain source file '$outdomain_source' not found") unless -e $outdomain_source || -e $outdomain_source.".gz";
die("ERROR: out-of-domain target file '$outdomain_target' not found") unless -e $outdomain_target || -e $outdomain_target.".gz";

die("ERROR: language model order not specified (-order NUM)") unless defined($order);
die("ERROR: language model settings not specified (-lm-settings STRING)") unless defined($lm_settings);
die("ERROR: language model command not specified (-lm-training CMD)") unless defined($lm_training);
die("ERROR: language model binarizer not specified (-lm-binarizer CMD)") unless defined($lm_binarizer);
die("ERROR: model not specified (-model FILESTEM)") unless defined($model);

&train_lm($indomain_source,"in-source");
&train_lm($indomain_target,"in-target");
&extract_vocabulary("in-source");
&extract_vocabulary("in-target");
&train_lm($outdomain_source,"out-source","in-source");
&train_lm($outdomain_target,"out-target","in-target");

sub extract_vocabulary {
  my ($type) = @_;
  print STDERR "extracting vocabulary from $type language model\n";
  open(LM,"$model.$type.lm");
  open(VOCAB,">$model.$type.vocab");
  my $unigrams = 0;
  while(<LM>) {
    $unigrams = 1 if /^\\1-grams:/;
    last if /^\\2-grams:/;
    next unless $unigrams;
    my @TOKEN = split(/\s/);
    next unless @TOKEN == 3;
    next if $TOKEN[1] eq '<s>';
    next if $TOKEN[1] eq '<unk>';
    next if $TOKEN[1] eq '<\\s>';
    print VOCAB $TOKEN[1]."\n";
  }
  close(LM);
  close(VOCAB);
}

sub train_lm {
  my ($file,$type,$vocab) = @_;
  print STDERR "training $type language model\n";
  if (defined($line_count)) {
    my $cmd = (-e $file.".gz" ? "zcat $file.gz" : "cat $file");
    $cmd .= " | shuf -n $line_count --random-source ".(-e $file.".gz" ? "$file.gz" : $file)." > $model.$type.tok";
    print STDERR "extracting $line_count random lines from $file\n$cmd\n";
    print STDERR `$cmd`;
    $file = "$model.$type.tok";
  }

  my $cmd = "$lm_training -order $order $lm_settings -text $file -lm $model.$type.lm";
  $cmd .= " -vocab $model.$vocab.vocab" if defined($vocab);
  print STDERR $cmd."\n";
  print STDERR `$cmd`;

  $cmd = "$lm_binarizer $model.$type.lm $model.$type.binlm";
  print STDERR $cmd."\n";
  print STDERR `$cmd`;
}

