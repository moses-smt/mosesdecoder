#!/usr/bin/perl -w

use strict;
use Getopt::Long "GetOptions";
use FindBin qw($Bin);

my $BITPAR = "/home/pkoehn/statmt/project/bitpar/GermanParser";
my $TMPDIR = "tmp";

my $DEBUG = 0;
my $BASIC = 0;
GetOptions(
    "basic" => \$BASIC,
    "bitpar=s" => \$BITPAR
    ) or die("ERROR: unknown options");

`mkdir -p $TMPDIR`;
my $tmpfile = "$TMPDIR/parse.tmp.$$";

open(TMP,"| iconv -c -f utf8 -t iso-8859-1 >$tmpfile");
while(<STDIN>) 
{
    foreach (split) 
    { 
	s/\(/\*LRB\*/g;
	s/\)/\*RRB\*/g;
	print TMP $_."\n"; 
    } 
    print TMP "\n";
}
close(STDIN);
close(TMP);

open(PARSER,"cat $tmpfile | $BITPAR/bin/bitpar -ts '()' -s TOP -v $BITPAR/Tiger/grammar $BITPAR/Tiger/lexicon -u $BITPAR/Tiger/open-class-tags -w $BITPAR/Tiger/wordclass.txt | iconv -c -t utf8 -f iso-8859-1 |");
while(my $line = <PARSER>) {
    if ($line =~ /^No parse for/) {
        print "\n";
        next;
    }
    print STDERR $line if $DEBUG;
    chop($line);
    my @LABEL = ();
    my @OUT = ();
    for(my $i=0;$i<length($line);$i++) {
        # print STDERR substr($line,$i)."\n";
	if (substr($line,$i,4) eq "(*T*") {
	   my ($trace,$rest) = split(/\)/,substr($line,$i+1)); 
	   $i+=length($trace)+2;
	   $i++ if substr($line,$i+1,1) eq " ";
	   die("ERROR: NO LABEL FOR TRACE") unless @LABEL;
	   if (!$DEBUG) { pop @LABEL; pop @OUT; }
	   print STDERR substr("                                                                               ",0,scalar @LABEL-1)."REMOVING LABEL ".(pop @LABEL)." ".(pop @OUT)."\n" if $DEBUG;
	}
        elsif (substr($line,$i,1) eq "(") {
            my ($label,$rest) = split(/[\( ]/,substr($line,$i+1));
            print STDERR substr("                                                                               ",0,scalar @LABEL)."BEGINNING of $label\n" if $DEBUG;
            $i+=length($label);
	    $label =~ s/\$/PUNC/g;       # no $!
	    $label =~ s/\|/:/g;          # moses does not like bars
	    $label =~ s/^[^A-Z]*([A-Z]+).*/$1/g if $BASIC; # basic labels only
            push @OUT,"<tree label=\"$label\">";
	    push @LABEL,$label;
            $i++ if substr($line,$i+1,1) eq " ";
	    $i++ if substr($line,$i+1,1) eq " ";
        }
        elsif (substr($line,$i,1) eq ")") {
            die("ERROR: NO LABEL ON STACK") unless @LABEL;
            my $label = pop @LABEL;
            print STDERR substr("                                                                               ",0,scalar @LABEL)."END of $label\n" if $DEBUG;
            push @OUT,"</tree>";
	    $i++ if substr($line,$i+1,1) eq " ";
        }
        elsif (substr($line,$i,3) eq "*T*") {
            die("ERROR: NO LABEL FOR TRACE") unless @LABEL;
            pop @LABEL;
            pop @OUT;
	    #print "POPPING TRACE LABEL ", pop @OUT;
            my ($trace,$rest) = split(/\)/,substr($line,$i+1));
            $i+=length($trace)+1;
        }
        else {
            my ($word,$rest) = split(/\)/,substr($line,$i));
            if (substr($line,$i,2) eq "\\)") {
                $word = substr($line,$i,2);
            }
            $i+=length($word)-1;
            print STDERR substr("                                                                               ",0,scalar @LABEL)."WORD $word\n" if $DEBUG;
            push @OUT,"$word";
        }
    }
    die("ERROR: STACK NOT EMPTY $#LABEL\n") if @LABEL;
    my $first=1;
    foreach (@OUT) {
        print " " unless $first;
        s/\\//;
	s/\*RRB\*/\)/g;
	s/\*LRB\*/\(/g;
        print $_;
        $first = 0;
    }
    print "\n";
}

`rm $tmpfile`;

