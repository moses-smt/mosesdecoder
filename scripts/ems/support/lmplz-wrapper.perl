#!/usr/bin/env perl 

use warnings;
use strict;
use Getopt::Long "GetOptions";

Getopt::Long::config("pass_through");

my ($TEXT,$ORDER,$BIN,$LM);

&GetOptions('text=s' => \$TEXT,
	    'lm=s' => \$LM,
            'bin=s' => \$BIN,
	    'order=i' => \$ORDER);

die("ERROR: specify at least --bin BIN --text CORPUS --lm LM and --order N!")
  unless defined($BIN) && defined($TEXT) && defined($LM) && defined($ORDER);

my $cmd = "$BIN --text $TEXT --order $ORDER --arpa $LM";
$cmd .= " " . join(' ', @ARGV) if scalar(@ARGV);  # Pass remaining args through.

print "exec: $cmd\n";
`$cmd`;
