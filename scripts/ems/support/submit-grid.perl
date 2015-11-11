#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;
use Cwd;
use FindBin qw($RealBin);
use Getopt::Long;
use File::Basename;


my $continue = 0;
my $args = "";
my $config;

GetOptions("continue=i"  => \$continue,
	   "args=s" => \$args,
           "config=s" => \$config
    ) or exit 1;
#print STDERR "args=$args\n";

# create temp run file
my $gridDir = cwd() ."/grid";
mkdir $gridDir;

my $runPath = "$gridDir/run.$$";
print STDERR "runPath=$runPath\n";

open (my $runFile, ">", $runPath);

print $runFile "#!/bin/bash\n";
print $runFile "#PBS -d" .cwd() ."\n\n";

my $path = $ENV{"PATH"};
my $user = $ENV{"USER"};
#print STDERR "path=$path\n";

print $runFile "export PATH=\"$path\"\n\n";
print $runFile "export PERL5LIB=\"/share/apps/NYUAD/perl/gcc_4.9.1/5.20.1:/home/$user/perl5/lib/perl5\"\n\n";

print $runFile "module load  NYUAD/2.0 \n";
print $runFile "module load gcc python/2.7.9 boost cmake zlib jdk perl expat \n\n";

my $emsDir = dirname($RealBin);

if ($continue) {
  print $runFile "nice ionice -c 3 $emsDir/experiment.perl -exec -continue=$continue \n\n";
}
else {
  print $runFile "nice ionice -c 3 $emsDir/experiment.perl -exec -config=$config \n\n";
}

close $runFile;


my $cmd = "qsub $args $runPath";
`$cmd`;

unlink $runFile;




