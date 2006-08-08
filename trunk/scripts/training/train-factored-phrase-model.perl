#!/usr/bin/perl -w

use strict;
use Getopt::Long "GetOptions";

# Train Factored Phrase Model
# (c) 2006 Philipp Koehn
# with contributions from other JHU WS participants
# Train a phrase model from a parallel corpus

# -----------------------------------------------------
$ENV{"LC_ALL"} = "C";

my($_ROOT_DIR,$_CORPUS_DIR,$_GIZA_E2F,$_GIZA_F2E,$_MODEL_DIR,$_CORPUS,$_FIRST_STEP,$_LAST_STEP,$_F,$_E,$_MAX_PHRASE_LENGTH,$_LEXICAL_DIR,$_NO_LEXICAL_WEIGHTING,$_VERBOSE,$_ALIGNMENT,@_LM,$_EXTRACT_FILE,$_GIZA_OPTION,$_HELP,$_PARTS,$_DIRECTION,$_ONLY_PRINT_GIZA,$_REORDERING,$_REORDERING_SMOOTH,$_ALIGNMENT_FACTORS,$_TRANSLATION_FACTORS,$_REORDERING_FACTORS,$_GENERATION_FACTORS,$_DECODING_STEPS,$_PARALLEL, $SCRIPTS_ROOTDIR);


$_HELP = 1
    unless &GetOptions('root-dir=s' => \$_ROOT_DIR,
		       'corpus-dir=s' => \$_CORPUS_DIR,
		       'corpus=s' => \$_CORPUS,
		       'f=s' => \$_F,
		       'e=s' => \$_E,
		       'giza-e2f=s' => \$_GIZA_E2F,
		       'giza-f2e=s' => \$_GIZA_F2E,
		       'max-phrase-length=i' => \$_MAX_PHRASE_LENGTH,
		       'lexical-dir=s' => \$_LEXICAL_DIR,
		       'no-lexical-weighting' => \$_NO_LEXICAL_WEIGHTING,
		       'model-dir=s' => \$_MODEL_DIR,
		       'extract-file=s' => \$_EXTRACT_FILE,
		       'alignment=s' => \$_ALIGNMENT,
		       'verbose' => \$_VERBOSE,
		       'first-step=i' => \$_FIRST_STEP,
		       'last-step=i' => \$_LAST_STEP,
		       'giza-option=s' => \$_GIZA_OPTION,
		       'parallel' => \$_PARALLEL,
		       'lm=s' => \@_LM,
		       'help' => \$_HELP,
		       'parts=i' => \$_PARTS,
		       'direction=i' => \$_DIRECTION,
		       'only-print-giza' => \$_ONLY_PRINT_GIZA,
		       'reordering=s' => \$_REORDERING,
		       'reordering-smooth=s' => \$_REORDERING_SMOOTH,
		       'alignment-factors=s' => \$_ALIGNMENT_FACTORS,
		       'translation-factors=s' => \$_TRANSLATION_FACTORS,
		       'reordering-factors=s' => \$_REORDERING_FACTORS,
		       'generation-factors=s' => \$_GENERATION_FACTORS,
		       'decoding-steps=s' => \$_DECODING_STEPS,
		       'scripts-root-dir=s' => \$SCRIPTS_ROOTDIR,
                      );

if ($_HELP) {
    print "Train Phrase Model

Steps: (--first-step to --last-step)
(1) prepare corpus
(2) run GIZA
(3) align words
(4) learn lexical translation
(5) extract phrases
(6) score phrases
(7) learn reordering model
(8) learn generation model
(9) create decoder config file

For more, please check manual or contact koehn\@inf.ed.ac.uk\n";
  exit(1);
}

if (!defined $SCRIPTS_ROOTDIR) {
  $SCRIPTS_ROOTDIR = $ENV{"SCRIPTS_ROOTDIR"};
  die "Please set SCRIPTS_ROOTDIR or specify --rootdir" if !defined $SCRIPTS_ROOTDIR;
}
print STDERR "Using SCRIPTS_ROOTDIR: $SCRIPTS_ROOTDIR\n";


# these variables may have to be adapted to local paths
my $BINDIR = "/export/ws06osmt/bin";
my $GIZA = "$BINDIR/GIZA++";
my $SNT2COOC = "$BINDIR/snt2cooc.out"; 
my $PHRASE_EXTRACT = "$SCRIPTS_ROOTDIR/training/phrase-extract/extract";
my $PHRASE_SCORE = "$SCRIPTS_ROOTDIR/training/phrase-extract/score";
my $MKCLS = "$BINDIR/mkcls";
my $ZCAT = "zcat";
my $BZCAT = "bzcat";


# set varibles to defaults or from options
my $___ROOT_DIR = ".";
$___ROOT_DIR = $_ROOT_DIR if $_ROOT_DIR;
my $___CORPUS_DIR  = $___ROOT_DIR."/corpus";
$___CORPUS_DIR = $_CORPUS_DIR if $_CORPUS_DIR;
die("use --corpus to specify corpus") unless $_CORPUS || ($_FIRST_STEP && $_FIRST_STEP>1);
my $___CORPUS      = $_CORPUS;

# foreign/English language extension
die("use --f to specify foreign language") unless $_F;
die("use --e to specify English language") unless $_E;
my $___F = $_F;
my $___E = $_E;

# vocabulary files in corpus dir
my $___VCB_E = $___CORPUS_DIR."/".$___E.".vcb";
my $___VCB_F = $___CORPUS_DIR."/".$___F.".vcb";

# GIZA generated files
my $___GIZA = $___ROOT_DIR."/giza";
my $___GIZA_E2F = $___GIZA.".".$___E."-".$___F;
my $___GIZA_F2E = $___GIZA.".".$___F."-".$___E;
$___GIZA_E2F = $_GIZA_E2F if $_GIZA_E2F;
$___GIZA_F2E = $_GIZA_F2E if $_GIZA_F2E;
my $___GIZA_OPTION = "";
$___GIZA_OPTION = $_GIZA_OPTION if $_GIZA_OPTION;

# alignment heuristic
my $___ALIGNMENT = "grow-diag-final";
$___ALIGNMENT = $_ALIGNMENT if $_ALIGNMENT;
my $___NOTE_ALIGNMENT_DROPS = 1;

# model dir and extract file
my $___MODEL_DIR = $___ROOT_DIR."/model";
$___MODEL_DIR = $_MODEL_DIR if $_MODEL_DIR;
my $___EXTRACT_FILE = $___MODEL_DIR."/extract";
$___EXTRACT_FILE = $_EXTRACT_FILE if $_EXTRACT_FILE;


my $___MAX_PHRASE_LENGTH = 7;
my $___LEXICAL_WEIGHTING = 1;
my $___LEXICAL_DIR = $___MODEL_DIR;
$___MAX_PHRASE_LENGTH = $_MAX_PHRASE_LENGTH if $_MAX_PHRASE_LENGTH;
$___LEXICAL_WEIGHTING = 0 if $_NO_LEXICAL_WEIGHTING;
$___LEXICAL_DIR = $_LEXICAL_DIR if $_LEXICAL_DIR;

my $___VERBOSE = 0;
my $___FIRST_STEP = 1;
my $___LAST_STEP = 9;
$___VERBOSE = $_VERBOSE if $_VERBOSE;
$___FIRST_STEP = $_FIRST_STEP if $_FIRST_STEP;
$___LAST_STEP =  $_LAST_STEP  if $_LAST_STEP;

my @___LM = ();
if ($___LAST_STEP == 9) {
  die "use --lm factor:order:filename to specify at least one language model"
    if scalar @_LM == 0;
  foreach my $lm (@_LM) {
    my ($f, $order, $filename) = split /:/, $lm, 3;
    die "Wrong format of --lm. Expected: --lm factor:order:filename"
      if $f !~ /^[0-9]+$/ || $order !~ /^[0-9]+$/ || !defined $filename;
    die "Language model file not found or empty: $filename"
      if ! -s $filename;
    push @___LM, [ $f, $order, $filename ];
  }
}

my $___PARTS = 1;
$___PARTS = $_PARTS if $_PARTS;

my $___DIRECTION = 0;
$___DIRECTION = $_DIRECTION if $_DIRECTION;

# don't fork
my $___NOFORK = !defined $_PARALLEL;

my $___ONLY_PRINT_GIZA = 0;
$___ONLY_PRINT_GIZA = 1 if $_ONLY_PRINT_GIZA;

# Reordering model (esp. lexicalized)
my $___REORDERING = "distance";
$___REORDERING = $_REORDERING if $_REORDERING;
my $___REORDERING_SMOOTH = 0.5;
$___REORDERING_SMOOTH = $_REORDERING_SMOOTH if $_REORDERING_SMOOTH;
my %REORDERING_MODEL;
my $REORDERING_LEXICAL = 0; # flag for building lexicalized reordering models
foreach my $r (split(/,/,$___REORDERING)) {
    if (!( $r eq "orientation-f" ||
         $r eq "orientation-fe" ||
         $r eq "orientation-bidirectional-f" ||
         $r eq "orientation-bidirectional-fe" ||
         $r eq "monotonicity-f" ||
         $r eq "monotonicity-fe" ||
         $r eq "monotonicity-bidirectional-f" ||
         $r eq "monotonicity-bidirectional-fe" ||
         $r eq "distance")) {
       print STDERR "unknwown reordering type: $r";
       exit(1);
    }
    if ($r ne "distance") { $REORDERING_LEXICAL = 1; }
    $REORDERING_MODEL{$r}++;
    if ($r =~ /-f$/) { $REORDERING_MODEL{"f"}++; }
    if ($r =~ /-fe$/) { $REORDERING_MODEL{"fe"}++; }
}
my ($mono_previous_f,$swap_previous_f,$other_previous_f);
my ($mono_previous_fe,$swap_previous_fe,$other_previous_fe);
my ($mono_following_f,$swap_following_f,$other_following_f);
my ($mono_following_fe,$swap_following_fe,$other_following_fe);
my ($f_current,$e_current);

