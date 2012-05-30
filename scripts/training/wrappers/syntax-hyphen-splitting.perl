#!/usr/bin/perl -w

use strict;
use Getopt::Long "GetOptions";

my $MARK_HYP = 0;
my $BINARIZE = 0;

die unless &GetOptions('binarize' => \$BINARIZE,'mark-split' => \$MARK_HYP);

while(<STDIN>) {
  chop;
  my @OUT = ();
  foreach (split) {
    if (/^</ || />$/) {
      push @OUT, $_;
    }
    elsif(/([\p{IsAlnum}])\-([\p{IsAlnum}])/) {
      s/([\p{IsAlnum}])\-([\p{IsAlnum}])/$1 \@-\@ $2/g;
      my @WORD = split;
      $OUT[$#OUT] =~ /label=\"([^\"]+)\"/;
      my $pos = $1;
      if ($MARK_HYP) {
        $OUT[$#OUT] =~ s/label=\"/label=\"HYP-/;
      }
      if ($BINARIZE) {
        for(my $i=0;$i<scalar(@WORD)-2;$i++) {
          push @OUT,"<tree label=\"\@".($MARK_HYP ? "HYP-" : "")."$pos\">";
        }
      }
      for(my $i=0;$i<scalar(@WORD);$i++) {
        if ($BINARIZE && $i>=2) {
          push @OUT, "</tree>";
        }
        push @OUT,"<tree label=\"".(($WORD[$i] eq "\@-\@") ? "HYP" : $pos)."\"> $WORD[$i] </tree>";
      }
    }
    else {
      push @OUT, $_;
    }
  }
  print join(" ",@OUT)."\n";
}
