#!/usr/bin/perl -w
use Getopt::Long;
use File::Basename;
use POSIX;

# read arguments
my $line_start = 0;
my $line_end = LONG_MAX;
my $tmp_dir = "";
my $dec_size = LONG_MAX;
$_HELP = 1 if (@ARGV < 1 or !GetOptions ("moses_ini=s" => \$moses_ini, #moses conf file
"start:i" => \$line_start, #fisrt phrase to process
"end:i" => \$line_end, #last sentence to process (not including)
"training_s=s" => \$training_s, #source training file
"training_t=s" => \$training_t, #target training file
"prune_bin=s" => \$prune_bin, #binary files in the pruning toolkit
"prune_scripts=s" => \$prune_scripts, #scripts in the pruning toolkit
"sig_bin=s" => \$sig_bin, #binary files to calculate significance
"moses_scripts=s" => \$moses_scripts, #dir with the moses scripts
"tmp_dir:s" => \$tmp_dir, #dir with the moses scripts
"dec_size:i" => \$dec_size, #dir with the moses scripts
"workdir=s" => \$workdir)); #directory to put all the output files

# help message if arguments are not correct
if ($_HELP) {
    print "
Usage: perl calcPruningScores.pl [PARAMS]
Function: Calculates relative entropy for each phrase pair in a translation model.
Authors: Wang Ling ( lingwang at cs dot cmu dot edu )
PARAMS:
  -moses_ini : moses configuration file with the model to prune (phrase table, reordering table, weights etc...)
  -training_s : source training file, please run salm first
  -training_t : target training file, please run salm first
  -prune_bin : path to the binaries for pruning (probably <PATH_TO_MOSES>/bin)
  -prune_scripts : path to the scripts for pruning (probably the directory where this script is)
  -sig_bin : path to the binary for significance testing included in this toolkit
  -moses_scripts : path to the moses training scripts (where filter-model-given-input.pl is)
  -workdir : directory to produce the output
  -tmp_dir : directory to store temporary files (improve performance if stored in a local disk), omit to store in workdir
  -dec_size : number of phrase pairs to be decoded at a time, omit to decode all selected phrase pairs at once
  -start and -end : starting and ending phrase pairs to process, to be used if you want to launch multiple processes in parallel for different parts of the phrase table. If specified the process will process the phrase pairs from <start> to <end-1>

For any questions contact lingwang at cs dot cmu dot edu
";
  exit(1);
}

# setting up working dirs
my $TMP_DIR = $tmp_dir;
if ($tmp_dir eq ""){
   $TMP_DIR = "$workdir/tmp";
}
my $SCORE_DIR = "$workdir/scores";
my $FILTER_DIR = "$TMP_DIR/filter";

# files for divergence module
my $SOURCE_FILE = "$TMP_DIR/source.txt";
my $CONSTRAINT_FILE = "$TMP_DIR/constraint.txt";
my $DIVERGENCE_FILE = "$SCORE_DIR/divergence.txt";

# files for significance module
my $SIG_TABLE_FILE = "$TMP_DIR/source_target.txt";
my $SIG_MOD_OUTPUT = "$TMP_DIR/sig_mod.out";
my $SIG_FILE = "$SCORE_DIR/significance.txt";
my $COUNT_FILE = "$SCORE_DIR/count.txt";
my $EMP_DIST_FILE= "$SCORE_DIR/empirical.txt";
my $REL_ENT_FILE= "$SCORE_DIR/rel_ent.txt";

# setting up executables
my $ZCAT = "gzip -cd";
my $BZCAT = "bzcat";
my $CP = "cp";
my $SED = "sed";
my $RM = "rm";
my $SORT_EXEC = "sort";
my $PRUNE_EXEC = "$prune_bin/calcDivergence";
my $SIG_EXEC = "$sig_bin/filter-pt";
my $FILTER_EXEC = "perl $moses_scripts/filter-model-given-input.pl";
my $CALC_EMP_EXEC ="perl $prune_scripts/calcEmpiricalDistribution.pl";
my $INT_TABLE_EXEC = "perl $prune_scripts/interpolateScores.pl";

# moses ini variables
my ($TRANSLATION_TABLE_FILE, $REORDERING_TABLE_FILE);

