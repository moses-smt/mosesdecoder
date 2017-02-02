#!/usr/bin/perl

use Getopt::Long qw(:config pass_through no_ignore_case permute);

my $poll_target = undef;
my $working_dir = undef;

GetOptions('poll-target=s'=> \$poll_target,
           'working-dir'=> \$working_dir
    ) or exit(1);


if (defined $working_dir) {
  chdir($working_dir);
}

my $cnt = 1;

print STDERR "Wait for file: $poll_target\n";

while (1) {
  if (-e $poll_target){
    print STDERR "\n File found!!\n";
    last;
  } else {
    sleep(10);
    print STDERR ".";
  }
}

