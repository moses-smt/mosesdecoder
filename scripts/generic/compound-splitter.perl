#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;
use Getopt::Long "GetOptions";

my ($CORPUS,$MODEL,$TRAIN,$HELP,$VERBOSE);
my $FILLER = ":s:es";
my $MIN_SIZE = 3;
my $MIN_COUNT = 5;
my $MAX_COUNT = 5;
my $FACTORED = 0;
my $SYNTAX = 0;
my $MARK_SPLIT = 0;
my $BINARIZE = 0;
$HELP = 1
    unless &GetOptions('corpus=s' => \$CORPUS,
		       'model=s' => \$MODEL,
		       'filler=s' => \$FILLER,
		       'factored' => \$FACTORED,
		       'min-size=i' => \$MIN_SIZE,
		       'min-count=i' => \$MIN_COUNT,
		       'max-count=i' => \$MAX_COUNT,
		       'help' => \$HELP,
		       'verbose' => \$VERBOSE,
		       'syntax' => \$SYNTAX,
		       'binarize' => \$BINARIZE,
		       'mark-split' => \$MARK_SPLIT,
		       'train' => \$TRAIN);

if ($HELP ||
    ( $TRAIN && !$CORPUS) ||
    (!$TRAIN && !$MODEL)) {
    print "Compound splitter\n";
    print "-----------------\n\n";
    print "train:   compound-splitter -train -corpus txt-file -model new-model\n";
    print "apply:   compound-splitter -model trained-model < in > out\n";
    print "options: -min-size: minimum word size (default $MIN_SIZE)\n";
    print "         -min-count: minimum word count (default $MIN_COUNT)\n";
    print "         -filler: filler letters between words (default $FILLER)\n";
    print "         -factor: factored data, assuming factor 0 as surface (default $FACTORED)\n";
    print "         -syntax: syntactically parsed data (default $SYNTAX)\n";
    print "         -mark-split: mark non-terminal label of split words (default $MARK_SPLIT)\n";
    print "         -binarize: binarize subtree for split word (default $BINARIZE)\n";
    exit;
}

if ($TRAIN) {
    if ($SYNTAX)      { &train_syntax(); }
    elsif ($FACTORED) { &train_factored(); }
    else              { &train(); }
}
else {
    &apply();
}

sub train {
    my %COUNT;
    open(CORPUS,$CORPUS) || die("ERROR: could not open corpus '$CORPUS'");
    while(<CORPUS>) {
	chop; s/\s+/ /g; s/^ //; s/ $//;
	foreach (split) {
	    $COUNT{$_}++;
	}
    }
    close(CORPUS);
    &save_trained_model(\%COUNT);
}

sub save_trained_model {
    my ($COUNT) = @_;
    my $id = 0;
    open(MODEL,">".$MODEL);
    foreach my $word (keys %$COUNT) {
	print MODEL "".(++$id)."\t".$word."\t".$$COUNT{$word}."\n";
    }
    close(MODEL);
    print STDERR "written model file with ".(scalar keys %$COUNT)." words.\n";
}

sub train_factored {
  my (%COUNT,%FACTORED_COUNT);
  # collect counts for interpretations for each surface word
  open(CORPUS,$CORPUS) || die("ERROR: could not open corpus '$CORPUS'");
  while(<CORPUS>) {
    chop; s/\s+/ /g; s/^ //; s/ $//;
    foreach my $factored_word (split) {
      my $word = $factored_word;
      $word =~ s/\|.+//g; # just first factor
      $FACTORED_COUNT{$word}{$factored_word}++;
	  }
  }
  close(CORPUS);
  # only preserve most frequent interpretation, assign sum of counts
  foreach my $word (keys %FACTORED_COUNT) {
    my ($max,$best,$total) = (0,"",0);
    foreach my $factored_word (keys %{$FACTORED_COUNT{$word}}) {
      my $count = $FACTORED_COUNT{$word}{$factored_word};
      $total += $count;
      if ($count > $max) {
        $max = $count;
        $best = $factored_word;
      }
    }
    $COUNT{$best} = $total;
  }
  &save_trained_model(\%COUNT);
}

sub train_syntax {
  my (%COUNT,%LABELED_COUNT);
  # collect counts for interpretations for each surface word
  open(CORPUS,$CORPUS) || die("ERROR: could not open corpus '$CORPUS'");
  while(<CORPUS>) {
    chop; s/\s+/ /g; s/^ //; s/ $//;
    my $label;
    foreach (split) {
      if (/^label="([^\"]+)"/) {
        $label = $1;
      }
      elsif (! /^</) {
        $LABELED_COUNT{$_}{$label}++;
      }
	  }
  }
  close(CORPUS);

  # only preserve most frequent label, assign sum of counts
  foreach my $word (keys %LABELED_COUNT) {
    my ($max,$best,$total) = (0,"",0);
    foreach my $label (keys %{$LABELED_COUNT{$word}}) {
      my $count = $LABELED_COUNT{$word}{$label};
      $total += $count;
      if ($count > $max) {
        $max = $count;
        $best = "$word $label";
      }
    }
    $COUNT{$best} = $total;
  }
  &save_trained_model(\%COUNT);
}

