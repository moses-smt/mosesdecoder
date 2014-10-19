#!/usr/bin/perl -w

# $Id$
use strict;
use FindBin qw($Bin);
use Getopt::Long "GetOptions";

binmode(STDIN, ":utf8");
binmode(STDOUT, ":utf8");

# apply switches
my ($DIR,$CORPUS,$SCRIPTS_ROOT_DIR,$CONFIG,$HELP,$ERROR);
my $LM = "KENLM"; # KENLM is default.
my $BUILD_LM = "build-lm.sh";
my $BUILD_KENLM = "$Bin/../../bin/lmplz";
my $NGRAM_COUNT = "ngram-count";
my $TRAIN_SCRIPT = "train-factored-phrase-model.perl";
my $MAX_LEN = 1;
my $FIRST_STEP = 1;
my $LAST_STEP = 11;
$ERROR = "training Aborted."
    unless &GetOptions('first-step=i' => \$FIRST_STEP,
                       'last-step=i' => \$LAST_STEP,
                       'corpus=s' => \$CORPUS,
                       'config=s' => \$CONFIG,
                       'dir=s' => \$DIR,
                       'ngram-count=s' => \$NGRAM_COUNT,
                       'build-lm=s' => \$BUILD_LM,
                       'build-kenlm=s' => \$BUILD_KENLM,
                       'lm=s' => \$LM,
                       'train-script=s' => \$TRAIN_SCRIPT,
                       'scripts-root-dir=s' => \$SCRIPTS_ROOT_DIR,
                       'max-len=i' => \$MAX_LEN,
                       'help' => \$HELP);

# check and set default to unset parameters
$ERROR = "please specify working dir --dir" unless defined($DIR) || defined($HELP);
$ERROR = "please specify --corpus" if !defined($CORPUS) && !defined($HELP) 
                                  && $FIRST_STEP <= 2 && $LAST_STEP >= 1;

if ($HELP || $ERROR) {
    if ($ERROR) {
        print STDERR "ERROR: " . $ERROR . "\n";
    }
    print STDERR "Usage: $0 --dir /output/recaser --corpus /Cased/corpus/files [options ...]";

    print STDERR "\n\nOptions:
  == MANDATORY ==
  --dir=dir                 ... outputted recaser directory.
  --corpus=file             ... inputted cased corpus.

  == OPTIONAL ==
  = Recaser Training configuration =
  --train-script=file       ... path to the train script (default: train-factored-phrase-model.perl in \$PATH).
  --config=config           ... training script configuration.
  --scripts-root-dir=dir    ... scripts directory.
  --max-len=int             ... max phrase length (default: 1).

  = Language Model Training configuration =
  --lm=[IRSTLM,SRILM,KENLM] ... language model (default: KENLM).
  --build-lm=file           ... path to build-lm.sh if not in \$PATH (used only with --lm=IRSTLM).
  --ngram-count=file        ... path to ngram-count.sh if not in \$PATH (used only with --lm=SRILM).

  = Steps this script will perform =
  (1) Truecasing;
  (2) Language Model Training;
  (3) Data Preparation
  (4-10) Recaser Model Training; 
  (11) Cleanup.
  --first-step=[1-11]       ... step where script starts (default: 1).
  --last-step=[1-11]        ... step where script ends (default: 11).

  --help                    ... this usage output.\n";
  if ($ERROR) {
    exit(1);
  }
  else {
    exit(0);
  }
}

# main loop
`mkdir -p $DIR`;
&truecase()           if $FIRST_STEP == 1;
$CORPUS = "$DIR/aligned.truecased" if (-e "$DIR/aligned.truecased");
&train_lm()           if $FIRST_STEP <= 2;
&prepare_data()       if $FIRST_STEP <= 3 && $LAST_STEP >= 3;
&train_recase_model() if $FIRST_STEP <= 10 && $LAST_STEP >= 3;
&cleanup()            if $LAST_STEP == 11;

exit(0);

### subs ###

sub truecase {
    print STDERR "(1) Truecase data @ ".`date`;
    print STDERR "(1) To build model without truecasing, use --first-step 2, and make sure $DIR/aligned.truecased does not exist\n";

    my $cmd = "$Bin/train-truecaser.perl --model $DIR/truecaser_model --corpus $CORPUS";
    print STDERR $cmd."\n";
    system($cmd) == 0 || die("Training truecaser died with error " . ($? >> 8) . "\n");

    $cmd = "$Bin/truecase.perl --model $DIR/truecaser_model < $CORPUS > $DIR/aligned.truecased";
    print STDERR $cmd."\n";
    system($cmd) == 0 || die("Applying truecaser died with error " . ($? >> 8) . "\n");

}

sub train_lm {
    print STDERR "(2) Train language model on cased data @ ".`date`;
    my $cmd = "";
    if (uc $LM eq "IRSTLM") {
        $cmd = "$BUILD_LM -t /tmp -i $CORPUS -n 3 -o $DIR/cased.irstlm.gz";
    }
    elsif (uc $LM eq "SRILM") {
        $LM = "SRILM";
        $cmd = "$NGRAM_COUNT -text $CORPUS -lm $DIR/cased.srilm.gz -interpolate -kndiscount";
    }
    else {
        $LM = "KENLM";
        $cmd = "$BUILD_KENLM --prune 0 0 1 -S 5% -T $DIR/lmtmp --order 3 --text $CORPUS --arpa $DIR/cased.kenlm.gz";
    }
    print STDERR "** Using $LM **" . "\n";
    print STDERR $cmd."\n";
    system($cmd) == 0 || die("Language model training failed with error " . ($? >> 8) . "\n");
}

sub prepare_data {
    print STDERR "\n(3) Preparing data for training recasing model @ ".`date`;
    open(CORPUS,$CORPUS);
    binmode(CORPUS, ":utf8");
    open(CASED,">$DIR/aligned.cased");
    binmode(CASED, ":utf8");
    print "$DIR/aligned.lowercased\n";
    open(LOWERCASED,">$DIR/aligned.lowercased");
    binmode(LOWERCASED, ":utf8");
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
    my $cmd = "$TRAIN_SCRIPT --root-dir $DIR --model-dir $DIR --first-step $first --alignment a --corpus $DIR/aligned --f lowercased --e cased --max-phrase-length $MAX_LEN";
    if (uc $LM eq "IRSTLM") {
        $cmd .= " --lm 0:3:$DIR/cased.irstlm.gz:1";
    }
    elsif (uc $LM eq "SRILM") {
        $cmd .= " --lm 0:3:$DIR/cased.srilm.gz:8";
    }
    else {
        $cmd .= " --lm 0:3:$DIR/cased.kenlm.gz:8";
    }
    $cmd .= " -config $CONFIG" if $CONFIG;
    print STDERR $cmd."\n";
    system($cmd) == 0 || die("Recaser model training failed with error " . ($? >> 8) . "\n");
}

sub cleanup {
    print STDERR "\n(11) Cleaning up @ ".`date`;
    `rm -f $DIR/extract*`;
    my $clean_1 = $?;
    `rm -f $DIR/aligned*`;
    my $clean_2 = $?;
    `rm -f $DIR/lex*`;
    my $clean_3 = $?;
    `rm -f $DIR/truecaser_model`;
    my $clean_4 = $?;
    if ($clean_1 + $clean_2 + $clean_3 + $clean_4 != 0) {
        print STDERR "Training successful but some files could not be cleaned.\n";
    }
}
