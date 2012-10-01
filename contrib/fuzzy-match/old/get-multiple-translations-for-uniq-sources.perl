#!/usr/bin/perl -w

use strict;

my $src_in = "corpus/acquis.truecased.4.en";
my $tgt_in = "corpus/acquis.truecased.4.fr";
my $align_in = "model/aligned.4.grow-diag-final-and";

my $src_out = "data/acquis.truecased.4.en.uniq";
my $tgt_out = "data/acquis.truecased.4.fr.uniq";
my $tgt_mf  = "data/acquis.truecased.4.fr.uniq.most-frequent";
my $align_out = "data/acquis.truecased.4.align.uniq";
my $align_mf  = "data/acquis.truecased.4.align.uniq.most-frequent";

my (%TRANS,%ALIGN);

open(SRC,$src_in);
open(TGT,$tgt_in);
open(ALIGN,$align_in);
while(my $src = <SRC>) {
  my $tgt = <TGT>;
  my $align = <ALIGN>;
  chop($tgt);
  chop($align);
  $TRANS{$src}{$tgt}++;
  $ALIGN{$src}{$tgt} = $align;
}
close(SRC);
close(TGT);

open(SRC_OUT,">$src_out");
open(TGT_OUT,">$tgt_out");
open(TGT_MF, ">$tgt_mf");
open(ALIGN_OUT,">$align_out");
open(ALIGN_MF, ">$align_mf");
foreach my $src (keys %TRANS) {
  print SRC_OUT $src;
  my $first = 1;
  my ($max,$best) = (0);
  foreach my $tgt (keys %{$TRANS{$src}}) {
    print TGT_OUT " ||| " unless $first;
    print TGT_OUT $TRANS{$src}{$tgt}." ".$tgt;
    print ALIGN_OUT " ||| " unless $first;
    print ALIGN_OUT $ALIGN{$src}{$tgt};
    if ($TRANS{$src}{$tgt} > $max) {
      $max = $TRANS{$src}{$tgt};
      $best = $tgt;
    }
    $first = 0;
  }
  print TGT_OUT "\n";
  print ALIGN_OUT "\n";
  print TGT_MF $best."\n";
  print ALIGN_MF $ALIGN{$src}{$best}."\n";
}
close(SRC_OUT);
close(TGT_OUT);