### Factored translation models
my $___ALIGNMENT_FACTORS = "0-0";
$___ALIGNMENT_FACTORS = $_ALIGNMENT_FACTORS if defined($_ALIGNMENT_FACTORS);
die("format for alignment factors is \"0-0\" or \"0,1,2-0,1\", you provided $___ALIGNMENT_FACTORS\n") if $___ALIGNMENT_FACTORS !~ /^\d+(\,\d+)*\-\d+(\,\d+)*$/;

my $___TRANSLATION_FACTORS = undef;
$___TRANSLATION_FACTORS = $_TRANSLATION_FACTORS if defined($_TRANSLATION_FACTORS);
die("format for translation factors is \"0-0\" or \"0-0+1-1\" or \"0-0+0,1-0,1\", you provided $___TRANSLATION_FACTORS\n") 
  if defined $___TRANSLATION_FACTORS && $___TRANSLATION_FACTORS !~ /^\d+(\,\d+)*\-\d+(\,\d+)*(\+\d+(\,\d+)*\-\d+(\,\d+)*)*$/;

my $___REORDERING_FACTORS = undef;
$___REORDERING_FACTORS = $_REORDERING_FACTORS if defined($_REORDERING_FACTORS);
die("format for reordering factors is \"0-0\" or \"0-0+1-1\" or \"0-0+0,1-0,1\", you provided $___REORDERING_FACTORS\n") 
  if defined $___REORDERING_FACTORS && $___REORDERING_FACTORS !~ /^\d+(\,\d+)*\-\d+(\,\d+)*(\+\d+(\,\d+)*\-\d+(\,\d+)*)*$/;

my $___GENERATION_FACTORS = undef;
$___GENERATION_FACTORS = $_GENERATION_FACTORS if defined($_GENERATION_FACTORS);
die("format for generation factors is \"0-1\" or \"0-1+0-2\" or \"0-1+0,1-1,2\", you provided $___GENERATION_FACTORS\n") 
  if defined $___GENERATION_FACTORS && $___GENERATION_FACTORS !~ /^\d+(\,\d+)*\-\d+(\,\d+)*(\+\d+(\,\d+)*\-\d+(\,\d+)*)*$/;

my $___DECODING_STEPS = $_DECODING_STEPS;
die("use --decoding-steps to specify decoding steps") if ( !defined $_DECODING_STEPS && $___LAST_STEP>=9 && $___FIRST_STEP<=9);
die("format for decoding steps is \"t0,g0,t1,g1\", you provided $___DECODING_STEPS\n") 
  if defined $___DECODING_STEPS && $___DECODING_STEPS !~ /^[tg]\d+(,[tg]\d+)*$/;

my ($factor,$factor_e,$factor_f);

my $alignment_id;

### MAIN

&prepare()                 if $___FIRST_STEP==1;
&run_giza()                if $___FIRST_STEP<=2 && $___LAST_STEP>=2;
&word_align()              if $___FIRST_STEP<=3 && $___LAST_STEP>=3;
&get_lexical_factored()    if $___FIRST_STEP<=4 && $___LAST_STEP>=4;
&extract_phrase_factored() if $___FIRST_STEP<=5 && $___LAST_STEP>=5;
&score_phrase_factored()   if $___FIRST_STEP<=6 && $___LAST_STEP>=6;
&get_reordering_factored() if $___FIRST_STEP<=7 && $___LAST_STEP>=7;
&get_generation_factored() if $___FIRST_STEP<=8 && $___LAST_STEP>=8;
&create_ini()              if                      $___LAST_STEP==9;

### (1) PREPARE CORPUS

sub prepare {
    print STDERR "(1) preparing corpus @ ".`date`;
    safesystem("mkdir -p $___CORPUS_DIR") or die;

    print STDERR "(1.0) selecting factors @ ".`date`;
    my ($factor_f,$factor_e) = split(/\-/,$___ALIGNMENT_FACTORS);
    my $corpus = $___CORPUS.".".$___ALIGNMENT_FACTORS;    
    if ($___NOFORK) {
	&reduce_factors($___CORPUS.".".$___F,$corpus.".".$___F,$factor_f);
	&reduce_factors($___CORPUS.".".$___E,$corpus.".".$___E,$factor_e);

	&make_classes($corpus.".".$___F,$___VCB_F.".classes");
	&make_classes($corpus.".".$___E,$___VCB_E.".classes");
    
	my $VCB_F = &get_vocabulary($corpus.".".$___F,$___VCB_F);
	my $VCB_E = &get_vocabulary($corpus.".".$___E,$___VCB_E);
    
	&numberize_txt_file($VCB_F,$corpus.".".$___F,
			$VCB_E,$corpus.".".$___E,
			$___CORPUS_DIR."/$___F-$___E-int-train.snt");
    
	&numberize_txt_file($VCB_E,$corpus.".".$___E,
			$VCB_F,$corpus.".".$___F,
			$___CORPUS_DIR."/$___E-$___F-int-train.snt");
    } else {
	print "Forking...\n";
	my $pid = fork();
	die "couldn't fork" unless defined $pid;
	if (!$pid) {
	    &reduce_factors($___CORPUS.".".$___F,$corpus.".".$___F,$factor_f);
	    exit 0;
	} else {
	    &reduce_factors($___CORPUS.".".$___E,$corpus.".".$___E,$factor_e);
	}
	waitpid($pid, 0);
	my $pid2 = 0;
	$pid = fork();
	die "couldn't fork" unless defined $pid;
	if (!$pid) {
	    &make_classes($corpus.".".$___F,$___VCB_F.".classes");
	    exit 0;
	} # parent
	$pid2 = fork();
	die "couldn't fork again" unless defined $pid2;
	if (!$pid2) { #child
	    &make_classes($corpus.".".$___E,$___VCB_E.".classes");
	    exit 0;
	}
    
	my $VCB_F = &get_vocabulary($corpus.".".$___F,$___VCB_F);
	my $VCB_E = &get_vocabulary($corpus.".".$___E,$___VCB_E);
    
	&numberize_txt_file($VCB_F,$corpus.".".$___F,
			$VCB_E,$corpus.".".$___E,
			$___CORPUS_DIR."/$___F-$___E-int-train.snt");
    
	&numberize_txt_file($VCB_E,$corpus.".".$___E,
			$VCB_F,$corpus.".".$___F,
			$___CORPUS_DIR."/$___E-$___F-int-train.snt");
	waitpid($pid2, 0);
	waitpid($pid, 0);
    }
}

sub reduce_factors {
    my ($full,$reduced,$factors) = @_;
		if (-e $reduced) {
				print STDERR "already $reduced in place, reusing\n";
				return;
		}
    # my %INCLUDE;
    # foreach my $factor (split(/,/,$factors)) {
	# $INCLUDE{$factor} = 1;
    # }
    my @INCLUDE = sort {$a <=> $b} split(/,/,$factors);
    open(IN,$full) or die "Can't read $full";		
    open(OUT,">".$reduced) or die "Can't write $reduced";
    my $nr = 0;
    while(<IN>) {
        $nr++;
        print STDERR "." if $nr % 10000 == 0;
        print STDERR "($nr)" if $nr % 100000 == 0;
	chomp; s/ +/ /g; s/^ //; s/ $//;
	my $first = 1;
	foreach (split) {
	    my @FACTOR = split(/\|/);
	    print OUT " " unless $first;
	    $first = 0;
	    my $first_factor = 1;
            foreach my $outfactor (@INCLUDE) {
              print OUT "|" unless $first_factor;
              $first_factor = 0;
              my $out = $FACTOR[$outfactor];
              die "Couldn't find factor $outfactor in token \"$_\" in $full LINE $nr" if !defined $out;
              print OUT $out;
            }
	    # for(my $factor=0;$factor<=$#FACTOR;$factor++) {
		# next unless defined($INCLUDE{$factor});
		# print OUT "|" unless $first_factor;
		# $first_factor = 0;
		# print OUT $FACTOR[$factor];
	    # }
	} 
	print OUT "\n";
    }
    print STDERR "\n";
    close(OUT);
    close(IN);
}

sub make_classes {
    my ($corpus,$classes) = @_;
    my $cmd = "$MKCLS -c50 -n2 -p$corpus -V$classes opt";
    print STDERR "(1.1) running mkcls  @ ".`date`."$cmd\n";
    safesystem("$cmd"); # ignoring the wrong exit code from mkcls (not dying)
}

sub get_vocabulary {
    return unless $___LEXICAL_WEIGHTING;
    my($corpus,$vcb) = @_;
    print STDERR "(1.2) creating vcb file $vcb @ ".`date`;
    
    my %WORD;
    open(TXT,$corpus) or die "Can't read $corpus";
    while(<TXT>) {
	chop;
	foreach (split) { $WORD{$_}++; }
    }
    close(TXT);
    
    my @NUM;
    foreach my $word (keys %WORD) {
	my $vcb_with_number = sprintf("%07d %s",$WORD{$word},$word);
	push @NUM,$vcb_with_number;
    }
    
    my %VCB;
    open(VCB,">$vcb") or die "Can't write $vcb";
    print VCB "1\tUNK\t0\n";
    my $id=2;
    foreach (reverse sort @NUM) {
	my($count,$word) = split;
	printf VCB "%d\t%s\t%d\n",$id,$word,$count;
	$VCB{$word} = $id;
	$id++;
    }
    close(VCB);
    
    return \%VCB;
}