sub apply {
    my (%COUNT,%TRUECASE,%LABEL);
    open(MODEL,$MODEL) || die("ERROR: could not open model '$MODEL'");
    while(<MODEL>) {
	chomp;
	my ($id,$factored_word,$count) = split(/\t/);
        my $label;
        ($factored_word,$label) = split(/ /,$factored_word);
        my $word = $factored_word;
        $word =~ s/\|.+//g; # just first factor
        my $lc = lc($word);
	# if word exists with multipe casings, only record most frequent
        next if defined($COUNT{$lc}) && $COUNT{$lc} > $count;
	$COUNT{$lc} = $count;
	$TRUECASE{$lc} = $factored_word;
	$LABEL{$lc} = $label if $SYNTAX;
    }
    close(MODEL);

    while(<STDIN>) {
	my $first = 1;
	chop; s/\s+/ /g; s/^ //; s/ $//;
	my @BUFFER; # for xml tags
	foreach my $factored_word (split) {
	    print " " unless $first;
	    $first = 0;

	    # syntax: don't split xml
	    if ($SYNTAX && ($factored_word =~ /^</ || $factored_word =~ />$/)) {
		push @BUFFER,$factored_word;
		$first = 1;
		next;
	    }

	    # get case class
	    my $word = $factored_word;
	    $word =~ s/\|.+//g; # just first factor
	    my $lc = lc($word);

	    print STDERR "considering $word ($lc)...\n" if $VERBOSE;
	    # don't split frequent words
	    if ((defined($COUNT{$lc}) && $COUNT{$lc}>=$MAX_COUNT) ||
	        $lc !~ /[a-zA-Z]/) {; # has to have at least one letter
		print join(" ",@BUFFER)." " if scalar(@BUFFER); @BUFFER = (); # clear buffer
		print $factored_word;
		print STDERR "\tfrequent word ($COUNT{$lc}>=$MAX_COUNT), skipping\n" if $VERBOSE;
		next;
	    }

	    # consider possible splits
	    my $final = length($word)-1;
	    my %REACHABLE;
	    for(my $i=0;$i<=$final;$i++) { $REACHABLE{$i} = (); }

	    print STDERR "splitting $word:\n" if $VERBOSE;
	    for(my $end=$MIN_SIZE;$end<length($word);$end++) {
		for(my $start=0;$start<=$end-$MIN_SIZE;$start++) {
		    next unless $start == 0 || defined($REACHABLE{$start-1});
		    foreach my $filler (split(/:/,$FILLER)) {
			next if $start == 0 && $filler ne "";
			next if lc(substr($word,$start,length($filler))) ne $filler;
			my $subword = lc(substr($word,
					        $start+length($filler),
					        $end-$start+1-length($filler)));
			next unless defined($COUNT{$subword});
			next unless $COUNT{$subword} >= $MIN_COUNT;
			print STDERR "\tmatching word $start .. $end ($filler)$subword $COUNT{$subword}\n" if $VERBOSE;
			push @{$REACHABLE{$end}},"$start $TRUECASE{$subword} $COUNT{$subword}";
		    }
		}
	    }

	    # no matches at all?
	    if (!defined($REACHABLE{$final})) {
    print join(" ",@BUFFER)." " if scalar(@BUFFER); @BUFFER = (); # clear buffer
		print $factored_word;
		next;
	    }

	    my ($best_split,$best_score) = ("",0);

	    my %ITERATOR;
	    for(my $i=0;$i<=$final;$i++) { $ITERATOR{$i}=0; }
	    my $done = 0;
	    while(1) {
		# read off word
		my ($pos,$decomp,$score,$num,@INDEX) = ($final,"",1,0);
		while($pos>0) {
		    last unless scalar @{$REACHABLE{$pos}} > $ITERATOR{$pos}; # dead end?
		    my ($nextpos,$subword,$count)
			= split(/ /,$REACHABLE{$pos}[ $ITERATOR{$pos} ]);
		    $decomp = $subword." ".$decomp;
		    $score *= $count;
		    $num++;
		    push @INDEX,$pos;
#		    print STDERR "($nextpos-$pos,$decomp,$score,$num)\n";
		    $pos = $nextpos-1;
		}

		chop($decomp);
		print STDERR "\tsplit: $decomp ($score ** 1/$num) = ".($score ** (1/$num))."\n" if $VERBOSE;
		$score **= 1/$num;
		if ($score>$best_score) {
		    $best_score = $score;
		    $best_split = $decomp;
		}

		# increase iterator
		my $increase = -1;
		while($increase<$final) {
		    $increase = pop @INDEX;
		    $ITERATOR{$increase}++;
		    last if scalar @{$REACHABLE{$increase}} > $ITERATOR{$increase};
		}
		last unless scalar @{$REACHABLE{$final}} > $ITERATOR{$final};
		for(my $i=0;$i<$increase;$i++) { $ITERATOR{$i}=0; }
	    }
      if ($best_split !~ / /) {
        print join(" ",@BUFFER)." " if scalar(@BUFFER); @BUFFER = (); # clear buffer
        print $factored_word; # do not change case for unsplit words
        next;
      }
      if (!$SYNTAX) {
        print $best_split;
      }
      else {
        $BUFFER[$#BUFFER] =~ s/label=\"/label=\"SPLIT-/ if $MARK_SPLIT;
        $BUFFER[$#BUFFER] =~ /label=\"([^\"]+)\"/ || die("ERROR: $BUFFER[$#BUFFER]\n");
        my $pos = $1;
        print join(" ",@BUFFER)." " if scalar(@BUFFER); @BUFFER = (); # clear buffer

        my @SPLIT = split(/ /,$best_split);
        my @OUT = ();
        if ($BINARIZE) {
          for(my $w=0;$w<scalar(@SPLIT)-2;$w++) {
            push @OUT,"<tree label=\"\@$pos\">";
          }
        }
        for(my $w=0;$w<scalar(@SPLIT);$w++) {
          if ($BINARIZE && $w>=2) { push @OUT, "</tree>"; }
          push @OUT,"<tree label=\"".$LABEL{lc($SPLIT[$w])}."\"> $SPLIT[$w] </tree>";
        }
        print join(" ",@OUT);
      }
	}
  print " ".join(" ",@BUFFER) if scalar(@BUFFER); @BUFFER = (); # clear buffer
	print "\n";
    }
}
