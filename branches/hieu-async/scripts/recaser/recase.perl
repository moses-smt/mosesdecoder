#!/usr/bin/perl -w
 
use strict;
use Getopt::Long "GetOptions";

my ($SRC,$INFILE,$RECASE_MODEL);
my $MOSES = "moses";
die("recase.perl --in file --model ini-file > out")
    unless &GetOptions('in=s' => \$INFILE,
                       'headline=s' => \$SRC,
		       'moses=s' => \$MOSES,
                       'model=s' => \$RECASE_MODEL)
    && defined($INFILE)
    && defined($RECASE_MODEL);

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
my $infile = $INFILE;
$infile =~ s/[\.\/]/_/g;
open(MODEL,"$MOSES -f $RECASE_MODEL -i $INFILE -dl 1|");
while(<MODEL>) {
    chomp;
    s/\s+$//;
    my @WORD  = split(/ /);

    # uppercase initial word
    &uppercase(\$WORD[0]);

    # uppercase after period
    for(my $i=1;$i<scalar(@WORD);$i++) {
	if ($WORD[$i-1] eq '.') {
	    &uppercase(\$WORD[$i]);
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
    substr($$W,0,1) =~ tr/a-z/A-Z/;
    substr($$W,0,1) =~ tr/à-þ/À-Þ/;
}