sub numberize_txt_file {
    my ($VCB_DE,$in_de,$VCB_EN,$in_en,$out) = @_;
    my %OUT;
    print STDERR "(1.3) numberizing corpus $out @ ".`date`;
    open(IN_DE,$in_de) or die "Can't read $in_de";
    open(IN_EN,$in_en) or die "Can't read $in_en";
    open(OUT,">$out") or die "Can't write $out";
    while(my $de = <IN_DE>) {
	my $en = <IN_EN>;
	print OUT "1\n";
	print OUT &numberize_line($VCB_EN,$en);
	print OUT &numberize_line($VCB_DE,$de);
    }
    close(IN_DE);
    close(IN_EN);
    close(OUT);
}

sub numberize_line {
    my ($VCB,$txt) = @_;
    chomp($txt);
    my $out = "";
    my $not_first = 0;
    foreach (split(/ /,$txt)) { 
	next if $_ eq '';
	$out .= " " if $not_first++;
	print STDERR "Unknown word '$_'\n" unless defined($$VCB{$_});
	$out .= $$VCB{$_};
    }
    return $out."\n";
}

### (2) RUN GIZA

sub run_giza {
    return &run_giza_on_parts if $___PARTS>1;

    print STDERR "(2) running giza @ ".`date`;
    if ($___DIRECTION == 1 || $___DIRECTION == 2 || $___NOFORK) {
	&run_single_giza($___GIZA_F2E,$___E,$___F,
		     $___VCB_E,$___VCB_F,
		     $___CORPUS_DIR."/$___F-$___E-int-train.snt")
	    unless $___DIRECTION == 2;
	&run_single_giza($___GIZA_E2F,$___F,$___E,
		     $___VCB_F,$___VCB_E,
		     $___CORPUS_DIR."/$___E-$___F-int-train.snt")
	    unless $___DIRECTION == 1;
    } else {
	my $pid = fork();
	if (!defined $pid) {
	    die "Failed to fork";
	}
	if (!$pid) { # i'm the child
	    &run_single_giza($___GIZA_F2E,$___E,$___F,
                     $___VCB_E,$___VCB_F,
                     $___CORPUS_DIR."/$___F-$___E-int-train.snt");
	    exit 0; # child exits
	} else { #i'm the parent
	    &run_single_giza($___GIZA_E2F,$___F,$___E,
                     $___VCB_F,$___VCB_E,
                     $___CORPUS_DIR."/$___E-$___F-int-train.snt");
	}
	printf "Waiting on second GIZA process...\n";
	waitpid($pid, 0);
    }
}

sub run_giza_on_parts {
    print STDERR "(2) running giza on $___PARTS cooc parts @ ".`date`;
    my $size = `cat $___CORPUS_DIR/$___F-$___E-int-train.snt | wc -l`;
    die "Failed to get number of lines in $___CORPUS_DIR/$___F-$___E-int-train.snt"
      if $size == 0;
    
    if ($___DIRECTION == 1 || $___DIRECTION == 2 || $___NOFORK) {
	&run_single_giza_on_parts($___GIZA_F2E,$___E,$___F,
			      $___VCB_E,$___VCB_F,
			      $___CORPUS_DIR."/$___F-$___E-int-train.snt",$size)
   	    unless $___DIRECTION == 2;
 
	&run_single_giza_on_parts($___GIZA_E2F,$___F,$___E,
			      $___VCB_F,$___VCB_E,
			      $___CORPUS_DIR."/$___E-$___F-int-train.snt",$size)
   	    unless $___DIRECTION == 1;
    } else {
	my $pid = fork();
	if (!defined $pid) {
	    die "Failed to fork";
	}
	if (!$pid) { # i'm the child
	    &run_single_giza_on_parts($___GIZA_F2E,$___E,$___F,
			      $___VCB_E,$___VCB_F,
			      $___CORPUS_DIR."/$___F-$___E-int-train.snt",$size);
	    exit 0; # child exits
	} else { #i'm the parent
	    &run_single_giza_on_parts($___GIZA_E2F,$___F,$___E,
			      $___VCB_F,$___VCB_E,
			      $___CORPUS_DIR."/$___E-$___F-int-train.snt",$size);
	}
	printf "Waiting on second GIZA process...\n";
	waitpid($pid, 0);
    }
}

sub run_single_giza_on_parts {
    my($dir,$e,$f,$vcb_e,$vcb_f,$train,$size) = @_;
    
    my $part = 0;

    # break up training data into parts
    open(SNT,$train) or die "Can't read $train";
    { 
	my $i=0;
	while(<SNT>) {
	    $i++;
	    if ($i%3==1 && $part < ($___PARTS*$i)/$size && $part<$___PARTS) {
		close(PART) if $part;
		$part++;
		safesystem("mkdir -p $___CORPUS_DIR/part$part") or die;
		open(PART,">$___CORPUS_DIR/part$part/$f-$e-int-train.snt")
                   or die "Can't write $___CORPUS_DIR/part$part/$f-$e-int-train.snt";
	    }
	    print PART $_;
	}
    }
    close(PART);
    close(SNT);

    # run snt2cooc in parts
    for(my $i=1;$i<=$___PARTS;$i++) {
	&run_single_snt2cooc("$dir/part$i",$e,$f,$vcb_e,$vcb_f,"$___CORPUS_DIR/part$i/$f-$e-int-train.snt");
    }

    # merge parts
    open(COOC,">$dir/$f-$e.cooc") or die "Can't write $dir/$f-$e.cooc";
    my(@PF,@CURRENT);
    for(my $i=1;$i<=$___PARTS;$i++) {
	open($PF[$i],"$dir/part$i/$f-$e.cooc")or die "Can't read $dir/part$i/$f-$e.cooc";
	my $pf = $PF[$i];
	$CURRENT[$i] = <$pf>;
	chop($CURRENT[$i]) if $CURRENT[$i];
    }

    while(1) {
	my ($min1,$min2) = (1e20,1e20);
	for(my $i=1;$i<=$___PARTS;$i++) {
	    next unless $CURRENT[$i];
	    my ($w1,$w2) = split(/ /,$CURRENT[$i]);
	    if ($w1 < $min1 || ($w1 == $min1 && $w2 < $min2)) {
		$min1 = $w1;
		$min2 = $w2;
	    }
	}
	last if $min1 == 1e20;
	print COOC "$min1 $min2\n";
	for(my $i=1;$i<=$___PARTS;$i++) {
	    next unless $CURRENT[$i];
	    my ($w1,$w2) = split(/ /,$CURRENT[$i]);
	    if ($w1 == $min1 && $w2 == $min2) {
		my $pf = $PF[$i];
		$CURRENT[$i] = <$pf>;
		chop($CURRENT[$i]) if $CURRENT[$i];
	    }
	}	
    }
    for(my $i=1;$i<=$___PARTS;$i++) {
	close($PF[$i]);
    }
    close(COOC);

    # run giza
    &run_single_giza($dir,$e,$f,$vcb_e,$vcb_f,$train);
}

sub run_single_giza {
    my($dir,$e,$f,$vcb_e,$vcb_f,$train) = @_;

    my %GizaDefaultOptions = 
	(p0 => .999 ,
	 m1 => 5 , 
	 m2 => 0 , 
	 m3 => 3 , 
	 m4 => 3 , 
	 o => "giza" ,
	 nodumps => 1 ,
	 onlyaldumps => 1 ,
	 nsmooth => 4 , 
         model1dumpfrequency => 1,
	 model4smoothfactor => 0.4 ,
	 t => $vcb_f,
         s => $vcb_e,
	 c => $train,
	 CoocurrenceFile => "$dir/$f-$e.cooc",
	 o => "$dir/$f-$e");

    if ($___GIZA_OPTION) {
	foreach (split(/[ ,]+/,$___GIZA_OPTION)) {
	    my ($option,$value) = split(/=/,$_,2);
	    $GizaDefaultOptions{$option} = $value;
	}
    }

    my $GizaOptions;
    foreach my $option (sort keys %GizaDefaultOptions){
	my $value = $GizaDefaultOptions{$option} ;
	$GizaOptions .= " -$option $value" ;
    }
    
    &run_single_snt2cooc($dir,$e,$f,$vcb_e,$vcb_f,$train) if $___PARTS == 1;

    print STDERR "(2.1b) running giza $f-$e @ ".`date`."$GIZA $GizaOptions\n";
    print "$GIZA $GizaOptions\n";
    return if  $___ONLY_PRINT_GIZA;
    safesystem("$GIZA $GizaOptions");
    die "Giza did not produce the output file $dir/$f-$e.A3.final. Is your corpus clean (reasonably-sized sentences)?"
      if ! -e "$dir/$f-$e.A3.final";
    safesystem("rm -f $dir/$f-$e.A3.final.gz") or die;
    safesystem("gzip $dir/$f-$e.A3.final") or die;
}

sub run_single_snt2cooc {
    my($dir,$e,$f,$vcb_e,$vcb_f,$train) = @_;
    print STDERR "(2.1a) running snt2cooc $f-$e @ ".`date`."\n";
    safesystem("mkdir -p $dir") or die;
    print "$SNT2COOC $vcb_e $vcb_f $train > $dir/$f-$e.cooc\n";
    safesystem("$SNT2COOC $vcb_e $vcb_f $train > $dir/$f-$e.cooc") or die;
}

### (3) CREATE WORD ALIGNMENT FROM GIZA ALIGNMENTS

