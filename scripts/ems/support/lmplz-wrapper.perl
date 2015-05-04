#!/usr/bin/env perl 

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
$cmd .= " " . join(' ', @ARGV) if scalar(@ARGV);  # Pass remaining args through.
print "exec: $cmd\n";
`$cmd`;
