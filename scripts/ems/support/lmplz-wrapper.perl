#!/usr/bin/perl -w

use strict;
use Getopt::Long "GetOptions";

my ($TEXT,$ORDER,$BIN,$LM) = @_;
&GetOptions('text=s' => \$TEXT,
	    'lm=s' => \$LM,
            'bin=s' => \$BIN,
	    'order=i' => \$ORDER);

`$BIN --text $TEXT --order $ORDER --arpa $LM`;