# phrase table variables
my ($N_PHRASES, $N_PHRASES_TO_PROCESS);

# main functions
&prepare();
&calc_sig_and_counts();
&calc_div();
&clear_up();

# (1) preparing data
sub prepare {
    print STDERR "(1) preparing data @ ".`date`;
    safesystem("mkdir -p $workdir") or die("ERROR: could not create work dir $workdir");
    safesystem("mkdir -p $TMP_DIR") or die("ERROR: could not create work dir $TMP_DIR");
    safesystem("mkdir -p $SCORE_DIR") or die("ERROR: could not create work dir $SCORE_DIR");
    &get_moses_ini_params();
    &copy_tables_to_tmp_dir();
    &write_data_files();
    
    $N_PHRASES = &get_number_of_phrases();
    $line_end = ($line_end > $N_PHRASES) ? $N_PHRASES : $line_end;
    $N_PHRASES_TO_PROCESS = $line_end - $line_start;
}

sub write_data_files {
    open(SOURCE_WRITER,">".$SOURCE_FILE) or die "ERROR: Can't write $SOURCE_FILE";
    open(CONSTRAINT_WRITER,">".$CONSTRAINT_FILE) or die "ERROR: Can't write $CONSTRAINT_FILE";
    open(TABLE_WRITER,">".$SIG_TABLE_FILE) or die "ERROR: Can't write $SIG_TABLE_FILE";
    open(TTABLE_READER, &open_compressed($TRANSLATION_TABLE_FILE)) or die "ERROR: Can't read $TRANSLATION_TABLE_FILE";
    
    $line_number = 0; 
    while($line_number < $line_start && !eof(TTABLE_READER)){
	<TTABLE_READER>;
	$line_number++;
    }
    while($line_number < $line_end && !eof(TTABLE_READER)) {
        my $line = <TTABLE_READER>;
        chomp($line);
	my @line_array = split(/\s+\|\|\|\s+/, $line);
	my $source = $line_array[0];
        my $target = $line_array[1];
        my $scores = $line_array[2];
        print TABLE_WRITER $source." ||| ".$target." ||| ".$scores."\n";
        print SOURCE_WRITER $source."\n";
	print CONSTRAINT_WRITER $target."\n";
        $line_number++;
    }
    
    close(SOURCE_WRITER);
    close(CONSTRAINT_WRITER);
    close(TABLE_WRITER);
    close(TTABLE_READER);
}

sub copy_tables_to_tmp_dir {
    $tmp_t_table = "$TMP_DIR/".basename($TRANSLATION_TABLE_FILE);
    $tmp_r_table = "$TMP_DIR/".basename($REORDERING_TABLE_FILE);
    $tmp_moses_ini = "$TMP_DIR/moses.ini";
    $cp_t_cmd = "$CP $TRANSLATION_TABLE_FILE $TMP_DIR";
    $cp_r_cmd = "$CP $REORDERING_TABLE_FILE $TMP_DIR";
    safesystem("$cp_t_cmd") or die("ERROR: could not run:\n $cp_t_cmd");
    safesystem("$cp_r_cmd") or die("ERROR: could not run:\n $cp_r_cmd");

    $sed_cmd = "$SED s#$TRANSLATION_TABLE_FILE#$tmp_t_table#g $moses_ini | $SED s#$REORDERING_TABLE_FILE#$tmp_r_table#g > $tmp_moses_ini";
    safesystem("$sed_cmd") or die("ERROR: could not run:\n $sed_cmd");
    
    $TRANSLATION_TABLE_FILE = $tmp_t_table;
    $REORDERING_TABLE_FILE = $tmp_r_table;
    $moses_ini = $tmp_moses_ini;
}

# (2) calculating sig and counts
sub calc_sig_and_counts {
    print STDERR "(2) calculating counts and significance".`date`;
    print STDERR "(2.1) running significance module".`date`;
    &run_significance_module();
    print STDERR "(2.2) writing counts and significance tables".`date`;
    &write_counts_and_significance_table();
    print STDERR "(2.3) calculating empirical distribution".`date`;
}

