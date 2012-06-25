#!/usr/bin/perl -w
use FindBin qw($Bin);
use warnings;
use strict;
$|++;

# file: daemon.pl

# Herve Saint-Amand
# Universitaet des Saarlandes
# Tue May 13 19:45:31 2008

# This script starts Moses to run in the background, so that it can be used by
# the CGI script. It spawns the Moses process, then binds itself to listen on
# some port, and when it gets a connection, reads it line by line, feeds those
# to Moses, and sends back the translation.

# You can either run one instance of this on your Web server, or, if you have
# the hardware setup for it, run several instances of this, then configure
# translate.cgi to connect to these.

#------------------------------------------------------------------------------
# includes

use IO::Socket::INET;
use IPC::Open2;

#------------------------------------------------------------------------------
# constants, global vars, config

my $MOSES      = "$Bin/../../../bin/moses";

die "usage: daemon.pl <hostname> <port> <ini>" unless (@ARGV == 3);
my $LISTEN_HOST = shift;
my $LISTEN_PORT = shift;
my $MOSES_INI = shift;

#------------------------------------------------------------------------------
# main

# spawn moses
my ($MOSES_IN, $MOSES_OUT);
my $pid = open2 ($MOSES_OUT, $MOSES_IN, $MOSES, '-f', $MOSES_INI, '-t');

# open server socket
my $server_sock = new IO::Socket::INET
    (LocalAddr => $LISTEN_HOST, LocalPort => $LISTEN_PORT, Listen => 1)
    || die "Can't bind server socket";

while (my $client_sock = $server_sock->accept) {
    while (my $line = <$client_sock>) {
        print $MOSES_IN $line;
        $MOSES_IN->flush ();
        print $client_sock scalar <$MOSES_OUT>;
    }

    $client_sock->close ();
}

#------------------------------------------------------------------------------
