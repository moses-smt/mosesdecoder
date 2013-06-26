#!/usr/bin/perl -w 

# experiment.perl support script
# get filtered rule and reordering tables and place them into a configuration file

die("ERROR: syntax: base-ini weight-ini out-ini\n") unless scalar @ARGV == 3;

# read initial ini file
my $inWeightSection = 0;
open(OUT, ">".$ARGV[2]) or die "ERROR cannot open out-ini '$ARGV[2]': $!";
open(BASEINI, $ARGV[0]) or die "ERROR cannot open base-ini '$ARGV[0]': $!";
while(my $line = <BASEINI>) {
  chomp($line);
  if ($line =~ /\[weight\]/ || $line =~ /\[weight-file\]/) {
    $inWeightSection = 1;
  }
  elsif ($line =~ /\[[a-zA-Z0-9\-]*\]/) {
    $inWeightSection = 0;
  }  

  if (!$inWeightSection) {
    print OUT "$line\n" unless $line =~ /dense weights for feature functions/;
  }
}
close(BASEINI);

# read tuned ini file
$inWeightSection = 0;
open(WEIGHTINI, $ARGV[1]) or die "ERROR cannot open weight-ini '$ARGV[1]': $!";
while(my $line = <WEIGHTINI>) {
  chomp($line);
  if ($line =~ /\[weight\]/) {
   $inWeightSection = 1;
   print OUT "# core weights\n";
  }
  elsif ($line =~ /\[weight-file\]/) {
    print OUT "# sparse weights\n";
    print OUT "$line\n";
    my $sparse_weight_file = <WEIGHTINI>;
    chop($sparse_weight_file);
    # copy sparse feature file
    `cp $sparse_weight_file $ARGV[2].sparse`;
    print OUT "$ARGV[2].sparse\n\n";
    $inWeightSection = 0;
  }
  elsif ($line =~ /\[[a-zA-Z0-9\-]*\]/) {
   print OUT "\n" if $inWeightSection;
   $inWeightSection = 0;
  }  

  if ($inWeightSection && $line !~ /^\s*$/) {
    print OUT "$line\n";
  }
}
close(WEIGHTINI);
