#!/usr/bin/perl -w

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

die("ERROR: in-domain source file '$indomain_source' not found") unless -e $indomain_source;
die("ERROR: in-domain target file '$indomain_target' not found") unless -e $indomain_target;
die("ERROR: out-of-domain source file '$outdomain_source' not found") unless -e $outdomain_source;
die("ERROR: out-of-domain target file '$outdomain_target' not found") unless -e $outdomain_target;

die("ERROR: language model order not specified (-order NUM)") unless defined($order);
die("ERROR: language model settings not specified (-lm-settings STRING)") unless defined($lm_settings);
die("ERROR: language model command not specified (-lm-training CMD)") unless defined($lm_training);
die("ERROR: language model binarizer not specified (-lm-binarizer CMD)") unless defined($lm_binarizer);
die("ERROR: model not specified (-model FILESTEM)") unless defined($model);

&train_lm($indomain_source,"in-source");
&train_lm($indomain_target,"in-target");
&train_lm($outdomain_source,"out-source");
&train_lm($outdomain_target,"out-target");

sub train_lm {
  my ($file,$type) = @_;
  print STDERR "training $type language model\n";
  if (defined($line_count)) {
    my $cmd = "cat $file | shuf -n $line_count --random-source $file > $model.$type.tok";
    print STDERR "extracting $line_count random lines from $file\n$cmd\n";
    print STDERR `$cmd`;
    $file = "$model.$type.tok";
  }

  my $cmd = "$lm_training -order $order $lm_settings -text $model.$type.tok -lm $model.$type.lm";
  print STDERR $cmd."\n";
  print STDERR `$cmd`;

  $cmd = "$lm_binarizer $model.$type.lm $model.$type.binlm";
  print STDERR $cmd."\n";
  print STDERR `$cmd`;
}