sub write_counts_and_significance_table {
    open(COUNT_WRITER,">".$COUNT_FILE) or die "ERROR: Can't write $COUNT_FILE";
    open(SIG_WRITER,">".$SIG_FILE) or die "ERROR: Can't write $SIG_FILE";
    open(SIG_MOD_READER, &open_compressed($SIG_MOD_OUTPUT)) or die "ERROR: Can't read $SIG_MOD_OUTPUT";

    while(<SIG_MOD_READER>) {
        my($line) = $_;
        chomp($line);
        my @line_array = split(/\s+\|\|\|\s+/, $line);
        my $count = $line_array[0];
        my $sig = $line_array[1];
        print COUNT_WRITER $count."\n";
        print SIG_WRITER $sig."\n";
    }

    close(SIG_MOD_READER);
    close(COUNT_WRITER);
    close(SIG_WRITER);
}

sub run_significance_module {
    my $sig_cmd = "cat $SIG_TABLE_FILE | $SIG_EXEC -e $training_t -f $training_s -l -10000 -p -c > $SIG_MOD_OUTPUT";
    safesystem("$sig_cmd") or die("ERROR: could not run:\n $sig_cmd");
}

# (3) calculating divergence
sub calc_div {
    print STDERR "(3) calculating relative entropy".`date`;
    print STDERR "(3.1) calculating empirical distribution".`date`;
    &calculate_empirical_distribution();
    print STDERR "(3.2) calculating divergence (this might take a while)".`date`;
    if($N_PHRASES_TO_PROCESS > $dec_size) {
       &calculate_divergence_shared("$FILTER_DIR");
    }
    else{
       &calculate_divergence($moses_ini);
    }
    print STDERR "(3.3) calculating relative entropy from empirical and divergence distributions".`date`;
    &calculate_relative_entropy();
}

sub calculate_empirical_distribution {
    my $emp_cmd = "$CALC_EMP_EXEC $COUNT_FILE > $EMP_DIST_FILE";
    safesystem("$emp_cmd") or die("ERROR: could not run:\n $emp_cmd");
}

sub get_fragmented_file_name {
    my ($name, $frag, $interval) = @_;
    return "$name-$frag-".($frag+$interval);
}    

sub calculate_divergence {
    my $moses_ini_file = $_[0];
    print STDERR "force decoding phrase pairs\n";
    my $prune_cmd = "cat $SOURCE_FILE | $PRUNE_EXEC -f $moses_ini_file -constraint $CONSTRAINT_FILE -early-discarding-threshold 0 -s 100000 -ttable-limit 0 > $DIVERGENCE_FILE 2> /dev/null";
    safesystem("$prune_cmd") or die("ERROR: could not run:\n $prune_cmd");
}

sub calculate_divergence_shared {
    my $filter_dir = $_[0];

    &split_file_into_chunks($SOURCE_FILE, $dec_size, $N_PHRASES_TO_PROCESS);
    &split_file_into_chunks($CONSTRAINT_FILE, $dec_size, $N_PHRASES_TO_PROCESS);

    for(my $i = 0; $i < $N_PHRASES_TO_PROCESS; $i = $i + $dec_size) {
        my $filter_cmd = "$FILTER_EXEC ".&get_fragmented_file_name($FILTER_DIR, $i, $dec_size)." $moses_ini ".&get_fragmented_file_name($SOURCE_FILE, $i, $dec_size);
        safesystem("$filter_cmd") or die("ERROR: could not run:\n $filter_cmd");

        my $moses_ini_file = &get_fragmented_file_name($filter_dir, $i, $dec_size)."/moses.ini";
        my $source_file = &get_fragmented_file_name($SOURCE_FILE, $i, $dec_size);
        my $constraint_file = &get_fragmented_file_name($CONSTRAINT_FILE, $i, $dec_size);
        my $prune_cmd;
	print STDERR "force decoding phrase pairs $i to ".($i + $dec_size)."\n";
	if($i == 0){
            $prune_cmd = "cat $source_file | $PRUNE_EXEC -f $moses_ini_file -constraint $constraint_file -early-discarding-threshold 0 -s 100000 -ttable-limit 0 > $DIVERGENCE_FILE 2> /dev/null";
	}
	else{
            $prune_cmd = "cat $source_file | $PRUNE_EXEC -f $moses_ini_file -constraint $constraint_file -early-discarding-threshold 0 -s 100000 -ttable-limit 0 >> $DIVERGENCE_FILE 2> /dev/null";
	}
        safesystem("$prune_cmd") or die("ERROR: could not run:\n $prune_cmd");

	my $rm_cmd = "$RM -r ".&get_fragmented_file_name($FILTER_DIR, $i, $dec_size);
        safesystem("$rm_cmd") or die("ERROR: could not run:\n $rm_cmd");

    }
}

