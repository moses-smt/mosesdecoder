#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

use warnings;
use strict;
use Getopt::Long "GetOptions";

Getopt::Long::config("no_auto_abbrev");
Getopt::Long::config("pass_through");

my ($TEXT,$ORDER,$BIN,$LM,$MEMORY,$TMPDIR);

&GetOptions('text=s' => \$TEXT,
	    'lm=s' => \$LM,
	    'S=s' => \$MEMORY,
	    'T=s' => \$TMPDIR,
            'bin=s' => \$BIN,
	    'order=i' => \$ORDER);

die("ERROR: specify at least --bin BIN --text CORPUS --lm LM and --order N!")
  unless defined($BIN) && defined($TEXT) && defined($LM) && defined($ORDER);

my $settings = join(' ', @ARGV);

my $cmd = "$BIN --text $TEXT --order $ORDER --arpa $LM $settings";
$cmd .= " -T $TMPDIR" if defined($TMPDIR);
$cmd .= " -S $MEMORY" if defined($MEMORY);
print STDERR "Executing: $cmd\n";
`$cmd`;
