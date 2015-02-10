#!/usr/bin/perl -w 

# experiment.perl support script
# get filtered rule and reordering tables and place them into a configuration file

if (scalar @ARGV < 1 || ! -e $ARGV[0]) {
  die("ERROR: could not find pseudo-config with filtered tables");
}

# read config sections about filtered tables
my @arr;
open(FILTERED, $ARGV[0]) or die "Cannot open: $!";
my $feature_section = 0;
while(my $line = <FILTERED>) {
  chomp($line);
  if ($line =~ /^\[(.+)\]/) {
    $feature_section = ($1 eq "feature");
  }
  next unless $feature_section;
  if ($line =~ /PhraseDictionary/ || $line =~ /RuleTable/) {
    print STDERR "pt:$line \n";
    push(@arr, $line);
  }
  elsif ($line =~ /LexicalReordering/) {
    print STDERR "ro:$line \n";
    push(@arr, $line);
  }
}
close(FILTERED);

# pass through master config file and replace table sections
my $ind = 0;
$feature_section = 0;
while(my $line = <STDIN>) {
  chomp($line);
  if ($line =~ /^\[(.+)\]/) {
    $feature_section = ($1 eq "feature");
  }
  if ($feature_section && ($line =~ /PhraseDictionary/ || $line =~ /RuleTable/)) {
    print $arr[$ind]."\n";
    ++$ind;
  }
  elsif ($feature_section && $line =~ /LexicalReordering/) {
    print $arr[$ind]."\n";
    ++$ind;
  }  
  else {
    print "$line\n";
  }
}
