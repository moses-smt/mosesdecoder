#!/usr/bin/env perl

use strict;

my $HOME = $ENV{"HOME"};
my $HOSTNAME = "s0565741\@thor.inf.ed.ac.uk";

my $sriPath = $ARGV[0];

my $cmd;

# what machine
my $machine = `uname`;
chomp($machine);

# COMPILE
$cmd = "git checkout master && git pull";
print STDERR "Executing: $cmd \n";
system($cmd);

$cmd = "make -f contrib/Makefiles/install-dependencies.gmake && ./compile.sh --without-tcmalloc";
print STDERR "Executing: $cmd \n";
system($cmd);

#ZIP
if ($machine eq "Darwin") {
  $machine = "mac";
}

$cmd = "mkdir -p mt-tools/moses && mv bin lib mt-tools/moses";
print STDERR "Executing: $cmd \n";
system($cmd);

$cmd = "tar -zcvf $machine.tgz mt-tools";
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
