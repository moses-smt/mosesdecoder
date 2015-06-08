#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;
use Getopt::Long "GetOptions";
use FindBin qw($RealBin);
use File::Basename;
use File::Temp qw/tempfile/;

my $BITPAR = "/exports/home/s0565741/work/bin/bitpar";
my $TMPDIR = "tmp";

my $SCRIPTS_ROOT_DIR = "$RealBin/../..";
my $DEESCAPE = "$SCRIPTS_ROOT_DIR/tokenizer/deescape-special-chars.perl";

my $DEBUG = 0;
my $BASIC = 0;
my $OLD_BITPAR = 0;
my $UNPARSEABLE = 0;

my $RAW = "";

GetOptions(
    "basic" => \$BASIC,
    "bitpar=s" => \$BITPAR,
    "old-bitpar" => \$OLD_BITPAR,
    "raw=s" => \$RAW,
    "unparseable" => \$UNPARSEABLE
    ) or die("ERROR: unknown options");

`mkdir -p $TMPDIR`;
my ($scriptname, $directories) = fileparse($0);
my ($TMP, $tmpfile) = tempfile("$scriptname-XXXXXXXXXX", DIR=>$TMPDIR, UNLINK=>1);
if ($OLD_BITPAR)
{
  open(INPUT,"$DEESCAPE | iconv -c -f UTF-8 -t iso-8859-1 |");
}
else
{
  open (INPUT,"$DEESCAPE |");
}
while(<INPUT>)
{
		my $hasWords = 0;
	  foreach (split)
    {
        if ($OLD_BITPAR)
        {
            s/\(/\*LRB\*/g;
            s/\)/\*RRB\*/g;
        }
        print $TMP $_."\n";

				$hasWords = 1;
    }

    if ($hasWords == 0) {
    	print $TMP " \n";
    }

    print $TMP "\n";
}
close($TMP);

my $pipeline = "cat $tmpfile | $BITPAR/BitPar/src/bitpar -ts '()' -s TOP -v $BITPAR/GermanParser/Tiger/grammar $BITPAR/GermanParser/Tiger/lexicon -u $BITPAR/GermanParser/Tiger/open-class-tags -w $BITPAR/GermanParser/Tiger/wordclass.txt |";
if ($RAW)
{
    $pipeline .= "tee \"$RAW\" |";
}
if ($OLD_BITPAR)
{
  $pipeline .= "iconv -c -t UTF-8 -f iso-8859-1 |";
}
open(PARSER,$pipeline);
while(my $line = <PARSER>) {
    if ($line =~ /^No parse for/) {
        if ($UNPARSEABLE) {
          my $len = length($line);
          $line = substr($line, 15, $len - 17);
          $line = escape($line);
          print $line;
        }
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
            if (&is_aux_label($label)) {
                # Discard auxiliary nodes.  Since we don't create a <tree> node,
                # any (non-auxiliary) subtrees will instead be attached to the
                # parent (or closest non-auxiliary ancestor).
            }
            else {
                if ($label =~ /^\\\$(,|.|Par)$/)
                {
                    $label = "PUNC$1";
                }
                else
                {
                    $label =~ s/\$/PUNC/g;       # no $!
                    $label =~ s/\|/:/g;          # moses does not like bars
                    $label =~ s/^[^A-Z]*([A-Z]+).*/$1/g if $BASIC; # basic labels only
                }
                push @OUT,"<tree label=\"$label\">";
            }
            push @LABEL,$label;
            $i++ if substr($line,$i+1,1) eq " ";
	    $i++ if substr($line,$i+1,1) eq " ";
        }
        elsif (substr($line,$i,1) eq ")") {
            die("ERROR: NO LABEL ON STACK") unless @LABEL;
            my $label = pop @LABEL;
            print STDERR substr("                                                                               ",0,scalar @LABEL)."END of $label\n" if $DEBUG;
            if (!&is_aux_label($label)) {
                push @OUT,"</tree>";
            }
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
            push @OUT,&escape($word);
        }
    }
    die("ERROR: STACK NOT EMPTY $#LABEL\n") if @LABEL;
    my $first=1;
    foreach (@OUT) {
        print " " unless $first;
        s/\\//;
        if ($OLD_BITPAR)
        {
            s/\*RRB\*/\)/g;
            s/\*LRB\*/\(/g;
        }
        print $_;
        $first = 0;
    }
    print "\n";
}

sub is_aux_label {
    my ($label) = @_;
    return ($label =~ /^\\<.*\\>$/);
}

sub escape {
    my ($text) = @_;
    $text =~ s/&/&amp;/g;
    $text =~ s/\|/&#124;/g;
    $text =~ s/</&lt;/g;
    $text =~ s/>/&gt;/g;
    $text =~ s/'/&apos;/g;
    $text =~ s/"/&quot;/g;
    $text =~ s/\[/&#91;/g;
    $text =~ s/\]/&#93;/g;
    return $text;
}
