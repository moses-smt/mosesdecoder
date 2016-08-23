#!/usr/bin/perl -w
use warnings;
use strict;
$|++;

# file: start-daemon-cluster.pl

# Herve Saint-Amand
# Universitaet des Saarlandes
# Thu May 15 08:22:13 2008

# Utility to start/stop the daemon processes on the 16 cluster machines.
# Config in here should match that given in translate.cgi (hostnames, ports)

#------------------------------------------------------------------------------

my $stop = @ARGV && $ARGV[0] eq '-s';

foreach my $i (qw/01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16/) {
    my $host = "cluster-$i";
    my $port = "90$i";

    my @cmd = $stop ?
        ('ssh', $host, 'killall', '-q', 'daemon.pl')
        :
        ('ssh', 
         '-L', "$port:localhost:$port",
         $host,
         '/local/herves/moses/daemon.pl', 'localhost', $port);

    print "@cmd\n";
    exec @cmd unless fork ();
}

#------------------------------------------------------------------------------
