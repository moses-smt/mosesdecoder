#!/usr/bin/perl -w

# $Id: report-experiment-scores.perl 407 2008-11-10 14:43:31Z philipp $

use strict;

my $email;

my %TYPE;
$TYPE{"nist-bleu"}   = "BLEU";
$TYPE{"multi-bleu"}  = "BLEU";
$TYPE{"nist-bleu-c"} = "BLEU-c";
$TYPE{"multi-bleu-c"}= "BLEU-c";
$TYPE{"ibm-bleu"}    = "IBM";
$TYPE{"ibm-bleu-c"}  = "IBM-c";
$TYPE{"meteor"} = "METEOR";
$TYPE{"bolt-bleu"}     = "BLEU";
$TYPE{"bolt-bleu-c"}   = "BLEU-c";
$TYPE{"bolt-ter"}      = "TER";
$TYPE{"bolt-ter-c"}    = "TER-c";

my %SCORE;
my %AVERAGE;
foreach (@ARGV) {
    if (/^email='(\S+)'/) {
	$email = $1;
    }
}
foreach (@ARGV) {
    if (/^set=(\S+),type=(\S+),file=(\S+)$/) {
	&process($1,$2,$3);
    }
}
foreach my $set (keys %SCORE) {
    my $score = $SCORE{$set};
    chop($score);
    print "$set: $score\n";
}
if ((scalar keys %SCORE) > 1) {
    print  "avg:";
    my $first = 1;
    foreach my $type (sort keys %AVERAGE) {
      print " ;" unless $first; $first = 0;
      printf " %.02f $TYPE{$type}",$AVERAGE{$type}/(scalar keys %SCORE);
    }
    print  "\n";
}

sub process {
    my ($set,$type,$file) = @_;
    $SCORE{$set} .= "; " if defined($SCORE{$set});
    if (! -e $file) {
	print STDERR "ERROR (score $type for set $set): file '$file' does not exist!\n";
    }
    elsif ($type eq 'nist-bleu' || $type eq 'nist-bleu-c') {
	$SCORE{$set} .= &extract_nist_bleu($file,$type)." ";
    }
    elsif ($type eq 'ibm-bleu' || $type eq 'ibm-bleu-c') {
	$SCORE{$set} .= &extract_ibm_bleu($file,$type)." ";
    }
    elsif ($type eq 'multi-bleu' || $type eq 'multi-bleu-c') {
	$SCORE{$set} .= &extract_multi_bleu($file,$type)." ";
    }
    elsif ($type eq 'meteor') {
	$SCORE{$set} .= &extract_meteor($file,$type)." ";
    }
    elsif ($type =~ /^bolt-(.+)$/) {
      $SCORE{$set} .= &extract_bolt($file,$1)." ";
    }
}

sub extract_nist_bleu {
    my ($file,$type) = @_;
    my ($bleu,$ratio);
    foreach (`cat $file`) {
	$bleu = $1*100 if /BLEU score = (\S+)/;
	$ratio = int(1000*$1)/1000 if /length ratio: (\S+)/;
    }
    if (!defined($bleu)) {
	print STDERR "ERROR (extract_nist_bleu): could not find BLEU score in file '$file'\n";
	return "";
    }
    my $output = sprintf("%.02f ",$bleu);
    $output .= sprintf("(%.03f) ",$ratio) if $ratio;

    $AVERAGE{$type} += $bleu;

    return $output.$TYPE{$type};
}

sub extract_ibm_bleu {
    my ($file,$type) = @_;
    my ($bleu,$ratio);
    foreach (`cat $file`) {
	$bleu = $1*100 if /BLEUr\dn4c?,(\S+)/;
	$ratio = int(1000*(1/$1))/1000 if /Ref2SysLen,(\S+)/;
    }
    if (!$bleu) {
	print STDERR "ERROR (extract_ibm_bleu): could not find BLEU score in file '$file'\n";
	return "";
    }
    my $output = sprintf("%.02f ",$bleu);
    $output .= sprintf("(%.03f) ",$ratio) if $ratio;

    $AVERAGE{$type} += $bleu;

    return $output.$TYPE{$type};
}

sub extract_multi_bleu {
    my ($file,$type) = @_;
    my ($bleu,$ratio);
    foreach (`cat $file`) {
	$bleu = $1 if /BLEU = (\S+), /;
	$ratio = $1 if / ration?=(\S+),/;
    }
    my $output = sprintf("%.02f ",$bleu);
    $output .= sprintf("(%.03f) ",$ratio) if $ratio;

    $AVERAGE{"multi-bleu"} += $bleu;

    return $output.$TYPE{$type};
}

sub extract_bolt {
  my ($file,$type) = @_;
  my $score;
  foreach (`cat $file`) {
    $score = $1 if $type eq 'bleu' && /Lowercase BLEU\s+([\d\.]+)/;
    $score = $1 if $type eq 'bleu-c' && /Cased BLEU\s+([\d\.]+)/;
    $score = $1 if $type eq 'ter' && /Lowercase TER\s+([\d\.]+)/;
    $score = $1 if $type eq 'ter-c' && /Cased TER\s+([\d\.]+)/;
  }
  my $output = sprintf("%.02f ",$score*100);
  $AVERAGE{"bolt-".$type} += $score*100;
  return $output.$TYPE{"bolt-".$type};
}
sub extract_meteor {
    my ($file,$type) = @_;
    my ($meteor, $precision);
    foreach (`cat $file`) {
	    $meteor = $1*100 if /Final score:\s*(\S+)/;
	    $precision = $1 if /Precision:\s*(\S+)/;
    }
    my $output = sprintf("%.02f ",$meteor);
    $output .= sprintf("(%.03f) ",$precision) if $precision;
    $AVERAGE{"meteor"} += $meteor;

    return $output.$TYPE{$type};

}
