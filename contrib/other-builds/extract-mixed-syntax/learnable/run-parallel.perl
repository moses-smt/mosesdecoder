#! /usr/bin/perl -w 

my $iniPath = $ARGV[0];

my $SPLIT_LINES = 100;
my $lineCount = 5000;

for (my $startLine = 0; $startLine < $lineCount; $startLine += $SPLIT_LINES) {
  my $endLine = $startLine + $SPLIT_LINES;

  my $cmd = "../../scripts/reachable.perl $iniPath 1 moses_chart extract-rules tmp-reachable $startLine $endLine &>out.reachable.$startLine &";
  print STDERR "Executing: $cmd \n";
  system($cmd);

}