sub word_align {
    print STDERR "(3) generate word alignment @ ".`date`;
    my (%WORD_TRANSLATION,%TOTAL_FOREIGN,%TOTAL_ENGLISH);
    print STDERR "Combining forward and inverted alignment from files:\n";
    print STDERR "  $___GIZA_F2E/$___F-$___E.A3.final.{bz2,gz}\n";
    print STDERR "  $___GIZA_E2F/$___F-$___E.A3.final.{bz2,gz}\n";
    if (-e "$___GIZA_F2E/$___F-$___E.A3.final.bz2") {
	open(ALIGNMENT,    "$BZCAT $___GIZA_F2E/$___F-$___E.A3.final.bz2|")
          or die "Can't $BZCAT $___GIZA_F2E/$___F-$___E.A3.final.bz2";
	open(ALIGNMENT_INV,"$BZCAT $___GIZA_E2F/$___E-$___F.A3.final.bz2|")
          or die "Can't $BZCAT $___GIZA_E2F/$___E-$___F.A3.final.bz2";
    }
    else {
        die "Can't find $___GIZA_F2E/$___F-$___E.A3.final.gz"
          if ! -e "$___GIZA_F2E/$___F-$___E.A3.final.gz";
	open(ALIGNMENT,    "$ZCAT $___GIZA_F2E/$___F-$___E.A3.final.gz|")
          or die "Can't $ZCAT $___GIZA_F2E/$___F-$___E.A3.final.gz";
        die "Can't find $___GIZA_E2F/$___E-$___F.A3.final.gz"
          if ! -e "$___GIZA_E2F/$___E-$___F.A3.final.gz";
	open(ALIGNMENT_INV,"$ZCAT $___GIZA_E2F/$___E-$___F.A3.final.gz|")
          or die "Can't $ZCAT $___GIZA_E2F/$___E-$___F.A3.final.gz";
    }
    safesystem("mkdir -p $___MODEL_DIR") or die;
    open(E,">$___MODEL_DIR/aligned.$___E")
      or die "Can't write $___MODEL_DIR/aligned.$___E";
    open(F,">$___MODEL_DIR/aligned.$___F")
      or die "Can't write $___MODEL_DIR/aligned.$___F";
    open(A,">$___MODEL_DIR/aligned.$___ALIGNMENT")
      or die "Cant' write $___MODEL_DIR/aligned.$___ALIGNMENT";
    for(my $id=0;;$id++) {    
	if (($id % 100) == 0) { print STDERR "."; }
	my ($status,$ALIGNMENT,$ENGLISH,$FOREIGN) 
	    = &get_alignment($id,\*ALIGNMENT,\*ALIGNMENT_INV);
        last if $status == -1;
	if ($status == 0 && $___NOTE_ALIGNMENT_DROPS) {
	    print E "\n";
	    print F "\n";
	    print A "\n";
	}
	next if $status == 0;
	
	for(my $fi=0;$fi<=$#$FOREIGN;$fi++) {
	    print F " " if $fi;
	    print F $$FOREIGN[$fi];
	}
	print F "\n";
	
	for(my $ei=0;$ei<=$#$ENGLISH;$ei++) {
	    print E " " if $ei;
	    print E $$ENGLISH[$ei];
	}
	print E "\n";
	
	my $count=0;
	for(my $fi=0;$fi<=$#$FOREIGN;$fi++) {     
	    next if !defined($$ALIGNMENT[$fi]);
	    for(my $ei=0;$ei<=$#$ENGLISH;$ei++) {
		next unless $$ALIGNMENT[$fi][$ei] == 2;
		print A " " if $count++;
		print A "$fi-$ei";
	    }
	}
	print A "\n";
    }
    close(ALIGNMENT_INV);
    close(ALIGNMENT);
    print STDERR "\n";
}

sub get_alignment {
    my ($id,$AFILE,$AFILE_INV) = @_;
    my $ALIGNMENT;
    
    # read alignments
    my ($ok, $F2E,$ENGLISH, $FOREIGN,$debug)  = &load_alignment_giza($AFILE,0);
    return -1 unless $ok; # signal end of file
    my ($ok2,$E2F,$FOREIGN2,$ENGLISH2,$debug2) = &load_alignment_giza($AFILE_INV,1);
    die "Inverted alignment file was shorter than the forward one!" unless $ok2;
    
    # sanity checks
    if ($#$ENGLISH != $#$ENGLISH2 || $#$FOREIGN != $#$FOREIGN2) {
	die "line $id: different number of words when combining the two directions of alignments ($#$ENGLISH != $#$ENGLISH2 || $#$FOREIGN != $#$FOREIGN2)\nThe reason is typically sentences over 100 words or whatever you said using gizaoption words in corpus (GIZA chops such sentences.).";
    }
    
    if ($___ALIGNMENT eq 'union') {
	$ALIGNMENT = &union($F2E,$E2F);
	print "UNION:\n" if $___VERBOSE >= 2;
	&print_alignment($ALIGNMENT) if $___VERBOSE >= 2;
    }
    else {
	print "INTERSECT:\n" if $___VERBOSE >= 2;
	$ALIGNMENT = &intersect($F2E,$E2F);
	&print_alignment($ALIGNMENT) if $___VERBOSE >= 2;
	if ($___ALIGNMENT eq 'e2f' || $___ALIGNMENT eq 'f2e') {
	    print "RELY ON MONO:\n" if $___VERBOSE >= 2;
	    my $one = 1.1;
	    $one = 1.2 if $___ALIGNMENT eq 'e2f';
	    for(my $ei=0;$ei<=$#{$$E2F[0]};$ei++) {
		for(my $fi=0;$fi<=$#$E2F;$fi++) {
		    $$ALIGNMENT[$fi][$ei] = 2 if $$ALIGNMENT[$fi][$ei] == $one;
		}
	    }
	    &print_alignment($ALIGNMENT) if $___VERBOSE >= 2;
	}
	elsif ($___ALIGNMENT =~ /grow/) {
	    print "GROW:\n" if $___VERBOSE >= 2;
	    if ($___ALIGNMENT =~ /grow12/) {
		while(&grow($ALIGNMENT,1.1,$___ALIGNMENT =~ /diag/)) {};
		while(&grow($ALIGNMENT,1.2,$___ALIGNMENT =~ /diag/)) {};
	    }
	    elsif ($___ALIGNMENT =~ /grow21/) {
		while(&grow($ALIGNMENT,1.2,$___ALIGNMENT =~ /diag/)) {};
		while(&grow($ALIGNMENT,1.1,$___ALIGNMENT =~ /diag/)) {};
	    }
	    else {
		while(&grow($ALIGNMENT,1,  $___ALIGNMENT =~ /diag/)) {};
	    }
	    &print_alignment($ALIGNMENT) if $___VERBOSE >= 2;
	    if ($___ALIGNMENT =~ /final/) {
		print "FINAL:\n" if $___VERBOSE >= 2;
		&grow_final($ALIGNMENT,1.1,$___ALIGNMENT =~ /final-and/) 
		    if $___ALIGNMENT =~ /12/;
		&grow_final($ALIGNMENT,1.2,$___ALIGNMENT =~ /final-and/);
		&grow_final($ALIGNMENT,1.1,$___ALIGNMENT =~ /final-and/) 
		    unless $___ALIGNMENT =~ /12/;
		&print_alignment($ALIGNMENT) if $___VERBOSE >= 2;
	    }
	}
    }
    return (1,$ALIGNMENT,$ENGLISH,$FOREIGN);
}

sub load_alignment_giza {
    my($FP,$inv_flag) = @_;
    
    # comment line
    my $dummy   = <$FP>;
    return 0 unless $dummy;  # end of file
    
    # foreign line
    my $foreign = <$FP>;
    chop($foreign);
    my @F = split(/ /,$foreign);
    
    # english line
    my $english_with_alignment = <$FP>;
    print $english_with_alignment if $___VERBOSE >= 2;
    my @ETOK = split(/\}\)/,$english_with_alignment);
    
    # extract alignment
    my @A = ();
    my @E = ();
    for(my $e=1;$e<$#ETOK;$e++) {
	my @ALIGN = split(/ /,$ETOK[$e]);
	push @E,$ALIGN[1];
	# init with 0
	for(my $f=0;$f<=$#F;$f++) {
	    $A[$f][$e-1] = 0 if !$inv_flag;
	    $A[$e-1][$f] = 0 if  $inv_flag;      
	}
	# scanning through 'the ({ 1 12'
	for(my $a=3; $a<=$#ALIGN; $a++) {
	    $A[$ALIGN[$a]-1][$e-1] = 1 if !$inv_flag;
	    $A[$e-1][$ALIGN[$a]-1] = 1 if  $inv_flag;
	}
    }
    
    return (1,\@A,\@E,\@F,$english_with_alignment.$foreign."/n");
}

sub intersect {
    my($F2E,$E2F) = @_;
    my @A = ();
    for(my $ei=0;$ei<=$#{$$E2F[0]};$ei++) {
	for(my $fi=0;$fi<=$#$E2F;$fi++) {
	    if ($$F2E[$fi][$ei] && $$E2F[$fi][$ei]) {
		$A[$fi][$ei] = 2;
	    }
	    elsif ($$F2E[$fi][$ei]) {
		$A[$fi][$ei] = 1.1;
	    }
	    elsif ($$E2F[$fi][$ei]) {
		$A[$fi][$ei] = 1.2;
	    }
	    else {
		$A[$fi][$ei] = 0;
	    }
	}
    }
    return \@A;
}

sub union {
    my($F2E,$E2F) = @_;
    my @A = ();
    for(my $ei=0;$ei<=$#{$$E2F[0]};$ei++) {
	for(my $fi=0;$fi<=$#$E2F;$fi++) {
	    $A[$fi][$ei] = ($$F2E[$fi][$ei] || $$E2F[$fi][$ei]) ? 2 : 0;
	}
    }
    return \@A;
}

