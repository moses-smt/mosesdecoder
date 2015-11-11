#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

# $Id$
use warnings;
use strict;
use Getopt::Long "GetOptions";

my ($SRC,$INFILE,$RECASE_MODEL,$UNBUFFERED);
my $MOSES = "moses";
my $LANGUAGE = "en"; # English by default;
die("recase.perl --in file --model ini-file > out")
    unless &GetOptions('in=s' => \$INFILE,
                       'headline=s' => \$SRC,
                       'lang=s' => \$LANGUAGE,
		       'moses=s' => \$MOSES,
                       'model=s' => \$RECASE_MODEL,
                       'b|unbuffered' => \$UNBUFFERED)
    && defined($INFILE)
    && defined($RECASE_MODEL);
if (defined($UNBUFFERED) && $UNBUFFERED) { $|=1; }

my %treated_languages = map { ($_,1) } qw/en cs/;
die "I don't know any rules for $LANGUAGE. Use 'en' as the default."
  if ! defined $treated_languages{$LANGUAGE};

# lowercase even in headline
my %ALWAYS_LOWER;
if ($LANGUAGE eq "en" ) {
  foreach ("a","after","against","al-.+","and","any","as","at","be","because","between","by","during","el-.+","for","from","his","in","is","its","last","not","of","off","on","than","the","their","this","to","was","were","which","will","with") { $ALWAYS_LOWER{$_} = 1; }
}

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

binmode(STDOUT, ":utf8");

my $sentence = 0;
my $infile = $INFILE;
$infile =~ s/[\.\/]/_/g;
open(MODEL,"$MOSES -v 0 -f $RECASE_MODEL -i $INFILE -dl 0|");
binmode(MODEL, ":utf8");
while(<MODEL>) {
    chomp;
    s/\s+$//;
    my @WORD  = split(/ /);

    # uppercase initial word
    &uppercase(\$WORD[0]);

    if ($LANGUAGE ne "cs") {
      # uppercase after period
      # unless in Czech where '.' is used after all ordinals
      for(my $i=1;$i<scalar(@WORD);$i++) {
	  if ($WORD[$i-1] eq '.') {
	      &uppercase(\$WORD[$i]);
	  }
      }
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
close(MODEL);

`rm -rf /tmp/filter.$infile`;

sub uppercase {
    my ($W) = @_;
    $$W = ucfirst($$W); # rely on Perl's Unicode knowledge, never use tr//
}
