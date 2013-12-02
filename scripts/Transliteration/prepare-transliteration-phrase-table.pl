#!/usr/bin/perl -w

use strict;

use utf8;
use Getopt::Long "GetOptions";
use FindBin qw($RealBin);
use IO::Handle;
use File::Basename;
binmode(STDIN,  ':utf8');
binmode(STDOUT, ':utf8');
binmode(STDERR, ':utf8');

my $OUT_DIR = "/tmp/Transliteration-Phrase-Table.$$";

my ($MOSES_SRC_DIR,$TRANSLIT_MODEL,$OOV_FILE,$EXTERNAL_BIN_DIR, $INPUT_EXTENSION, $OUTPUT_EXTENSION);
die("ERROR: wrong syntax when invoking train-transliteration-PT.pl")
    unless &GetOptions('moses-src-dir=s' => \$MOSES_SRC_DIR,
			'external-bin-dir=s' => \$EXTERNAL_BIN_DIR,
			'transliteration-model-dir=s' => \$TRANSLIT_MODEL,
			'input-extension=s' => \$INPUT_EXTENSION,
			'output-extension=s' => \$OUTPUT_EXTENSION,
			'out-dir=s' => \$OUT_DIR,
			'oov-file=s' => \$OOV_FILE);

# check if the files are in place
die("ERROR: you need to define --moses-src-dir --external-bin-dir, --transliteration-model-dir, --oov-file, --input-extension, --output-extension")
    unless (defined($MOSES_SRC_DIR) &&
            defined($TRANSLIT_MODEL) &&
            defined($OOV_FILE) &&
	    defined($INPUT_EXTENSION)&&	
	    defined($OUTPUT_EXTENSION));

die("ERROR: could not find Transliteration Model '$TRANSLIT_MODEL'")
    unless -e $TRANSLIT_MODEL;
die("ERROR: could not find OOV file '$OOV_FILE'")
    unless -e $OOV_FILE;

 my $UNK_FILE_NAME = basename($OOV_FILE);
`mkdir -p $OUT_DIR/$UNK_FILE_NAME/training`;
`cp $OOV_FILE $OUT_DIR/$UNK_FILE_NAME/$UNK_FILE_NAME`;

my $translitFile = "$OUT_DIR/" . $UNK_FILE_NAME . "/" . $UNK_FILE_NAME . ".translit";

print "Preparing for Transliteration\n";
prepare_for_transliteration ($OOV_FILE , $translitFile);
print "Run Transliteration\n";
run_transliteration ($MOSES_SRC_DIR , $EXTERNAL_BIN_DIR , $TRANSLIT_MODEL , $translitFile);
print "Form Transliteration Corpus\n";
form_corpus ($translitFile , $translitFile.".op.nBest" , $OUT_DIR);


################### Read the UNK word file and prepare for Transliteration ###############################

sub prepare_for_transliteration
{
	my @list = @_;
	my $testFile = $list[0];
	my $translitFile = $list[1];
	my %UNK;
	my @words;
	my $src;
	open MYFILE,  "<:encoding(UTF-8)", $testFile or die "Can't open $testFile: $!\n";

	while (<MYFILE>) 
	{
        chomp;
        #print "$_\n";
        @words = split(/ /, "$_");

	  foreach (@words)
         {
         	$UNK{"$_"} = 1;
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

	`touch $eval_file.moses.table.ini`;
	
	print "Filter Table\n";

	`$MOSES_SRC/scripts/training/train-model.perl -mgiza -mgiza-cpus 10 -dont-zip -first-step 9 -external-bin-dir $EXTERNAL_BIN_DIR -f $INPUT_EXTENSION -e $OUTPUT_EXTENSION -alignment grow-diag-final-and -parts 5 -reordering msd-bidirectional-fe -score-options '--KneserNey' -phrase-translation-table $TRANSLIT_MODEL/model/phrase-table -reordering-table $TRANSLIT_MODEL/model/reordering-table -config $eval_file.moses.table.ini -lm 0:3:$eval_file.moses.table.ini:8`;
	
	`$MOSES_SRC/scripts/training/filter-model-given-input.pl $eval_file.filtered $eval_file.moses.table.ini $eval_file  -Binarizer "$MOSES_SRC/bin/processPhraseTable"`;

	`rm  $eval_file.moses.table.ini`;

	print "Apply Filter\n";

	`$MOSES_SRC/scripts/ems/support/substitute-filtered-tables-and-weights.perl $eval_file.filtered/moses.ini $TRANSLIT_MODEL/model/moses.ini $TRANSLIT_MODEL/tuning/moses.tuned.ini $eval_file.filtered.ini`;

	`$MOSES_SRC/bin/moses -search-algorithm 1 -cube-pruning-pop-limit 5000 -s 5000 -threads 16 -drop-unknown -distortion-limit 0 -n-best-list $eval_file.op.nBest 50 -f $eval_file.filtered.ini < $eval_file > $eval_file.op`;

}

################### Read the output of Transliteration Model and Form Corpus ###############################


sub form_corpus
{

	my @list = @_;
	my $inp_file = $list[0];
	my $testFile = $list[1];
	my $EVAL_DIR = $list[2];
	my %vocab;
	my @words;
	my $thisStr;

	my $UNK_FILE_NAME = basename($OOV_FILE);
	my $target = $EVAL_DIR . "/$UNK_FILE_NAME/training/corpus.$OUTPUT_EXTENSION";
	
	
	open MYFILE,  "<:encoding(UTF-8)", $testFile or die "Can't open $testFile: $!\n";


	while (<MYFILE>) 
	{
       	 chomp;
        	#print "$_\n";
        	@words = split(/ /, "$_");
	 	
	 
		my $i = 2;
		my $prob;

		$thisStr = "";

		while ($words[$i] ne "|||")
		{
			$thisStr = $thisStr . $words[$i];
			$i++;
		}

		$i++;

		while ($words[$i] ne "|||")
		{
			$i++;	
		}
		
		$i++;
		$prob = $words[$i];
        	
		print "$thisStr \t $prob\n";
 	}
 	close (MYFILE);
}

