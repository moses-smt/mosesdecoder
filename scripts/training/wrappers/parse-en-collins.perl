#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;
use File::Basename;
use File::Temp qw/tempfile/;
use Getopt::Long "GetOptions";

my $COLLINS = "/exports/home/s0565741/work/bin/COLLINS-PARSER";
my $MXPOST  = "/exports/home/s0565741/work/bin/mxpost";
my $TMPDIR = "tmp";
my $KEEP_TMP = 0;
my $RAW = undef;

my $BASIC = 0;
GetOptions(
    "collins=s" => \$COLLINS,
    "mxpost=s" => \$MXPOST,
    "tmpdir=s" => \$TMPDIR,
    "keep-tmp" => \$KEEP_TMP,
    "raw=s" => \$RAW
    ) or die("ERROR: unknown options");

`mkdir -p $TMPDIR`;

# parser settings
my $MaxChar=10000;
my $MaxWord=120;
my $ParserBin="$COLLINS/code/parser";
my $ParserEvn="$COLLINS/models/model2/events.gz";
my $ParserGrm="$COLLINS/models/model2/grammar";
my ($scriptname, $directories) = fileparse($0);
my ($TMP, $tmpfile) = tempfile("$scriptname-XXXXXXXXXX", DIR=>$TMPDIR, UNLINK=>!$KEEP_TMP);

# tag and prepare input for parser
my $pipeline = "perl -ne 'use Encode; encode(\"iso-8859-1\", decode(\"utf8\", \$_)); print \$_;' |";
$pipeline .= "perl -ne 'tr/\\x20-\\x7f//cd; print \$_.\"\\n\";' | ";
$pipeline .= "$MXPOST/mxpost $MXPOST/tagger.project |";

open(TAG,$pipeline);
my $sentence_count=0;
while(<TAG>) {
  if ($sentence_count % 2000 == 0) {
    close(PARSER_IN) if $sentence_count;
    open(PARSER_IN,sprintf(">%s.%05d",$tmpfile,$sentence_count/2000));
  }
  $sentence_count++;
  chop;

  # convert tagged sequence into parser format
  my $line = &conv_posfmt($_);

   # check char length or word length
  $line = "1 SentenceTooLong NN" if (! &check_length($line));

  # put to tmpfile
  print PARSER_IN "$line\n";
}
close(TAG);
close(PARSER_IN);

# parse
for(my $i=0;$i * 2000 < $sentence_count;$i++) {
  my $i_formatted = sprintf("%05d",$i);
  `gunzip -c $ParserEvn | $ParserBin $tmpfile.$i_formatted $ParserGrm 10000 1 1 1 1 > $tmpfile.$i_formatted.out`;
}

# process output of parser
my $DEBUG = 0;
my $DEBUG_SPACE = "                                                       ";
open(PARSER,"cat $tmpfile.?????.out|");
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
	    $w = &escape($w);
	    $p =~ s/^-//;
	    $p =~ s/-$//;
            push @OUT,"<tree label=\"$p\"> $w </tree>";
        }
    }
    die("ERROR: STACK NOT EMPTY $#LABEL\n") if @LABEL;
    my $first=1;
    foreach (@OUT) {
        print " " unless $first;
	# s/\\//; #why?
        print $_;
        $first = 0;
    }
    print "\n";
}

sub escape {
    my ($text) = @_;
    $text =~ s/&/&amp;/g;
    $text =~ s/</&lt;/g;
    $text =~ s/>/&gt;/g;
    return $text;
}

sub check_length {
    my ($line) = @_;
    my ($numc,$numw,@words);

    return 0 if $line =~ /^\d+ [^a-z0-9]+$/i || $line eq "0"  || $line eq "0 ";

    $numc = length($line);
    @words = split(" ",$line);
    $numw = ($#words+1)/2;

    return ($numc <= $MaxChar) && ($numw <= $MaxWord);
}

sub conv_posfmt {
    my ($line) = @_;
    my ($sep,$ret,$w,$i,$w1,$w2,$numw);

    # find the last '_' for each word, and replace it with ' '

    $ret=""; $sep=""; $numw=0;
    for $w (split(" ",$line)) {
	$i = rindex($w,"_");
	$w1 = substr($w,0,$i);	# before _
	$w2 = substr($w,$i+1);	# after _
	$ret .= "$sep$w1 $w2";
	$sep = " "; $numw++;
    }
    $ret = "$numw $ret";

    # also convert '()' into -LRB- and -RRB-
    $ret =~ s/\(/-LRB-/g;
    $ret =~ s/\)/-RRB-/g;

    $ret;
}
