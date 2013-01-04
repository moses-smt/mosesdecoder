#!/usr/bin/perl -w

# read arguments
my $tmp_dir = "";
my $percentage = -1;
my $threshold = -1;
use Getopt::Long;
$_HELP = 1 if (@ARGV < 1 or !GetOptions ("table=s" => \$table, #table to filter
"scores=s" => \$scores_file, #scores of each phrase pair, should have same size as the table to filter
"percentage=f" => \$percentage, # percentage of phrase table to remain
"threshold=f" => \$threshold)); # threshold (score < threshold equals prune entry)

# help message if arguments are not correct
if ($_HELP) {
    print "Relative Entropy Pruning
Usage: perl prunePT.pl [PARAMS]
Function: prunes a phrase table given a score file 
Authors: Wang Ling ( lingwang at cs dot cmu dot edu )
PARAMS:
  -table : table to prune
  -percentage : percentage of phrase table to remain (if the scores do not allow the exact percentage if multiple entries have the same threshold, the script chooses to retain more than the given percentage)
  -threshold : threshold to prune (score < threshold equals prune entry), do not use this if percentage is specified
For any questions contact lingwang at cs dot cmu dot edu
";
  exit(1);
}


my $THRESHOLD = $threshold;
if ($percentage != -1){
   $THRESHOLD = &get_threshold_by_percentage($percentage);
}

my $ZCAT = "gzip -cd";
my $BZCAT = "bzcat";

&prune_by_threshold($THRESHOLD);

sub prune_by_threshold {
   my $th = $_[0];
   print STDERR "pruning using threshold $th \n";
   open (SCORE_READER, &open_compressed($scores_file));
   open (TABLE_READER, &open_compressed($table));
   $number_of_phrases=0;
   $number_of_unpruned_phrases=0;
   while(!eof(SCORE_READER) && !eof(TABLE_READER)){
      $score_line = <SCORE_READER>;
      $table_line = <TABLE_READER>;
      chomp($score_line);
      if($score_line >= $th){
        print $table_line;
        $number_of_unpruned_phrases++;
      }
      $number_of_phrases++;
   }
   print STDERR "pruned ".($number_of_phrases - $number_of_unpruned_phrases)." phrase pairs out of $number_of_phrases\n";
}

sub get_threshold_by_percentage {
   my $percentage = $_[0];
   $ret = 0;

   $number_of_phrases = &get_number_of_phrases();
   $stop_phrase = ($percentage * $number_of_phrases) / 100;
   $phrase_number = 0;

   
   open (SCORE_READER, &open_compressed($scores_file));
   while(<SCORE_READER>) {
       my $line = $_;

   }
   close (SCORE_READER);

   open (SCORE_READER, "cat $scores_file | LC_ALL=c sort -g |");
   while(<SCORE_READER>) {
      my $line = $_;
      if($phrase_number >= $stop_phrase){
	 chomp($line);
         $ret = $line;
         last;
      }
      $phrase_number++;
   }
   
   close (SCORE_READER);
   return $ret;
}

sub get_number_of_phrases {
   $ret = 0;
   open (SCORE_READER, $scores_file);

   while(<SCORE_READER>) {
      $ret++;
   }
   
   close (SCORE_READER);
   return $ret;
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
