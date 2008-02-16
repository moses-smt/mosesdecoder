#!/usr/bin/perl
$x=0;
$oldcode = "";
while (<>) {
  chomp;
  ($code,$trans,$featscores,$globscores) = split(/[\s]*\|\|\|[\s]*/,$_);
  $x = 0 if $oldcode ne $code;
  $x++;
  chomp($code);
  print "TRANSLATION_${code}_NBEST_${x}=$trans ||| $featscores\n";
  $oldcode = $code;
}
