#!/usr/bin/perl -w
use strict;

#TODO : rewrite so that it does not rely on alignments...
# Remember position of source non-terminal and get right alignment

my @e;
my @f;
my @a;
my @s;
my @r;

my @targetNonTermAlignAll;
my @targetNonTermAlign;

my @sourceNonTermAlignAll;
my @sourceNonTermAlign;


sub FindSourceNonTerminals {
  my @f = @{ $_[0] };
  my $index = 0;
  my @sourceNonTermPos = ();

 	for (my $i=0; $i<=$#f; $i++) {
		if($f[$i] =~ /(\[X\]\[[^\s\]]+\])/) { # NIN: rewrite expression so it matches ".$" and other stuff
      push(@sourceNonTermPos,$i);
		}
	}
	return @sourceNonTermPos;
}


# Take the alignment point of target nt and replace the corresponding nt-pair in e with the index given by alignment-array
sub ReplaceValueWithIndex() {
	my @e = @{ $_[0] };
	my @targetNonTermAlignment = @{ $_[1] };

  for (my $i=0; $i<=$#targetNonTermAlignment; $i++) {
    my $offset = $targetNonTermAlignment[$i];
    my $nt = $e[$targetNonTermAlignment[$i]];
    my $newnt = $nt . $i;
    splice(@e,$offset,1,$newnt);
  }
	return (\@e);
}


### MAIN
while(<STDIN>){
	@e = ();
	@a = ();
	@f = ();
	@s = ();
	@r = ();

  # all target sides of alignments
	my @sourceAlignAll = ();
	my @targetAlignAll = ();

  my $stage = 0;
  chomp;
  #print "New line : $_ \n";

  # This does not work for hiero because we can have empty alignments	 
  # my ($f,$e,$s,$a) = split(/ \|\|\| /);
  my @token = split('\s', $_);

	foreach (@token) {
		#print "Obtained Token : $_ \n";

		if ($_ eq "|||") {
			++$stage;
		}
		if($stage == 0) {
      if($_ ne "|||") {
        #print "FRENCH PART : $_ \n";
        push(@f,$_);
      }
		}
		if($stage == 1) {
      if($_ ne "|||") {
        push(@e,$_);
      }
		}
		if($stage == 3) {
      if($_ ne "|||") {
        push(@a,$_);
        # split alignment and put into separate arrays
        my @align = split("-",$_);
        push(@sourceAlignAll, $align[0]);
        push(@targetAlignAll, $align[1]);
      }
		}
		if($stage == 2) {
      if($_ ne "|||") {
        push(@s,$_);
      }
		}
		if($stage == 4) {
      if($_ ne "|||") {
        push(@r,$_);
      }
		}
	}

  # count non-terminals in french (source) side of rule
  my @sourceNonTerms = &FindSourceNonTerminals(\@f);

  #if there are no nonterminals just take as is
  if(scalar(@sourceNonTerms) == 0) {
    my $f = join(" ",@f);
   	my $e = join(" ",@e);
    my $a = join(" ",@a);
   	my $s = join(" ",@s);
    my $r = join(" ",@r);

    print "$f ||| $e ||| $s ||| $a ||| $r \n"
  }
  else {
    my @targetNonTerms = ();

    #TODO : this should be implemented in a more efficient way
    #Find the index of source non-terminals to find target non-terminals
    foreach (@sourceNonTerms) {
      my $nonTermToFind = $_;
      for ( my $i=0; $i<=$#sourceAlignAll; $i++ ) {
        #find index of source non-terminal
        #get target at this index
        if ($sourceAlignAll[$i] eq $nonTermToFind) {
          my $targetToInsert = $targetAlignAll[$i];
          push(@targetNonTerms,$targetToInsert);
        }
      }
    }

   	my ($eref) = &ReplaceValueWithIndex(\@e, \@targetNonTerms);
   	my @mode = @$eref;

    my $f = join(" ",@f);
    my $e = join(" ",@mode);
    my $a = join(" ",@a);
    my $s = join(" ",@s);
    my $r = join(" ",@r);

    print "$f ||| $e ||| $s ||| $a ||| $r \n"
  }

  #For hiero : add alignments to target phrases
  #$frPhrases{$f}++;
  #print "Align augmented line \n"; 
  #print $e;
  #print "\n";
  #$enPhrases{$e}++;
}