sub grow {
    my($A,$one,$diagonal) = @_;
    my @OFFSET;
    { my @P = (-1, 0); push @OFFSET,\@P; }
    { my @P = ( 0,-1); push @OFFSET,\@P; }
    { my @P = ( 1, 0); push @OFFSET,\@P; }
    { my @P = ( 0, 1); push @OFFSET,\@P; }
    if ($diagonal) {
	{ my @P = (-1,-1); push @OFFSET,\@P; }
	{ my @P = (-1, 1); push @OFFSET,\@P; }
	{ my @P = ( 1,-1); push @OFFSET,\@P; }
	{ my @P = ( 1, 1); push @OFFSET,\@P; }
    }
    my $added=0;
    $one = 1 unless $one;
    my ($FOREIGN_ALIGNED,$ENGLISH_ALIGNED) = &count_aligned($A);
    for(my $ei=0;$ei<=$#{$$A[0]};$ei++) {
	for(my $fi=0;$fi<=$#$A;$fi++) {
	    next unless $$A[$fi][$ei] == 2;
	    foreach my $P (@OFFSET) {
		my ($offset_e,$offset_f) = @{$P};
		next if $fi+$offset_f<0 || $fi+$offset_f>$#$A;
		next if $ei+$offset_e<0 || $ei+$offset_e>$#{$$A[0]};
		next unless (($one==1 && int($$A[$fi+$offset_f][$ei+$offset_e]) == 1)
			     || $$A[$fi+$offset_f][$ei+$offset_e] == $one);
		next if ($$FOREIGN_ALIGNED{$fi+$offset_f} && 
			 $$ENGLISH_ALIGNED{$ei+$offset_e});
		$$A[$fi+$offset_f][$ei+$offset_e] = 2;
		$$FOREIGN_ALIGNED{$fi+$offset_f}++;
		$$ENGLISH_ALIGNED{$ei+$offset_e}++;
		$added++;
	    }
	}
    }
    return $added;
}

sub grow_final {
    my($A,$one,$and) = @_;
    my $added = 0;
    my ($FOREIGN_ALIGNED,$ENGLISH_ALIGNED) = &count_aligned($A);
    for(my $ei=0;$ei<=$#{$$A[0]};$ei++) {
	for(my $fi=0;$fi<=$#$A;$fi++) {
	    next unless $$A[$fi][$ei] == $one;
	    next if (!$and && $$FOREIGN_ALIGNED{$fi} && $$ENGLISH_ALIGNED{$ei});
	    next if ($and && ($$FOREIGN_ALIGNED{$fi} || $$ENGLISH_ALIGNED{$ei}));
	    $$A[$fi][$ei] = 2;
	    $$FOREIGN_ALIGNED{$fi}++;
	    $$ENGLISH_ALIGNED{$ei}++;
	}
    }
    return $added;
}

sub print_alignment {
    my($A) = @_;
    for(my $ei=0;$ei<=$#{$$A[0]};$ei++) {
	for(my $fi=0;$fi<=$#$A;$fi++) {
	    if($$A[$fi][$ei] == 2) {
		print "#";
	    }
	    elsif ($$A[$fi][$ei] == 1.1) {
		print "-";
	    }
	    elsif ($$A[$fi][$ei] == 1.2) {
		print "|";
	    }
	    else {
		print ".";
	    }
	}
	print "\n";
    }
}

sub count_aligned {
    my ($A) = @_;
    my (%FOREIGN_ALIGNED,%ENGLISH_ALIGNED);
    for(my $ei=0;$ei<=$#{$$A[0]};$ei++) {
	for(my $fi=0;$fi<=$#$A;$fi++) {
	    next unless $$A[$fi][$ei] == 2;
	    $FOREIGN_ALIGNED{$fi}++;
	    $ENGLISH_ALIGNED{$ei}++;
	}
    }
    return (\%FOREIGN_ALIGNED,\%ENGLISH_ALIGNED);
}

### (4) BUILDING LEXICAL TRANSLATION TABLE

sub get_lexical_factored {
    print STDERR "(4) generate lexical translation table $___TRANSLATION_FACTORS @ ".`date`;
    foreach my $f (split(/\+/,$___TRANSLATION_FACTORS)) {
	$factor = $f;
	($factor_f,$factor_e) = split(/\-/,$factor);
	&reduce_factors($___CORPUS.".".$___F,
			$___MODEL_DIR."/aligned.".$factor_f.".".$___F,
			$factor_f);
	&reduce_factors($___CORPUS.".".$___E,
			$___MODEL_DIR."/aligned.".$factor_e.".".$___E,
			$factor_e);
	&get_lexical();
    }
}

sub get_lexical {
    print STDERR "(4) [$factor] generate lexical translation table @ ".`date`;
		my (%WORD_TRANSLATION,%TOTAL_FOREIGN,%TOTAL_ENGLISH);

		&open_alignment();
    while(my $e = <E>) {
        if (($alignment_id++ % 1000) == 0) { print STDERR "!"; }
        chomp($e);
        my @ENGLISH = split(/ /,$e);
        my $f = <F>; chomp($f);
        my @FOREIGN = split(/ /,$f);
        my $a = <A>; chomp($a);

        my (%FOREIGN_ALIGNED,%ENGLISH_ALIGNED);
        foreach (split(/ /,$a)) {
            my ($fi,$ei) = split(/\-/);
						if ($fi >= scalar(@FOREIGN) || $ei >= scalar(@ENGLISH)) {
								print STDERR "alignment point ($fi,$ei) out of range (0-$#FOREIGN,0-$#ENGLISH) in line $alignment_id, ignoring\n";
						}
						else {
								# local counts
								$FOREIGN_ALIGNED{$fi}++;
								$ENGLISH_ALIGNED{$ei}++;
								
								# global counts
								$WORD_TRANSLATION{$FOREIGN[$fi]}{$ENGLISH[$ei]}++;
								$TOTAL_FOREIGN{$FOREIGN[$fi]}++;
								$TOTAL_ENGLISH{$ENGLISH[$ei]}++;
						}
        }

        # unaligned words
        for(my $ei=0;$ei<scalar(@ENGLISH);$ei++) {
          next if defined($ENGLISH_ALIGNED{$ei});
          $WORD_TRANSLATION{"NULL"}{$ENGLISH[$ei]}++;
          $TOTAL_ENGLISH{$ENGLISH[$ei]}++;
          $TOTAL_FOREIGN{"NULL"}++;
        }
        for(my $fi=0;$fi<scalar(@FOREIGN);$fi++) {
          next if defined($FOREIGN_ALIGNED{$fi});
          $WORD_TRANSLATION{$FOREIGN[$fi]}{"NULL"}++;
          $TOTAL_FOREIGN{$FOREIGN[$fi]}++;
          $TOTAL_ENGLISH{"NULL"}++;
        }
    }
		&close_alignment();
    &save_word_translation(\%WORD_TRANSLATION,\%TOTAL_FOREIGN,\%TOTAL_ENGLISH);
}

sub open_alignment {
    open(E,"$___MODEL_DIR/aligned.$factor_e.$___E")
      or die "Can't read $___MODEL_DIR/aligned.$factor_e.$___E";
    open(F,"$___MODEL_DIR/aligned.$factor_f.$___F")
      or die "Can't read $___MODEL_DIR/aligned.$factor_f.$___F";
    open(A,"$___MODEL_DIR/aligned.$___ALIGNMENT")
      or die "Can't read $___MODEL_DIR/aligned.$___ALIGNMENT";
    $alignment_id=0;
}

sub close_alignment {
    print STDERR "\n";
    close(A);
    close(F);
    close(E);
}

sub save_word_translation {
    my ($WORD_TRANSLATION,$TOTAL_FOREIGN,$TOTAL_ENGLISH) = @_;
    safesystem("mkdir -p $___LEXICAL_DIR") or die;
    open(F2E,">$___LEXICAL_DIR/lex.$factor.f2n")
      or die "Can't write $___LEXICAL_DIR/lex.$factor.f2n";
    open(E2F,">$___LEXICAL_DIR/lex.$factor.n2f")
      or die "Can't write $___LEXICAL_DIR/lex.$factor.n2f";
    foreach my $f (keys %{$WORD_TRANSLATION}) {
	foreach my $e (keys %{$$WORD_TRANSLATION{$f}}) {
	    printf F2E "%s %s %.7f\n",$e,$f,$$WORD_TRANSLATION{$f}{$e}/$$TOTAL_FOREIGN{$f};
	    printf E2F "%s %s %.7f\n",$f,$e,$$WORD_TRANSLATION{$f}{$e}/$$TOTAL_ENGLISH{$e};
	}
    }
    close(E2F);
    close(F2E);
    print STDERR "Saved: $___LEXICAL_DIR/lex.$factor.f2n and $___LEXICAL_DIR/lex.$factor.n2f\n";
}

### (5) PHRASE EXTRACTION

sub extract_phrase_factored {
    print STDERR "(5) extract phrases @ ".`date`;
    my %generated;
    foreach my $f (split(/\+/,"$___TRANSLATION_FACTORS"
                     .($REORDERING_LEXICAL ? "+$___REORDERING_FACTORS" : ""))) {
        # we extract phrases for all translation steps and also for reordering factors (if lexicalized reordering is used)
        next if $generated{$f};
        $generated{$f} = 1;
	$factor = $f;
	($factor_f,$factor_e) = split(/\-/,$factor);
	&extract_phrase();
    }
}

