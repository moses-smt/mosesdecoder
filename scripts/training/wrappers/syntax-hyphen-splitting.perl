#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;
use Getopt::Long "GetOptions";

my $MARK_HYP = 0;
my $BINARIZE = 0;
my $SLASH = 0;

die unless &GetOptions('binarize' => \$BINARIZE,'mark-split' => \$MARK_HYP,'slash' => \$SLASH);

my $punc = $SLASH ? "/" : "-";

while(<STDIN>) {
  chop;
  my @OUT = ();
  foreach (split) {
    if (/^</ || />$/) {
      push @OUT, $_;
    }
    elsif(/([\p{IsAlnum}])$punc([\p{IsAlnum}])/) {
      s/([\p{IsAlnum}])$punc([\p{IsAlnum}])/$1 \@$punc\@ $2/g;
      my @WORD = split;
      $OUT[$#OUT] =~ /label=\"([^\"]+)\"/;
      my $pos = $1;
      my $mark = $SLASH ? "SLASH-" : "HYP-";
      my $punc_pos = $SLASH ? "SLASH" : "HYP";
      if ($MARK_HYP) {
        $OUT[$#OUT] =~ s/label=\"/label=\"$mark/;
      }
      if ($BINARIZE) {
        for(my $i=0;$i<scalar(@WORD)-2;$i++) {
          push @OUT,"<tree label=\"\@".($MARK_HYP ? $mark : "")."$pos\">";
        }
      }
      for(my $i=0;$i<scalar(@WORD);$i++) {
        if ($BINARIZE && $i>=2) {
          push @OUT, "</tree>";
        }
        push @OUT,"<tree label=\"".(($WORD[$i] eq "\@$punc\@") ? $punc_pos : $pos)."\"> $WORD[$i] </tree>";
      }
    }
    else {
      push @OUT, $_;
    }
  }
  print join(" ",@OUT)."\n";
}
