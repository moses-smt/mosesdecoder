#!/usr/bin/perl -w

use strict;
use Getopt::Long "GetOptions";

my ($TEXT,$ORDER,$BIN,$LM,$TMP);

&GetOptions('text=s' => \$TEXT,
	    'lm=s' => \$LM,
	    'tmp=s' => \$TMP,
            'bin=s' => \$BIN,
	    'order=i' => \$ORDER);

die("ERROR: specify --text CORPUS --lm LM --order N --bin THOT_BINARY !")
  unless defined($TEXT) && defined($LM) && defined($ORDER) && defined($BIN);

my $cmd = "$BIN -c $TEXT -n $ORDER -o $LM -unk -sdir $TMP -tdir $TMP";

print "exec: $cmd\n";
`$cmd`;