sub extract_phrase {
    print STDERR "(5) [$factor] extract phrases @ ".`date`;
    my $cmd = "$PHRASE_EXTRACT $___MODEL_DIR/aligned.$factor_e.$___E $___MODEL_DIR/aligned.$factor_f.$___F $___MODEL_DIR/aligned.$___ALIGNMENT $___EXTRACT_FILE.$factor $___MAX_PHRASE_LENGTH orientation";
    print STDERR "$cmd\n";
    safesystem("$cmd") or die "Phrase extraction failed (missing input files?)";
    safesystem("cat $___EXTRACT_FILE.$factor.o.part* > $___EXTRACT_FILE.$factor.o") or die;
    safesystem("rm -f $___EXTRACT_FILE.$factor.o.gz") or die;
    safesystem("gzip $___EXTRACT_FILE.$factor.o") or die;
    safesystem("rm -f $___EXTRACT_FILE.$factor.o.part*") or die;
    safesystem("cat $___EXTRACT_FILE.$factor.part* > $___EXTRACT_FILE.$factor") or die;
    safesystem("rm -f $___EXTRACT_FILE.$factor.part*") or die;
    safesystem("cat $___EXTRACT_FILE.$factor.inv.part* > $___EXTRACT_FILE.$factor.inv") or die;
    safesystem("rm -f $___EXTRACT_FILE.$factor.inv.part*") or die;
}

### (6) PHRASE SCORING

sub score_phrase_factored {
    print STDERR "(6) score phrases @ ".`date`;
    foreach my $f (split(/\+/,$___TRANSLATION_FACTORS)) {
	$factor = $f;
	($factor_f,$factor_e) = split(/\-/,$factor);
	&score_phrase();
    }
}

sub score_phrase {
    print STDERR "(6) [$factor] score phrases @ ".`date`;
    if (-e "$___EXTRACT_FILE.$factor.gz") {
      safesystem("gunzip < $___EXTRACT_FILE.$factor.gz > $___EXTRACT_FILE.$factor") or die;
    }
    if (-e "$___EXTRACT_FILE.$factor.inv.gz") {
      safesystem("gunzip < $___EXTRACT_FILE.$factor.inv.gz > $___EXTRACT_FILE.$factor.inv") or die;
    }
    print STDERR "(6.1) [$factor]  sorting @ ".`date`;
    # print "LC_ALL=C sort -T $___MODEL_DIR $___EXTRACT_FILE.$factor > $___EXTRACT_FILE.$factor.sorted\n";
    safesystem("LC_ALL=C sort -T $___MODEL_DIR $___EXTRACT_FILE.$factor > $___EXTRACT_FILE.$factor.sorted") or die;
    safesystem("rm -f $___EXTRACT_FILE.$factor.gz") or die;
    safesystem("gzip $___EXTRACT_FILE.$factor") or die;
    print STDERR "(6.2) [$factor]  sorting inv @ ".`date`;
    # print "LC_ALL=C sort -T $___MODEL_DIR $___EXTRACT_FILE.$factor.inv > $___EXTRACT_FILE.$factor.inv.sorted\n";
    safesystem("LC_ALL=C sort -T $___MODEL_DIR $___EXTRACT_FILE.$factor.inv > $___EXTRACT_FILE.$factor.inv.sorted") or die;
    safesystem("rm -f $___EXTRACT_FILE.$factor.inv.gz") or die;
    safesystem("gzip $___EXTRACT_FILE.$factor.inv") or die;

    for my $direction ("f2n","n2f") {
	print STDERR "(6.3) [$factor]  creating table half $___MODEL_DIR/phrase-table-half.$factor.$direction @ ".`date`;
	my $extract = "$___EXTRACT_FILE.$factor.sorted";
	$extract = "$___EXTRACT_FILE.$factor.inv.sorted" if $direction eq "n2f";
	my $inverse = "";
	$inverse = " inverse" if $direction eq "n2f";
	my $part_count = &split_extract($extract);
	for(my $i=0;$i<$part_count;$i++) {
	    my $part = sprintf("%04d",$i);
	    print "$PHRASE_SCORE $extract.part$part $___LEXICAL_DIR/lex.$factor.$direction $___MODEL_DIR/phrase-table-half.$factor.$direction.part$part $inverse\n";
	    safesystem("$PHRASE_SCORE $extract.part$part $___LEXICAL_DIR/lex.$factor.$direction $___MODEL_DIR/phrase-table-half.$factor.$direction.part$part $inverse")
              or die "Scoring of phrases failed";
	    safesystem("rm $extract.part$part") or die;
	}
	safesystem("cat $___MODEL_DIR/phrase-table-half.$factor.$direction.part* >$___MODEL_DIR/phrase-table-half.$factor.$direction") or die;
    }
    print STDERR "(6.4) [$factor]  sorting inverse n2f table@ ".`date`;
    print "LC_ALL=C sort -T $___MODEL_DIR $___MODEL_DIR/phrase-table-half.$factor.n2f > $___MODEL_DIR/phrase-table-half.$factor.n2f inverse.sorted\n";
    safesystem("LC_ALL=C sort -T $___MODEL_DIR $___MODEL_DIR/phrase-table-half.$factor.n2f > $___MODEL_DIR/phrase-table-half.$factor.n2f.sorted") or die;
    print STDERR "(6.5) [$factor]  consolidating the two halves @ ".`date`;
    open(F2N,"$___MODEL_DIR/phrase-table-half.$factor.f2n")
      or die "Can't read $___MODEL_DIR/phrase-table-half.$factor.f2n";
    open(N2F,"$___MODEL_DIR/phrase-table-half.$factor.n2f.sorted")
      or die "Can't read $___MODEL_DIR/phrase-table-half.$factor.n2f.sorted";
    open(TABLE,">$___MODEL_DIR/phrase-table.$factor")
      or die "Can't write $___MODEL_DIR/phrase-table.$factor";
    my $i=0;
    while(my $f2n = <F2N>) {
	$i++;
	my $n2f = <N2F>;
	my ($english,$foreign,$p) = split(/ \|\|\| /,$n2f); chop($p);
	my ($english2,$foreign2,$p2) = split(/ \|\|\| /,$f2n); chop($p2);
	if ($english ne $english2 || $foreign ne $foreign2) {
	    print STDERR "mismatch line $i: ($english ne $english2 || $foreign ne $foreign2)\n";
	    next;
	}
	print TABLE "$english ||| $foreign ||| $p $p2 2.718\n";
    }
    close(N2F);
    close(F2N);
    safesystem("rm -f $___MODEL_DIR/phrase-table-half.$factor.*") or die;
    safesystem("rm -f $___MODEL_DIR/extract*sorted*") or die;
    safesystem("rm -f $___MODEL_DIR/phrase-table.$factor.gz") or die;
    safesystem("gzip $___MODEL_DIR/phrase-table.$factor") or die;
}

sub split_extract {
    my ($file) = @_;
    my $i=0;
    my $part = 1;
    my $split_when_possible = 0;
    my ($first,$dummy);
    my $partfname = sprintf("%s.part%04d",$file,0);
    open(PART,">$partfname") or die "Can't write $partfname";
    open(EXTRACT,$file) or die "Can't read $file";
    while(<EXTRACT>) {
	if ($i>0 && $i % 10000000 == 0) {
	    $split_when_possible = 1;
	    ($first,$dummy) = split(/ \|\|\| /);
	}
	elsif ($split_when_possible) {
	    my ($f,$dummy) = split(/ \|\|\| /);
	    if ($f ne $first) {
		close(PART) if $i;
                my $partfname = sprintf("%s.part%04d",$file,$part);
		open(PART,">$partfname") or die "Can't write $partfname";
		$split_when_possible = 0;
		$part++;
	    }
	}
	print PART $_;
	$i++;
    }
    close(EXTRACT);
    return $part;
}

### (7) LEARN REORDERING MODEL

sub get_reordering_factored {
    print STDERR "(7) learn reordering model @ ".`date`;
    if ($REORDERING_LEXICAL) {
      foreach my $f (split(/\+/,$___REORDERING_FACTORS)) {
	  $factor = $f;
	  ($factor_f,$factor_e) = split(/\-/,$factor);
	  &get_reordering();
      }
    } else {
      print STDERR "  ... skipping this step, reordering is not lexicalized ...\n";
    }
}

