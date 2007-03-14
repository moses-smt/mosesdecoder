#!/usr/bin/perl -w
 
# $Id$
use strict;
use Getopt::Long "GetOptions";

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");

# apply switches
my ($DIR,$CORPUS,$SCRIPTS_ROOT_DIR,$CONFIG);
my $NGRAM_COUNT = "ngram-count";
my $TRAIN_SCRIPT = "train-factored-phrase-model.perl";
my $MAX_LEN = 1;
my $FIRST_STEP = 1;
my $LAST_STEP = 11;
die("train-recaser.perl --dir recaser --corpus cased")
    unless &GetOptions('first-step=i' => \$FIRST_STEP,
                       'last-step=i' => \$LAST_STEP,
                       'corpus=s' => \$CORPUS,
                       'config=s' => \$CONFIG,
		       'dir=s' => \$DIR,
		       'ngram-count=s' => \$NGRAM_COUNT,
		       'train-script=s' => \$TRAIN_SCRIPT,
		       'scripts-root-dir=s' => \$SCRIPTS_ROOT_DIR,
		       'max-len=i' => \$MAX_LEN);

# check and set default to unset parameters
die("please specify working dir --dir") unless defined($DIR);
die("please specify --corpus") if !defined($CORPUS) 
                                  && $FIRST_STEP <= 2 && $LAST_STEP >= 1;

# main loop
`mkdir -p $DIR`;
&truecase()           if 0 && $FIRST_STEP == 1;
&train_lm()           if $FIRST_STEP <= 2;
&prepare_data()       if $FIRST_STEP <= 3 && $LAST_STEP >= 3;
&train_recase_model() if $FIRST_STEP <= 10 && $LAST_STEP >= 3;
&cleanup()            if $LAST_STEP == 11;

### subs ###

sub truecase {
    # to do
}

sub train_lm {
    print STDERR "(2) Train language model on cased data @ ".`date`;
    my $cmd = "$NGRAM_COUNT -text $CORPUS -lm $DIR/cased.srilm.gz -interpolate -kndiscount";
    print STDERR $cmd."\n";
    print STDERR `$cmd`;
}

sub prepare_data {
    print STDERR "\n(3) Preparing data for training recasing model @ ".`date`;
    open(CORPUS,$CORPUS);
    open(CASED,">$DIR/aligned.cased");
    print "$DIR/aligned.lowercased\n";
    open(LOWERCASED,">$DIR/aligned.lowercased");
    open(ALIGNMENT,">$DIR/aligned.a");
    while(<CORPUS>) {
	next if length($_)>2000;
	s/\x{0}//g;
	s/\|//g;
	s/ +/ /g;
	s/^ //;
	s/ [\r\n]*$/\n/;
	next if /^$/;
	print CASED $_;
	print LOWERCASED lc($_);
	my $i=0;
	foreach (split) {
	    print ALIGNMENT "$i-$i ";
	    $i++;
	}
	print ALIGNMENT "\n";
    }
    close(CORPUS);
    close(CASED);
    close(LOWERCASED);
    close(ALIGNMENT);
}

sub train_recase_model {
    my $first = $FIRST_STEP;
    $first = 4 if $first < 4;
    print STDERR "\n(4) Training recasing model @ ".`date`;
    my $cmd = "$TRAIN_SCRIPT --root-dir $DIR --model-dir $DIR --first-step $first --alignment a --corpus $DIR/aligned --f lowercased --e cased --max-phrase-length $MAX_LEN --lm 0:3:$DIR/cased.srilm.gz:0";
    $cmd .= " -scripts-root-dir $SCRIPTS_ROOT_DIR" if $SCRIPTS_ROOT_DIR;
    $cmd .= " -config $CONFIG" if $CONFIG;
    print STDERR $cmd."\n";
    print STDERR `$cmd`;
}

sub cleanup {
    print STDERR "\n(11) Cleaning up @ ".`date`;
    `rm -f $DIR/extract*`;
    `rm -f $DIR/aligned*`;
    `rm -f $DIR/lex*`;
}
