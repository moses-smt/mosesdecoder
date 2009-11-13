#!/usr/bin/perl -w

use strict;
use Getopt::Long "GetOptions";
my $COLLINS = "/home/pkoehn/statmt/project/collins-parser.linux";
my $MXPOST  = "/home/pkoehn/statmt/project/mxpost";
my $TMPDIR = "tmp";
my $DEBUG = 0;
my $DEBUG_SPACE = "                                                       ";

my $BASIC = 0;
GetOptions(
    "collins=s" => \$COLLINS,
    "mxpost=s" => \$MXPOST,
    "tmpdir=s" => \$TMPDIR
    ) or die("ERROR: unknown options");

`mkdir -p $TMPDIR`;
#my $tmpfile2 = "tmp/collins.8087";
my $tmpfile = "$TMPDIR/mxpost.$$";
my $tmpfile2 = "$TMPDIR/collins.$$";

open(MXPOST,"|$MXPOST/mxpost.unicode $MXPOST/tagger.project > $tmpfile");
while(<STDIN>) 
{
  print MXPOST $_;
}
close(MXPOST);

`$COLLINS/parse3.pl -maxw 200 -maxc 10000 < $tmpfile > $tmpfile2`;

open(PARSER,"$tmpfile2");
while(my $line = <PARSER>) {
    next unless $line =~ /^\(/;
    if ($line =~ /SentenceTooLong/) {
	print "\n";
	next;
    }
    chop($line);
    my @LABEL = ();
    my @OUT = ();
    for(my $i=0;$i<length($line);$i++) {
        # print STDERR substr($line,$i)."\n";
        if (substr($line,$i,1) eq "(") {
            my ($label,$rest) = split(/[\( ]/,substr($line,$i+1));
            print STDERR substr($DEBUG_SPACE,0,scalar @LABEL)."BEGINNING of $label\n" if $DEBUG;
            $i+=length($label);
	    $label =~ s/\$/PUNC/g;       # no $!
	    $label =~ s/\|/:/g;          # moses does not like bars
	    $label =~ s/\~.+//;          # no head node info
            push @OUT,"<tree label=\"$label\">";
	    push @LABEL,$label;
            $i++ if substr($line,$i+1,1) eq " ";
	    $i++ if substr($line,$i+1,1) eq " ";
        }
        elsif (substr($line,$i,1) eq ")") {
            die("ERROR: NO LABEL ON STACK") unless @LABEL;
            my $label = pop @LABEL;
            print STDERR substr($DEBUG_SPACE,0,scalar @LABEL)."END of $label\n" if $DEBUG;
            push @OUT,"</tree>";
	    $i++ if substr($line,$i+1,1) eq " ";
        }
        else {
            my ($word,$rest) = split(/ /,substr($line,$i));
            if (substr($line,$i,2) eq "\\)") {
                $word = substr($line,$i,2);
            }
            $i+=length($word);
            print STDERR substr($DEBUG_SPACE,0,scalar @LABEL)."WORD $word\n" if $DEBUG;
	    $word =~ /^(.+)\/([^\/]+)$/;
	    my ($w,$p) = ($1,$2);
	    $w = "(" if $w eq "-LRB-";
	    $w = ")" if $w eq "-RRB-";
	    $p =~ s/^-//;
	    $p =~ s/-$//;
            push @OUT,"<tree label=\"$p\"> $w </tree>";
        }
    }
    die("ERROR: STACK NOT EMPTY $#LABEL\n") if @LABEL;
    my $first=1;
    foreach (@OUT) {
        print " " unless $first;
        s/\\//;
        print $_;
        $first = 0;
    }
    print "\n";
}

#`rm $tmpfile`;

