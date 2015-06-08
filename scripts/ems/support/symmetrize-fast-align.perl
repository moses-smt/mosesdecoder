#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;

die("ERROR: syntax is fastalign2bal.perl direct-alignment inverse-alignment source-file target-file out-stem symmetrization-method symal\n") unless scalar(@ARGV) == 7;

my ($direct_file,$inverse_file,$source_file,$target_file,$out_stem,$symmetrization_method,$symal) = @ARGV;

# symal options (from train-model.perl)
my ($__symal_a,$__symal_d,$__symal_f,$__symal_b) = ("","no","no","no");
$__symal_a = "union" if $symmetrization_method eq 'union';
$__symal_a = "intersect" if $symmetrization_method=~ /intersect/;
$__symal_a = "grow" if $symmetrization_method=~ /grow/;
$__symal_a = "srctotgt" if $symmetrization_method=~ /srctotgt/;
$__symal_a = "tgttosrc" if $symmetrization_method=~ /tgttosrc/;
$__symal_d = "yes" if $symmetrization_method=~ /diag/;
$__symal_f = "yes" if $symmetrization_method=~ /final/;
$__symal_b = "yes" if $symmetrization_method=~ /final-and/;
my $symal_options = "-alignment=\"$__symal_a\" -diagonal=\"$__symal_d\" -final=\"$__symal_f\" -both=\"$__symal_b\"";

# open files
open(DIRECT,$direct_file)   || die("ERROR: can't open direct alignment file '$direct_file'");
open(INVERSE,$inverse_file) || die("ERROR: can't open inverse alignment file '$inverse_file'");
open(SOURCE,$source_file)   || die("ERROR: can't open source corpus file '$source_file'");
open(TARGET,$target_file)   || die("ERROR: can't open target corpus file '$target_file'");
open(OUT,"| $symal $symal_options > $out_stem.$symmetrization_method");

# loop through sentence pairs and bi-directional alignments
while(my $direct = <DIRECT>) {
  my $inverse = <INVERSE>;
  my $source = <SOURCE>;
  my $target = <TARGET>;

  print OUT "1\n";
  &convert($target,$direct,0);
  &convert($source,$inverse,1);
}
close(TARGET);
close(SOURCE);
close(INVERSE);
close(DIRECT);

sub convert {
  my ($text,$alignment,$is_inverse) = @_;
  chop($text);
  chop($alignment);
  $text =~ s/\<[^\>]+\>/ /g;
  $text =~ s/\s+/ /;
  $text =~ s/ $//;
  $text =~ s/^ //;
  $alignment =~ s/\s+$//;
  my @TEXT = split(/\s+/,$text);
  print OUT scalar(@TEXT)." ".$text." #";
  #print STDERR scalar(@TEXT)." ".$text." #";
  my %ALIGNMENT;
  foreach (split(/\s+/,$alignment)) {
    my ($target,$source);
    ($target,$source) = split(/\-/,$_) unless $is_inverse;
    ($source,$target) = split(/\-/,$_) if $is_inverse;
    $ALIGNMENT{$source} = $target+1;
  }
  for(my $i=0;$i<@TEXT;$i++) {
    print OUT " ".(defined($ALIGNMENT{$i}) ? $ALIGNMENT{$i} : 0);
    #print STDERR " ".(defined($ALIGNMENT{$i}) ? $ALIGNMENT{$i} : 0);
  }
  print OUT "\n";
  #print STDERR "\n";
}
