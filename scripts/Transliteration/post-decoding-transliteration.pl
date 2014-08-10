#!/usr/bin/perl -w

use strict;

use utf8;
use File::Basename;
use Getopt::Long "GetOptions";
use FindBin qw($RealBin);
use Scalar::Util qw(looks_like_number);
use IO::Handle;
binmode(STDIN,  ':utf8');
binmode(STDOUT, ':utf8');
binmode(STDERR, ':utf8');

my $___FACTOR_DELIMITER = "|";

my ($MOSES_SRC_DIR,$TRANSLIT_MODEL,$EVAL_DIR,$OUTPUT_FILE,$OUTPUT_FILE_NAME,$OOV_FILE, $OOV_FILE_NAME, $EXTERNAL_BIN_DIR, $LM_FILE, $INPUT_EXTENSION, $OUTPUT_EXTENSION, $INPUT_FILE,$VERBOSE,$DECODER);
die("ERROR: wrong syntax when invoking postDecodingTransliteration.perl")
    unless &GetOptions('moses-src-dir=s' => \$MOSES_SRC_DIR,
			'external-bin-dir=s' => \$EXTERNAL_BIN_DIR,
			'transliteration-model-dir=s' => \$TRANSLIT_MODEL,
			'input-extension=s' => \$INPUT_EXTENSION,
			'output-extension=s' => \$OUTPUT_EXTENSION,
			'decoder=s' => \$DECODER,
			'oov-file=s' => \$OOV_FILE,
			'input-file=s' => \$INPUT_FILE,
			'output-file=s' => \$OUTPUT_FILE,
			'verbose' => \$VERBOSE,
			'language-model=s' => \$LM_FILE);

# check if the files are in place
die("ERROR: you need to define --moses-src-dir --external-bin-dir, --transliteration-model-dir, --oov-file, --output-file --input-extension, --output-extension, and --language-model")
    unless (defined($MOSES_SRC_DIR) &&
            defined($TRANSLIT_MODEL) &&
            defined($OOV_FILE) &&
	     defined($INPUT_EXTENSION)&&	
	     defined($OUTPUT_EXTENSION)&&	
	     defined($INPUT_FILE)&&
	     defined($EXTERNAL_BIN_DIR)&&	
            defined($LM_FILE));
if (! -e $LM_FILE) {
  my $LM_FILE_WORD = `ls $LM_FILE*word*`;
  chop($LM_FILE_WORD);
  $LM_FILE = $LM_FILE_WORD if $LM_FILE_WORD ne "";
}
die("ERROR: could not find Language Model '$LM_FILE'")
    unless -e $LM_FILE;
die("ERROR: could not find Transliteration Model '$TRANSLIT_MODEL'")
    unless -e $TRANSLIT_MODEL;
die("ERROR: could not find OOV file $OOV_FILE'")
    unless -e $OOV_FILE;
die("ERROR: could not find Input file $INPUT_FILE'")
    unless -e $INPUT_FILE;

$EVAL_DIR = dirname($INPUT_FILE);
$OUTPUT_FILE_NAME = basename ($OUTPUT_FILE);
$OOV_FILE_NAME = basename ($OOV_FILE);

`mkdir $TRANSLIT_MODEL/evaluation`;
`cp $OOV_FILE $TRANSLIT_MODEL/evaluation/`;
my $translitFile = $TRANSLIT_MODEL . "/evaluation/" . $OOV_FILE_NAME;

print "Preparing for Transliteration\n";
prepare_for_transliteration ($OOV_FILE, $translitFile);
print "Run Transliteration\n";
run_transliteration ($MOSES_SRC_DIR , $EXTERNAL_BIN_DIR , $TRANSLIT_MODEL , $OOV_FILE_NAME);
print "Pick Best Transliteration\n";
form_corpus ($translitFile , $translitFile.".op.nBest" , $EVAL_DIR);
run_decoder($MOSES_SRC_DIR, $EXTERNAL_BIN_DIR, $LM_FILE);

################### Read the UNK word file and prepare for Transliteration ###############################

sub prepare_for_transliteration
{
	my @list = @_;
	my $testFile = $list[0];
	my $translitFile = $list[1];
	my %UNK;
	my @words;
	my $src;
	my @tW;

	open MYFILE,  "<:encoding(UTF-8)", $testFile or die "Can't open $testFile: $!\n";

	while (<MYFILE>) 
	{
        chomp;
        #print "$_\n";
        @words = split(/ /, "$_");

	 foreach (@words)
         {
		
		@tW = split /\Q$___FACTOR_DELIMITER/;

		if (defined $tW[0])
		{
		
		  if (! ($tW[0] =~ /[0-9.,]/))
		   {
			$UNK{$tW[0]} = 1;
		   }
		   else
		   {
		   	print "Not transliterating $tW[0] \n";
		   }
		}    
         }
	}
	 close (MYFILE);

	open MYFILE,  ">:encoding(UTF-8)", $translitFile or die "Can't open $translitFile: $!\n";

	foreach my $key ( keys %UNK )
	{
  		$src=join(' ', split('',$key));
 		print MYFILE "$src\n";	
	}
	 close (MYFILE);
}

