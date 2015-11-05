#!/usr/bin/env perl

use strict;

my $HOSTNAME = "s0565741\@thor.inf.ed.ac.uk";

my $sriPath = $ARGV[0];

my $cmd;

# COMPILE
$cmd = "git pull && ./previous.sh";
system($cmd);

#ZIP
my $machine = `$sriPath/sbin//machine-type`;
chomp($machine);

$cmd = "tar -zcvf $machine.tgz bin lib";
print STDERR "Executing: $cmd \n";
system($cmd);

# UPLOAD
my $date = `date "+%F"`;
chomp($date);

my $targetDir = "/fs/thor1/hieu/binaries/$date/";
print STDERR "Directory=$targetDir\n";

$cmd = "ssh $HOSTNAME mkdir -p $targetDir";
print STDERR "Executing: $cmd \n";
system($cmd);

$cmd = "rsync -rv --delete $machine.tgz $HOSTNAME:$targetDir";
print STDERR "Executing: $cmd \n";
system($cmd);

$cmd = "rm $machine.tgz";
print STDERR "Executing: $cmd \n";
system($cmd);