sub get_reordering {
    print STDERR "(7) [$factor] learn reordering model @ ".`date`;
    print STDERR "(7.1) [$factor]  sorting extract.o @ ".`date`;
    if (-e "$___EXTRACT_FILE.$factor.o.gz") {
      safesystem("gunzip $___EXTRACT_FILE.$factor.o.gz") or die;
    }
    # print "LC_ALL=C sort -T $___MODEL_DIR $___EXTRACT_FILE.$factor.o > $___EXTRACT_FILE.$factor.o.sorted\n";
    safesystem("LC_ALL=C sort -T $___MODEL_DIR $___EXTRACT_FILE.$factor.o > $___EXTRACT_FILE.$factor.o.sorted") or die;
    safesystem("rm -f $___EXTRACT_FILE.$factor.o.gz") or die;
    safesystem("gzip $___EXTRACT_FILE.$factor.o") or die;

    my $smooth = $___REORDERING_SMOOTH;
    my @REORDERING_SMOOTH_PREVIOUS = ($smooth,$smooth,$smooth);
    my @REORDERING_SMOOTH_FOLLOWING = ($smooth,$smooth,$smooth);

    my (%SMOOTH_PREVIOUS,%SMOOTH_FOLLOWING);
    if ($smooth =~ /(.+)u$/) {
	$smooth = $1;
	my $smooth_total = 0; 
	open(O,"$___EXTRACT_FILE.$factor.o.sorted")
          or die "Can't read $___EXTRACT_FILE.$factor.o.sorted";
	while(<O>) {
	    chomp;
	    my ($f,$e,$o) = split(/ \|\|\| /);
	    my ($o_previous,$o_following) = split(/ /,$o);
	    $SMOOTH_PREVIOUS{$o_previous}++;
	    $SMOOTH_FOLLOWING{$o_following}++;
	    $smooth_total++;
	}
	close(O);
	@REORDERING_SMOOTH_PREVIOUS = ($smooth*($SMOOTH_PREVIOUS{"mono"}+0.1)/$smooth_total,
				       $smooth*($SMOOTH_PREVIOUS{"swap"}+0.1)/$smooth_total,
				       $smooth*($SMOOTH_PREVIOUS{"other"}+0.1)/$smooth_total);
	@REORDERING_SMOOTH_FOLLOWING = ($smooth*($SMOOTH_FOLLOWING{"mono"}+0.1)/$smooth_total,
					$smooth*($SMOOTH_FOLLOWING{"swap"}+0.1)/$smooth_total,
					$smooth*($SMOOTH_FOLLOWING{"other"}+0.1)/$smooth_total);
	printf "$smooth*($SMOOTH_FOLLOWING{mono}+0.1)/$smooth_total,
					$smooth*($SMOOTH_FOLLOWING{swap}+0.1)/$smooth_total,
					$smooth*($SMOOTH_FOLLOWING{other}+0.1)/$smooth_total\n";
	printf "smoothed following to %f,%f,%f\n",@REORDERING_SMOOTH_FOLLOWING;
    }
    
    ($mono_previous_f,$swap_previous_f,$other_previous_f) = @REORDERING_SMOOTH_PREVIOUS;
    ($mono_previous_fe,$swap_previous_fe,$other_previous_fe) = @REORDERING_SMOOTH_PREVIOUS;
    ($mono_following_f,$swap_following_f,$other_following_f) = @REORDERING_SMOOTH_FOLLOWING;
    ($mono_following_fe,$swap_following_fe,$other_following_fe) = @REORDERING_SMOOTH_FOLLOWING;

    print STDERR "(7.2) building tables @ ".`date`;
    open(O,"$___EXTRACT_FILE.$factor.o.sorted")
      or die "Can't read $___EXTRACT_FILE.$factor.o.sorted";
    open(OF,  "|gzip >$___MODEL_DIR/orientation-table.$factor.f.$___REORDERING_SMOOTH.gz") 
	if defined($REORDERING_MODEL{"orientation-f"});
    open(OFE, "|gzip >$___MODEL_DIR/orientation-table.$factor.fe.$___REORDERING_SMOOTH.gz") 
	if defined($REORDERING_MODEL{"orientation-fe"});
    open(OBF, "|gzip >$___MODEL_DIR/orientation-table.$factor.bi.f.$___REORDERING_SMOOTH.gz") 
	if defined($REORDERING_MODEL{"orientation-bidirectional-f"});
    open(OBFE,"|gzip >$___MODEL_DIR/orientation-table.$factor.bi.fe.$___REORDERING_SMOOTH.gz") 
	if defined($REORDERING_MODEL{"orientation-bidirectional-fe"});
    open(MF,  "|gzip >$___MODEL_DIR/monotonicity-table.$factor.f.$___REORDERING_SMOOTH.gz") 
	if defined($REORDERING_MODEL{"monotonicity-f"});
    open(MFE, "|gzip >$___MODEL_DIR/monotonicity-table.$factor.fe.$___REORDERING_SMOOTH.gz") 
	if defined($REORDERING_MODEL{"monotonicity-fe"});
    open(MBF, "|gzip >$___MODEL_DIR/monotonicity-table.$factor.bi.f.$___REORDERING_SMOOTH.gz") 
	if defined($REORDERING_MODEL{"monotonicity-bidirectional-f"});
    open(MBFE,"|gzip >$___MODEL_DIR/monotonicity-table.$factor.bi.fe.$___REORDERING_SMOOTH.gz") 
	if defined($REORDERING_MODEL{"monotonicity-bidirectional-fe"});

    my $first = 1;
    while(<O>) {
	chomp;
	my ($f,$e,$o) = split(/ \|\|\| /);
	my ($o_previous,$o_following) = split(/ /,$o);
	
	# store counts if new f,e
	if ($first) {
	    $f_current = $f;
	    $e_current = $e;
	    $first = 0;
	}
	elsif ($f ne $f_current || $e ne $e_current) {
	    
	    if (defined($REORDERING_MODEL{"fe"})) {
		# compute probs, store them
		&store_reordering_fe();
		
		# reset counters
		($mono_previous_fe,$swap_previous_fe,$other_previous_fe) = @REORDERING_SMOOTH_PREVIOUS;
		($mono_following_fe,$swap_following_fe,$other_following_fe) = @REORDERING_SMOOTH_FOLLOWING;
	    }

	    # store counts if new f
	    if ($f ne $f_current && defined($REORDERING_MODEL{"f"})) {
		
		# compute probs, store them
		&store_reordering_f();
		
		# reset counters
		($mono_previous_f,$swap_previous_f,$other_previous_f) = @REORDERING_SMOOTH_PREVIOUS;
		($mono_following_f,$swap_following_f,$other_following_f) = @REORDERING_SMOOTH_FOLLOWING;
		
	    }
	    $f_current = $f;
	    $e_current = $e;
	}	
	# update counts
	if    ($o_previous eq 'mono') {  $mono_previous_f++;  $mono_previous_fe++; }
	elsif ($o_previous eq 'swap') {  $swap_previous_f++;  $swap_previous_fe++; }
	elsif ($o_previous eq 'other'){ $other_previous_f++; $other_previous_fe++; }
	else { print STDERR "buggy line (o_previous:$o_previous): $_\n"; }
	
	if    ($o_following eq 'mono') {  $mono_following_f++;  $mono_following_fe++; }
	elsif ($o_following eq 'swap') {  $swap_following_f++;  $swap_following_fe++; }
	elsif ($o_following eq 'other'){ $other_following_f++; $other_following_fe++; }
	else { print STDERR "buggy line (o_following:$o_following): $_\n"; }

    }
    if (defined($REORDERING_MODEL{"f"})) {
	&store_reordering_f();
    }
    if (defined($REORDERING_MODEL{"fe"})) {
	&store_reordering_fe();
    }
    safesystem("rm $___EXTRACT_FILE.$factor.o.sorted") or die;
}

sub store_reordering_f {
    my $total_previous_f = $mono_previous_f+$swap_previous_f+$other_previous_f;
    my $total_following_f = $mono_following_f+$swap_following_f+$other_following_f;
    if(defined($REORDERING_MODEL{"orientation-f"})) {
 	printf OF ("%s ||| %.5f %.5f %.5f\n",
		   $f_current, 
		   $mono_previous_f/$total_previous_f,
		   $swap_previous_f/$total_previous_f,
		   $other_previous_f/$total_previous_f);
    }
    if(defined($REORDERING_MODEL{"orientation-bidirectional-f"})) {
	printf OBF ("%s ||| %.5f %.5f %.5f %.5f %.5f %.5f\n",
		    $f_current, 
		    $mono_previous_f/$total_previous_f,
		    $swap_previous_f/$total_previous_f,
		    $other_previous_f/$total_previous_f,
		    $mono_following_f/$total_following_f,
		    $swap_following_f/$total_following_f,
		    $other_following_f/$total_following_f);
    }
    if(defined($REORDERING_MODEL{"monotonicity-f"})) {
	printf MF ("%s ||| %.5f %.5f\n",
		  $f_current, 
		   $mono_previous_f/$total_previous_f,
		   ($swap_previous_f+$other_previous_f)/$total_previous_f);
    }
    if(defined($REORDERING_MODEL{"monotonicity-bidirectional-f"})) {
	printf MBF ("%s ||| %.5f %.5f %.5f %.5f\n",
		    $f_current, 
		    $mono_previous_f/$total_previous_f,
		    ($swap_previous_f+$other_previous_f)/$total_previous_f,
		    $mono_following_f/$total_following_f,
		    ($swap_following_f+$other_following_f)/$total_following_f);
    }
}

sub store_reordering_fe {
    my $total_previous_fe = $mono_previous_fe+$swap_previous_fe+$other_previous_fe;
    my $total_following_fe = $mono_following_fe+$swap_following_fe+$other_following_fe;
    
    if(defined($REORDERING_MODEL{"orientation-fe"})) {
 	printf OFE ("%s ||| %s ||| %.5f %.5f %.5f\n",
		   $f_current, $e_current, 
		   $mono_previous_fe/$total_previous_fe,
		   $swap_previous_fe/$total_previous_fe,
		   $other_previous_fe/$total_previous_fe);
    }
    if(defined($REORDERING_MODEL{"orientation-bidirectional-fe"})) {
	printf OBFE ("%s ||| %s ||| %.5f %.5f %.5f %.5f %.5f %.5f\n",
		    $f_current, $e_current, 
		    $mono_previous_fe/$total_previous_fe,
		    $swap_previous_fe/$total_previous_fe,
		    $other_previous_fe/$total_previous_fe,
		    $mono_following_fe/$total_following_fe,
		    $swap_following_fe/$total_following_fe,
		    $other_following_fe/$total_following_fe);
    }
    if(defined($REORDERING_MODEL{"monotonicity-fe"})) {
	printf MFE ("%s ||| %s ||| %.5f %.5f\n",
		   $f_current, $e_current, 
		   $mono_previous_fe/$total_previous_fe,
		   ($swap_previous_fe+$other_previous_fe)/$total_previous_fe);
    }
    if(defined($REORDERING_MODEL{"monotonicity-bidirectional-fe"})) {
	printf MBFE ("%s ||| %s ||| %.5f %.5f %.5f %.5f\n",
		    $f_current, $e_current, 
		    $mono_previous_fe/$total_previous_fe,
		    ($swap_previous_fe+$other_previous_fe)/$total_previous_fe,
		    $mono_following_fe/$total_following_fe,
		    ($swap_following_fe+$other_following_fe)/$total_following_fe);
    }
}

### (8) LEARN GENERATION MODEL

my $factor_e_source;
sub get_generation_factored {
    print STDERR "(8) learn generation model @ ".`date`;
    if (defined $___GENERATION_FACTORS) {
      foreach my $f (split(/\+/,$___GENERATION_FACTORS)) {
	$factor = $f;
	($factor_e_source,$factor_e) = split(/\-/,$factor);
	&get_generation();
      }
    } else {
      print STDERR "  no generation model requested, skipping step\n";
    }
}