################### Run Transliteration Module to Obtain Transliterations ###############################

sub run_transliteration
{
	my @list = @_;
	my $MOSES_SRC = $list[0];
	my $EXTERNAL_BIN_DIR = $list[1];
	my $TRANSLIT_MODEL = $list[2];
	my $eval_file = $list[3];

	`touch $TRANSLIT_MODEL/evaluation/$eval_file.moses.table.ini`;
	
	print "Filter Table\n";

	`$MOSES_SRC/scripts/training/train-model.perl -mgiza -mgiza-cpus 10 -dont-zip -first-step 9 -external-bin-dir $EXTERNAL_BIN_DIR -f $INPUT_EXTENSION -e $OUTPUT_EXTENSION -alignment grow-diag-final-and -parts 5 -score-options '--KneserNey' -phrase-translation-table $TRANSLIT_MODEL/model/phrase-table -config $TRANSLIT_MODEL/evaluation/$eval_file.moses.table.ini -lm 0:3:$TRANSLIT_MODEL/evaluation/$eval_file.moses.table.ini:8`;

	`$MOSES_SRC/scripts/training/filter-model-given-input.pl $TRANSLIT_MODEL/evaluation/$eval_file.filtered $TRANSLIT_MODEL/evaluation/$eval_file.moses.table.ini $TRANSLIT_MODEL/evaluation/$eval_file  -Binarizer "$MOSES_SRC/bin/processPhraseTable"`;

	`rm  $TRANSLIT_MODEL/evaluation/$eval_file.moses.table.ini`;

	print "Apply Filter\n";

	`$MOSES_SRC/scripts/ems/support/substitute-filtered-tables-and-weights.perl $TRANSLIT_MODEL/evaluation/$eval_file.filtered/moses.ini $TRANSLIT_MODEL/model/moses.ini $TRANSLIT_MODEL/tuning/moses.tuned.ini $TRANSLIT_MODEL/evaluation/$eval_file.filtered.ini`;

  my $drop_stderr = $VERBOSE ? "" : " 2>/dev/null";
	`$DECODER -search-algorithm 1 -cube-pruning-pop-limit 5000 -s 5000 -threads 16 -drop-unknown -distortion-limit 0 -n-best-list $TRANSLIT_MODEL/evaluation/$eval_file.op.nBest 1000 distinct -f $TRANSLIT_MODEL/evaluation/$eval_file.filtered.ini < $TRANSLIT_MODEL/evaluation/$eval_file > $TRANSLIT_MODEL/evaluation/$eval_file.op $drop_stderr`;

}

################### Read the output of Transliteration Model and Form Corpus ###############################


sub form_corpus
{

	my @list = @_;
	my $inp_file = $list[0];
	my $testFile = $list[1];
	my $EVAL_DIR = $list[2];
	my @words;
	my $thisStr;
	my $features;
	my $prev = 0;
	my $sNum;
	my @UNK;
	my %vocab;

	`mkdir -p $EVAL_DIR/Transliteration-Module/$OUTPUT_FILE_NAME/model`;
	my $antLog = exp(0.2);

	my $phraseTable = $EVAL_DIR . "/Transliteration-Module/$OUTPUT_FILE_NAME/model/phrase-table";
	
	open MYFILE,  "<:encoding(UTF-8)", $inp_file or die "Can't open $inp_file: $!\n";
	open PT,  ">:encoding(UTF-8)", $phraseTable or die "Can't open $phraseTable: $!\n";

	while (<MYFILE>) 
	{
        chomp;
        #print "$_\n";
        @words = split(/ /, "$_");
	 
	  $thisStr = "";
	  foreach (@words)
         {
         	$thisStr = $thisStr . "$_";
         }

	  push(@UNK, $thisStr);
	  $vocab{$thisStr} = 1;	
	}
	 close (MYFILE);

	open MYFILE,  "<:encoding(UTF-8)", $testFile or die "Can't open $testFile: $!\n";
	my $inpCount = 0;

	while (<MYFILE>) 
	{
       	 chomp;
        	#print "$_\n";
        	@words = split(/ /, "$_");

	 	$sNum = $words[0];

		if ($prev != $sNum){
			$inpCount++;
		} 
	
		my $i = 2;
		$thisStr = "";
		$features = "";

		while ($words[$i] ne "|||")
		{
			$thisStr = $thisStr . $words[$i];
			$i++;
		}

		$i++;
		
		while ($words[$i] ne "|||")
		{
			if ($words[$i] =~ /Penalty0/ || $words[$i] eq "Distortion0=" || $words[$i] eq "LM0=" ){
				$i++;
			}
			elsif (looks_like_number($words[$i])){
				$features = $features . " " . exp($words[$i]);
			}

			$i++;
		}
		$i++;

		#$features = $features . " " . $words[$i];
		
		if ($thisStr ne ""){
		 print PT "$UNK[$inpCount] ||| $thisStr ||| $features ||| 0-0 ||| 0 0 0\n";
		}
		$prev = $sNum;
 	}
 	close (MYFILE);
	

	open MYFILE,  "<:encoding(UTF-8)", $INPUT_FILE or die "Can't open $INPUT_FILE: $!\n";
	
	my %dd;

	while (<MYFILE>) 
	{
       	chomp;
        	@words = split(/ /, "$_");
		
		foreach (@words)
         	{
         		if (! exists $vocab{$_} && ! exists $dd{$_}){
			
				print PT "$_ ||| $_ ||| 1.0 1.0 1.0 1.0 ||| 0-0 ||| 0 0 0\n";
				$dd{$_} = 1;	
			}
         	}
	}			
		
	close (PT);
	close (MYFILE);
		
	`gzip $phraseTable`;
	
}


