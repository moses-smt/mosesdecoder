#!/usr/bin/perl -w

use strict;
use Getopt::Long "GetOptions";

my ($TEXT,$ORDER,$PRUNE,$BIN,$LM,$MEMORY,$TMP);

&GetOptions('text=s' => \$TEXT,
	    'lm=s' => \$LM,
            'bin=s' => \$BIN,
            'prune=s' => \$PRUNE,
            'T=s' => \$TMP,
            'S=s' => \$MEMORY,
	    'order=i' => \$ORDER);

die("ERROR: specify at least --text CORPUS --arpa LM and --order N!")
  unless defined($TEXT) && defined($LM) && defined($ORDER);

my $cmd = "$BIN --text $TEXT --order $ORDER --arpa $LM";
$cmd .= " --prune $PRUNE" if defined($PRUNE);
$cmd .= " -S $MEMORY" if defined($MEMORY);
$cmd .= " -T $TMP" if defined($TMP);

print "exec: $cmd\n";
`$cmd`;