sub calculate_relative_entropy {
    my $int_cmd = "$INT_TABLE_EXEC -files \"$EMP_DIST_FILE $DIVERGENCE_FILE\" -weights \"1 1\" -operation \"*\" > $REL_ENT_FILE";
    safesystem("$int_cmd") or die("ERROR: could not run:\n $int_cmd");

}

# (4) clear up stuff that is not needed
sub clear_up {
   print STDERR "(4) removing tmp dir".`date`;
   $rm_cmd = "$RM -r $TMP_DIR";
   safesystem("$rm_cmd") or die("ERROR: could not run:\n $rm_cmd");
}

# utility functions

sub safesystem {
  print STDERR "Executing: @_\n";
  system(@_);
  if ($? == -1) {
      print STDERR "ERROR: Failed to execute: @_\n  $!\n";
      exit(1);
  }
  elsif ($? & 127) {
      printf STDERR "ERROR: Execution of: @_\n  died with signal %d, %s coredump\n",
          ($? & 127),  ($? & 128) ? 'with' : 'without';
      exit(1);
  }
  else {
    my $exitcode = $? >> 8;
    print STDERR "Exit code: $exitcode\n" if $exitcode;
    return ! $exitcode;
  }
}

sub open_compressed {
    my ($file) = @_;
    print STDERR "FILE: $file\n";

    # add extensions, if necessary
    $file = $file.".bz2" if ! -e $file && -e $file.".bz2";
    $file = $file.".gz"  if ! -e $file && -e $file.".gz";

    # pipe zipped, if necessary
    return "$BZCAT $file|" if $file =~ /\.bz2$/;
    return "$ZCAT $file|"  if $file =~ /\.gz$/;
    return $file;
}

sub get_moses_ini_params {

    open(MOSES_READER, $moses_ini);
    while(<MOSES_READER>) {
        my($line) = $_;
        chomp($line);

        if($line eq "[ttable-file]"){
            $tableLine = <MOSES_READER>;
            chomp($tableLine);
            ($_,$_,$_,$_,$TRANSLATION_TABLE_FILE) = split(" ",$tableLine); # put the other parameters there if needed
        }
        if($line eq "[distortion-file]"){
            $tableLine = <MOSES_READER>;
            chomp($tableLine);
            ($_,$_,$_,$REORDERING_TABLE_FILE) = split(" ",$tableLine); # put the other parameters there if needed
        }
    }
    close(MOSES_READER);
}

sub get_number_of_phrases {
   my $ret = 0;
   open(TABLE_READER, &open_compressed($TRANSLATION_TABLE_FILE)) or die "ERROR: Can't read $TRANSLATION_TABLE_FILE";

   while(<TABLE_READER>) {
      $ret++;
   }

   close (TABLE_READER);
   return $ret;
}

sub split_file_into_chunks {
    my ($file_to_split, $chunk_size, $number_of_phrases_to_process) = @_;
    open(SOURCE_READER, &open_compressed($file_to_split)) or die "ERROR: Can't read $file_to_split";
    my $FRAG_SOURCE_WRITER;
    for(my $i = 0; $i < $number_of_phrases_to_process && !eof(SOURCE_READER); $i++) {
       if(($i % $chunk_size) == 0){ # open fragmented file to write
          my $frag_file = &get_fragmented_file_name($file_to_split, $i, $chunk_size);
          open(FRAG_SOURCE_WRITER, ">".$frag_file) or die "ERROR: Can't write $frag_file";
       }
       my $line = <SOURCE_READER>;
       print FRAG_SOURCE_WRITER $line;
       if((%i % $chunk_size) == $chunk_size - 1 || (%i % $chunk_size) == $number_of_phrases_to_process - 1){ # close fragmented file before opening a new one
          close(FRAG_SOURCE_WRITER);
       }
    }
}