sub get_generation {
    print STDERR "(8) [$factor] generate generation table @ ".`date`;
    my (%WORD_TRANSLATION,%TOTAL_FOREIGN,%TOTAL_ENGLISH);

    my %INCLUDE_SOURCE;
    foreach my $factor (split(/,/,$factor_e_source)) {
	
	$INCLUDE_SOURCE{$factor} = 1;
    }
    my %INCLUDE;
    foreach my $factor (split(/,/,$factor_e)) {
	$INCLUDE{$factor} = 1;
    }

    my (%GENERATION,%GENERATION_TOTAL_SOURCE,%GENERATION_TOTAL_TARGET);
    open(E,$___CORPUS.".".$___E) or die "Can't read ".$___CORPUS.".".$___E;
    $alignment_id=0;
    while(<E>) {
	chomp;
	foreach (split) {
	    my @FACTOR = split(/\|/);

	    my ($source,$target);
	    my $first_factor = 1;
	    foreach my $factor (split(/,/,$factor_e_source)) {
		$source .= "|" unless $first_factor;
		$first_factor = 0;
		$source .= $FACTOR[$factor];
	    }

	    $first_factor = 1;
	    foreach my $factor (split(/,/,$factor_e)) {
		$target .= "|" unless $first_factor;
		$first_factor = 0;
		$target .= $FACTOR[$factor];
	    }	    
	    $GENERATION{$source}{$target}++;
	    $GENERATION_TOTAL_SOURCE{$source}++;
	    $GENERATION_TOTAL_TARGET{$target}++;
	}
    } 
    close(E);
 
    open(GEN,">$___MODEL_DIR/generation.$factor") or die "Can't write $___MODEL_DIR/generation.$factor";
    foreach my $source (keys %GENERATION) {
	foreach my $target (keys %{$GENERATION{$source}}) {
	    printf GEN ("%s %s %.7f %.7f\n",$source,$target,
			$GENERATION{$source}{$target}/$GENERATION_TOTAL_SOURCE{$source},
			$GENERATION{$source}{$target}/$GENERATION_TOTAL_TARGET{$target});
	}
    }
    close(GEN);
    safesystem("rm -f $___MODEL_DIR/generation.$factor.gz") or die;
    safesystem("gzip $___MODEL_DIR/generation.$factor") or die;
}

### (9) CREATE CONFIGURATION FILE

sub create_ini {
    print STDERR "(9) create moses.ini @ ".`date`;
    
    &full_path(\$___MODEL_DIR);
    &full_path(\$___VCB_E);
    &full_path(\$___VCB_F);
    open(INI,">$___MODEL_DIR/moses.ini") or die "Can't write $___MODEL_DIR/moses.ini";
    print INI "#########################
### MOSES CONFIG FILE ###
#########################
\n";

    if (defined $___TRANSLATION_FACTORS) {
      print INI "# input factors\n";
      print INI "[input-factors]\n";
      my $INPUT_FACTOR_MAX = 0;
      foreach my $table (split /\+/, $___TRANSLATION_FACTORS) {
	      my ($factor_list, $output) = split /-+/, $table;
        foreach (split(/,/,$factor_list)) {
          $INPUT_FACTOR_MAX = $_ if $_>$INPUT_FACTOR_MAX;
        }  
      }
      for (my $c = 0; $c <= $INPUT_FACTOR_MAX; $c++) { print INI "$c\n"; }
    } else {
      die "No translation steps defined, cannot prepare [input-factors] section\n";
    }


    my %stepsused;
    print INI "\n# mapping steps
[mapping]\n";
   foreach (split(/,/,$___DECODING_STEPS)) {
     s/t/T /g; 
     s/g/G /g;
     my ($type, $num) = split /\s+/;
     $stepsused{$type} = $num+1 if !defined $stepsused{$type} || $stepsused{$type} < $num+1;
     print INI $_."\n";
   }
   print INI "\n# translation tables: source-factors, target-factors, number of scores, file 
[ttable-file]\n";
   my $num_of_ttables = 0;
   foreach my $f (split(/\+/,$___TRANSLATION_FACTORS)) {
     $num_of_ttables++;
     my $ff = $f;
     $ff =~ s/\-/ /;
     print INI "$ff 5 $___MODEL_DIR/phrase-table.$f.gz\n";
   }
   if ($num_of_ttables != $stepsused{"T"}) {
     print STDERR "WARNING: Your [mapping-steps] require translation steps up to id $stepsused{T} but you defined translation steps 0..$num_of_ttables\n";
     exit 1 if $num_of_ttables < $stepsused{"T"}; # fatal to define less
   }

    my $weights_per_generation_model = 2;

    if (defined $___GENERATION_FACTORS) {
      print INI "\n# generation models: source-factors, target-factors, number-of-weights, filename\n";
      print INI "[generation-file]\n";
      my $cnt = 0;
      foreach my $f (split(/\+/,$___GENERATION_FACTORS)) {
        $cnt++;
        my $ff = $f;
        $ff =~ s/\-/ /;
        print INI "$ff $weights_per_generation_model $___MODEL_DIR/generation.$f.gz\n";
      }
      if ($cnt != $stepsused{"G"}) {
        print STDERR "WARNING: Your [mapping-steps] require generation steps up to id $stepsused{G} but you defined generation steps 0..$cnt\n";
        exit 1 if $cnt < $stepsused{"G"}; # fatal to define less
      }
    } else {
      print INI "\n# no generation models, no generation-file section\n";
    }

print INI "\n# language models: type(srilm/irstlm), factors, order, file
[lmodel-file]\n";
  foreach my $lm (@___LM) {
    my ($f, $o, $fn) = @$lm;
    my $type = 0; # default to srilm
    print INI "$type $f $o $fn\n";
  }

print INI "\n\n# limit on how many phrase translations e for each phrase f are loaded
# 0 = all elements loaded
[ttable-limit]
20\n";
  foreach(1..$num_of_ttables) {
    print INI "0\n";
  }

  my $weight_d_count = 0;
  if ($___REORDERING ne "distance") {
    my $file = "# distortion (reordering) files\n[distortion-file]\n";
    my $type = "# distortion (reordering) type\n[distortion-type]\n";
    my $factor_i = 0;
    foreach my $factor (split(/\+/,$___REORDERING_FACTORS)) {
	foreach my $r (keys %REORDERING_MODEL) {
	    next if $r eq "fe" || $r eq "f";
	    next if $r eq "distance" && $factor_i>0;
	    $type .= $r."\n";
	    if ($r eq "distance") { $weight_d_count++; } 
	    else {
		$r =~ s/-bidirectional/.bi/;
		$r =~ s/-f/.f/;
		$r =~ s/orientation/orientation-table.$factor/;
		$r =~ s/monotonicity/monotonicity-table.$factor/;
		$file .= "$___MODEL_DIR/$r.$___REORDERING_SMOOTH.gz\n";
		
		my $w;
		if ($r =~ /orient/) { $w = 3; } else { $w = 1; }
		if ($r =~ /bi/) { $w *= 2; }
		$weight_d_count += $w;
	    }
	}
        $factor_i++;
    }
    print INI $type."\n".$file."\n";
  }
  else {
    $weight_d_count = 1;
  }
  
  print INI "# distortion (reordering) weight\n[weight-d]\n";
  for(my $i=0;$i<$weight_d_count;$i++) { 
    print INI "".(0.6/(scalar keys %REORDERING_MODEL))."\n";
  }
  print INI "\n# language model weights
[weight-l]\n";
  my $lmweighttotal = 0.5;
  foreach(1..scalar @___LM) {
    printf INI "%.4f\n", $lmweighttotal / scalar @___LM;
  }

print INI "\n\n# translation model weights
[weight-t]\n";
   foreach my $f (split(/\+/,$___TRANSLATION_FACTORS)) {
     print INI "0.2\n0.2\n0.2\n0.2\n0.2\n";
   }

    if (defined $___GENERATION_FACTORS) {
      print INI "\n# generation model weights, for each model $weights_per_generation_model weights\n";
      print INI "[weight-generation]\n";
      foreach my $f (split(/\+/,$___GENERATION_FACTORS)) {
        print INI "0.3\n0\n";
      }
    } else {
      print INI "\n# no generation models, no weight-generation section\n";
    }

print INI "\n# word penalty
[weight-w]
-1

[distortion-limit]
6
";


  close(INI);
}

sub full_path {
    my ($PATH) = @_;
    return if $$PATH =~ /^\//;
    $$PATH = `pwd`."/".$$PATH;
    $$PATH =~ s/[\r\n]//g;
    $$PATH =~ s/\/\.\//\//g;
    $$PATH =~ s/\/+/\//g;
    my $sanity = 0;
    while($$PATH =~ /\/\.\.\// && $sanity++<10) {
	$$PATH =~ s/\/+/\//g;
	$$PATH =~ s/\/[^\/]+\/\.\.\//\//g;
    }
    $$PATH =~ s/\/[^\/]+\/\.\.$//;
    $$PATH =~ s/\/+$//;
}

sub safesystem {
  print STDERR "Executing: @_\n";
  system(@_);
  if ($? == -1) {
      print STDERR "Failed to execute: @_\n  $!\n";
      exit(1);
  }
  elsif ($? & 127) {
      printf STDERR "Execution of: @_\n  died with signal %d, %s coredump\n",
          ($? & 127),  ($? & 128) ? 'with' : 'without';
  }
  else {
    my $exitcode = $? >> 8;
    print STDERR "Exit code: $exitcode\n" if $exitcode;
    return ! $exitcode;
  }
}
