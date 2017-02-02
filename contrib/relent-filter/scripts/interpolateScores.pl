#!/usr/bin/perl -w
use Getopt::Long;
use File::Basename;
use POSIX;

$operation="+";

# read arguments
$_HELP = 1 if (@ARGV < 1 or !GetOptions ("files=s" => \$files, #moses conf file
"weights=s" => \$weights,
"operation=s" => \$operation)); #directory to put all the output files


# help message if arguments are not correct
if ($_HELP) {
    print "Relative Entropy Pruning
Usage: perl interpolateScores.pl [PARAMS]
Function: interpolates any number of score files interlated by their weights 
Authors: Wang Ling ( lingwang at cs dot cmu dot edu )
PARAMS:
  -files=s : table files to interpolate separated by a space (Ex \"file1 file2 file3\")
  -weights : interpolation weights separated by a space (Ex \"0.3 0.3 0.4\")
  -operation : +,* or min depending on the operation to perform to combine scores
For any questions contact lingwang at cs dot cmu dot edu
";
  exit(1);
}

@FILES = split(/\s+/, $files);
@WEIGHTS = split(/\s+/, $weights);

my $ZCAT = "gzip -cd";
my $BZCAT = "bzcat";

&interpolate();

sub interpolate {
    my @READERS;
    for($i = 0; $i < @FILES; $i++){
	local *FILE;
        open(FILE, &open_compressed($FILES[$i])) or die "ERROR: Can't read $FILES[$i]";
        push(@READERS, *FILE);
    }
    $FIRST = $READERS[0];
    while(!eof($FIRST)) {
        if($operation eq "+"){
	    my $score = 0;
            for($i = 0; $i < @FILES; $i++){
	        my $READER = $READERS[$i];
                my $line = <$READER>;
                chomp($line);
		$score += $line*$WEIGHTS[$i];
            }
	    print "$score\n";
	}
        if($operation eq "*"){
	    my $score = 1;
	    for($i = 0; $i < @FILES; $i++){
                my $READER = $READERS[$i];
                my $line = <$READER>;
                chomp($line);
                $score *= $line ** $WEIGHTS[$i];
            }
	    print "$score\n"
	}
	if($operation eq "min"){
	    my $score = 99999;
            for($i = 0; $i < @FILES; $i++){
                my $READER = $READERS[$i];
                my $line = <$READER>;
                chomp($line);
		if ($score > $line*$WEIGHTS[$i]){
		    $score = $line*$WEIGHTS[$i];
		}
            }
            print "$score\n"

	}
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
