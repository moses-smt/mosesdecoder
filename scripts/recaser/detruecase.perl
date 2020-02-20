#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;
use Getopt::Long "GetOptions";

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");

my ($SRC,$INFILE,$UNBUFFERED);
die("detruecase.perl < in > out")
    unless &GetOptions('headline=s' => \$SRC,
		       'in=s' => \$INFILE,
                       'b|unbuffered' => \$UNBUFFERED);
if (defined($UNBUFFERED) && $UNBUFFERED) { $|=1; }

my %SENTENCE_END = ("."=>1,":"=>1,"?"=>1,"!"=>1);
my %DELAYED_SENTENCE_START = ("("=>1,"["=>1,"\""=>1,"'"=>1,"&quot;"=>1,"&apos;"=>1,"&#91;"=>1,"&#93;"=>1);

# lowercase even in headline
my %ALWAYS_LOWER;
foreach ("a","after","against","al-.+","and","any","as","at","be","because","between","by","during","el-.+","for","from","his","in","is","its","last","not","of","off","on","than","the","their","this","to","was","were","which","will","with") { $ALWAYS_LOWER{$_} = 1; }

# find out about the headlines
my @HEADLINE;
if (defined($SRC)) {
    open(SRC,$SRC);
    my $headline_flag = 0;
    while(<SRC>) {
	$headline_flag = 1 if /<hl>/;
	$headline_flag = 0 if /<.hl>/;
	next unless /^<seg/;
	push @HEADLINE, $headline_flag;
    }
    close(SRC);
}

my $sentence = 0;
if ($INFILE) {
  open(IN,$INFILE) || die("ERROR: could not open file '$INFILE'");
  binmode(IN, ":utf8");
  while(<IN>) {
    &process($_,$sentence++);
  }
  close(IN);
}
else {
  while(<STDIN>) {
    &process($_,$sentence++);
  }
}

sub process {
    my $line = $_[0];
    chomp($line);
    $line =~ s/^\s+//;
    $line =~ s/\s+$//;
    my @WORD  = split(/\s+/,$line);

    # uppercase at sentence start
    my $sentence_start = 1;
    for(my $i=0;$i<scalar(@WORD);$i++) {
      &uppercase(\$WORD[$i]) if $sentence_start;
      if (defined($SENTENCE_END{ $WORD[$i] })) { $sentence_start = 1; }
      elsif (!defined($DELAYED_SENTENCE_START{$WORD[$i] })) { $sentence_start = 0; }
    }

    # uppercase headlines {
    if (defined($SRC) && $HEADLINE[$sentence]) {
	foreach (@WORD) {
	    &uppercase(\$_) unless $ALWAYS_LOWER{$_};
	}	
    }

    # output
    my $first = 1;
    foreach (@WORD) {
	print " " unless $first;
	$first = 0;
	print $_;
    }
    print "\n";
    $sentence++;
}

sub uppercase {
    my ($W) = @_;
    $$W = uc(substr($$W,0,1)).substr($$W,1);
}