################### Run Decoder ###############################

sub run_decoder
{

	my @list = @_;
	my $MOSES_SRC = $list[0];
	my $EXTERNAL_BIN_DIR = $list[1];
	my $corpus_dir = $EVAL_DIR . "/Transliteration-Module/$OUTPUT_FILE_NAME";
	my $LM_FILE = $list[2];
	my @words;
	my $final_file = $EVAL_DIR . "/$OUTPUT_FILE_NAME";

	my $find = ".cleaned.";
	my $replace = ".transliterated.";
  if ($final_file !~ /$find/) {
    $find = ".output.";
  }
	$final_file =~ s/$find/$replace/g;
	
	`mkdir $corpus_dir/evaluation`;

	`$MOSES_SRC/scripts/training/train-model.perl -mgiza -mgiza-cpus 10 -dont-zip -first-step 9 -external-bin-dir $EXTERNAL_BIN_DIR -f $INPUT_EXTENSION -e $OUTPUT_EXTENSION -alignment grow-diag-final-and -parts 5 -lmodel-oov-feature "yes" -post-decoding-translit "yes" -phrase-translation-table $corpus_dir/model/phrase-table -config $corpus_dir/model/moses.ini -lm 0:5:$LM_FILE:8`;

	`touch $corpus_dir/evaluation/$OUTPUT_FILE_NAME.moses.table.ini`;

	`$MOSES_SRC/scripts/training/train-model.perl -mgiza -mgiza-cpus 10 -dont-zip -first-step 9 -external-bin-dir $EXTERNAL_BIN_DIR -f $INPUT_EXTENSION -e $OUTPUT_EXTENSION -alignment grow-diag-final-and -parts 5 -lmodel-oov-feature "yes" -post-decoding-translit "yes" -phrase-translation-table $corpus_dir/model/phrase-table -config $corpus_dir/evaluation/$OUTPUT_FILE_NAME.moses.table.ini -lm 0:3:$corpus_dir/evaluation/$OUTPUT_FILE_NAME.moses.table.ini:8`;

	`$MOSES_SRC/scripts/training/filter-model-given-input.pl $corpus_dir/evaluation/filtered $corpus_dir/evaluation/$OUTPUT_FILE_NAME.moses.table.ini $INPUT_FILE  -Binarizer "$MOSES_SRC/bin/processPhraseTable"`;

	`rm $corpus_dir/evaluation/$OUTPUT_FILE_NAME.moses.table.ini`;

	`$MOSES_SRC/scripts/ems/support/substitute-filtered-tables.perl $corpus_dir/evaluation/filtered/moses.ini < $corpus_dir/model/moses.ini > $corpus_dir/evaluation/moses.filtered.ini`;
	
  my $drop_stderr = $VERBOSE ? "" : " 2>/dev/null";
	`$DECODER -search-algorithm 1 -cube-pruning-pop-limit 5000 -s 5000 -threads 16 -feature-overwrite 'TranslationModel0 table-limit=100' -max-trans-opt-per-coverage 100 -f $corpus_dir/evaluation/moses.filtered.ini -distortion-limit 0 < $INPUT_FILE > $OUTPUT_FILE $drop_stderr`;
	
	print "$DECODER -search-algorithm 1 -cube-pruning-pop-limit 5000 -s 5000 -threads 16 -feature-overwrite 'TranslationModel0 table-limit=100' -max-trans-opt-per-coverage 100 -f $corpus_dir/evaluation/moses.filtered.ini -distortion-limit 0 < $INPUT_FILE > $OUTPUT_FILE $drop_stderr\n";
}


