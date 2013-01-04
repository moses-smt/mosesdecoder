#!/usr/bin/perl -w

# $Id$
# Given a moses.ini file and an input text prepare minimized translation
# tables and a new moses.ini, so that loading of tables is much faster.

# original code by Philipp Koehn
# changes by Ondrej Bojar
# adapted for hierarchical models by Phil Williams

use strict;

use FindBin qw($RealBin);
use Getopt::Long;

my $SCRIPTS_ROOTDIR;
if (defined($ENV{"SCRIPTS_ROOTDIR"})) {
    $SCRIPTS_ROOTDIR = $ENV{"SCRIPTS_ROOTDIR"};
} else {
    $SCRIPTS_ROOTDIR = $RealBin;
    if ($SCRIPTS_ROOTDIR eq '') {
        $SCRIPTS_ROOTDIR = dirname(__FILE__);
    }
    $SCRIPTS_ROOTDIR =~ s/\/training$//;
    $ENV{"SCRIPTS_ROOTDIR"} = $SCRIPTS_ROOTDIR;
}

# consider phrases in input up to $MAX_LENGTH
# in other words, all phrase-tables will be truncated at least to 10 words per
# phrase.
my $MAX_LENGTH = 10;

# utilities
my $ZCAT = "gzip -cd";

# get optional parameters
my $opt_hierarchical = 0;
my $binarizer = undef;
my $opt_min_non_initial_rule_count = undef;
my $opt_gzip = 1; # gzip output files (so far only phrase-based ttable until someone tests remaining models and formats)

GetOptions(
    "gzip!" => \$opt_gzip,
    "Hierarchical" => \$opt_hierarchical,
    "Binarizer=s" => \$binarizer,
    "MinNonInitialRuleCount=i" => \$opt_min_non_initial_rule_count
) or exit(1);

# get command line parameters
my $dir = shift;
my $config = shift;
my $input = shift;

if (!defined $dir || !defined $config || !defined $input) {
  print STDERR "usage: filter-model-given-input.pl targetdir moses.ini input.text [-Binarizer binarizer] [-Hierarchical]\n";
  exit 1;
}
$dir = ensure_full_path($dir);

# buggy directory in place?
if (-d $dir && ! -e "$dir/info") {
    print STDERR "The directory $dir already exists. Please delete $dir and rerun!\n";
    exit(1);
}

# already filtered? check if it can be re-used
if (-d $dir) {
    my @INFO = `cat $dir/info`;
    chop(@INFO);
    if($INFO[0] ne $config 
       || ($INFO[1] ne $input && 
	   $INFO[1].".tagged" ne $input)) {
      print STDERR "WARNING: directory exists but does not match parameters:\n";
      print STDERR "  ($INFO[0] ne $config || $INFO[1] ne $input)\n";
      exit 1;
    }
    print STDERR "The filtered model was ready in $dir, not doing anything.\n";
    exit 0;
}


# filter the translation and distortion tables
safesystem("mkdir -p $dir") or die "Can't mkdir $dir";

# get tables to be filtered (and modify config file)
my (@TABLE,@TABLE_FACTORS,@TABLE_NEW_NAME,%CONSIDER_FACTORS,%KNOWN_TTABLE,@TABLE_WEIGHTS,%TABLE_NUMBER);
my %new_name_used = ();
open(INI_OUT,">$dir/moses.ini") or die "Can't write $dir/moses.ini";
open(INI,$config) or die "Can't read $config";
while(<INI>) {
    print INI_OUT $_;
    if (/ttable-file\]/) {
      while(1) {	       
    	my $table_spec = <INI>;
    	if ($table_spec !~ /^(\d+) ([\d\,\-]+) ([\d\,\-]+) (\d+) (\S+)( \S+)?$/) {
    	    print INI_OUT $table_spec;
    	    last;
    	}
    	my ($phrase_table_impl,$source_factor,$t,$w,$file,$table_flag) = ($1,$2,$3,$4,$5,$6);
      $table_flag = "" if (!defined($table_flag));

        if (($phrase_table_impl ne "0" && $phrase_table_impl ne "6") || $file =~ /glue-grammar/) {
            # Only Memory ("0") and NewFormat ("6") can be filtered.
            print INI_OUT $table_spec;
            next;
        }

    	chomp($file);
    	push @TABLE, $file;
	push @TABLE_WEIGHTS,$w;
	$KNOWN_TTABLE{$#TABLE}++;

    	my $new_name = "$dir/phrase-table.$source_factor-$t.".(++$TABLE_NUMBER{"$source_factor-$t"});
        my $cnt = 1;
        $cnt ++ while (defined $new_name_used{"$new_name.$cnt"});
        $new_name .= ".$cnt";
        $new_name_used{$new_name} = 1;
	if ($binarizer && $phrase_table_impl == 6) {
    	  print INI_OUT "2 $source_factor $t $w $new_name.bin$table_flag\n";
        }
        elsif ($binarizer && $phrase_table_impl == 0) {
          if ($binarizer =~ /processPhraseTableMin/) {
            print INI_OUT "12 $source_factor $t $w $new_name$table_flag\n";
          } else {
    	    print INI_OUT "1 $source_factor $t $w $new_name$table_flag\n";
          }
        } else {
          $new_name .= ".gz" if $opt_gzip;
    	    print INI_OUT "$phrase_table_impl $source_factor $t $w $new_name$table_flag\n";
        }
    	push @TABLE_NEW_NAME,$new_name;

    	$CONSIDER_FACTORS{$source_factor} = 1;
        print STDERR "Considering factor $source_factor\n";
    	push @TABLE_FACTORS, $source_factor;
      }
    }
    elsif (/distortion-file/) {
        while(1) {
    	  my $table_spec = <INI>;
    	  if ($table_spec !~ /^([\d\,\-]+) (\S+) (\d+) (\S+)$/) {
    	      print INI_OUT $table_spec;
    	      last;
    	}
    	my ($factors,$t,$w,$file) = ($1,$2,$3,$4);
	my $source_factor = $factors;
	$source_factor =~ s/\-[\d,]+$//;

    	chomp($file);
    	push @TABLE,$file;

    	$file =~ s/^.*\/+([^\/]+)/$1/g;
    	my $new_name = "$dir/$file";
	    $new_name =~ s/\.gz//;
    	print INI_OUT "$factors $t $w $new_name\n";
    	push @TABLE_NEW_NAME,$new_name;

    	$CONSIDER_FACTORS{$source_factor} = 1;
        print STDERR "Considering factor $source_factor\n";
    	push @TABLE_FACTORS,$source_factor;
        }
    }
}
close(INI);
close(INI_OUT);

my %TMP_INPUT_FILENAME;

if ($opt_hierarchical)
{
    # Write a separate, temporary input file for each combination of source
    # factors
    foreach my $key (keys %CONSIDER_FACTORS) {
        my $filename = "$dir/input-$key";
        open(FILEHANDLE,">$filename") or die "Can't open $filename for writing";
        $TMP_INPUT_FILENAME{$key} = $filename;
        my @FACTOR = split(/,/, $key);
        open(PIPE,"$SCRIPTS_ROOTDIR/training/reduce_combine.pl $input @FACTOR |");
        while (my $line = <PIPE>) {
            print FILEHANDLE $line
        }
        close(FILEHANDLE);
    }
}

my %PHRASE_USED;
if (!$opt_hierarchical) {
    # get the phrase pairs appearing in the input text, up to the $MAX_LENGTH
    open(INPUT,mk_open_string($input)) or die "Can't read $input";
    while(my $line = <INPUT>) {
        chomp($line);
        my @WORD = split(/ +/,$line);
        for(my $i=0;$i<=$#WORD;$i++) {
            for(my $j=0;$j<$MAX_LENGTH && $j+$i<=$#WORD;$j++) {
                foreach (keys %CONSIDER_FACTORS) {
                    my @FACTOR = split(/,/);
                    my $phrase = "";
                    for(my $k=$i;$k<=$i+$j;$k++) {
                        my @WORD_FACTOR = split(/\|/,$WORD[$k]);
                        for(my $f=0;$f<=$#FACTOR;$f++) {
                            $phrase .= $WORD_FACTOR[$FACTOR[$f]]."|";
                        }
                        chop($phrase);
                        $phrase .= " ";
                    }
                    chop($phrase);
                    $PHRASE_USED{$_}{$phrase}++;
                }
            }
        }
    }
    close(INPUT);
}

sub mk_open_string {
  my $file = shift;
  my $openstring;
  if ($file !~ /\.gz$/ && -e "$file.gz") {
    $openstring = "$ZCAT $file.gz |";
  } elsif ($file =~ /\.gz$/) {
    $openstring = "$ZCAT $file |";
  } elsif ($opt_hierarchical) {
    $openstring = "cat $file |";
  } else {
    $openstring = "< $file";
  }
  return $openstring;
}


# filter files
for(my $i=0;$i<=$#TABLE;$i++) {
    my ($used,$total) = (0,0);
    my $file = $TABLE[$i];
    my $factors = $TABLE_FACTORS[$i];
    my $new_file = $TABLE_NEW_NAME[$i];
    print STDERR "filtering $file -> $new_file...\n";

    my $openstring = mk_open_string($file);

    my $new_openstring;
    if ($new_file =~ /\.gz$/) {
      $new_openstring = "| gzip -c > $new_file";
    } else {
      $new_openstring = ">$new_file";
    }

    open(FILE_OUT,$new_openstring) or die "Can't write to $new_openstring";

    if ($opt_hierarchical) {
        my $tmp_input = $TMP_INPUT_FILENAME{$factors};
        my $options = "";
        $options .= "--min-non-initial-rule-count=$opt_min_non_initial_rule_count" if defined($opt_min_non_initial_rule_count);
        open(PIPE,"$openstring $SCRIPTS_ROOTDIR/training/filter-rule-table.py $options $tmp_input |");
        while (my $line = <PIPE>) {
            print FILE_OUT $line
        }
        close(FILEHANDLE);
    } else {
        open(FILE,$openstring) or die "Can't open '$openstring'";
        while(my $entry = <FILE>) {
            my ($foreign,$rest) = split(/ \|\|\| /,$entry,2);
            $foreign =~ s/ $//;
            if (defined($PHRASE_USED{$factors}{$foreign})) {
                print FILE_OUT $entry;
                $used++;
            }
            $total++;
        }
        close(FILE);
        die "No phrases found in $file!" if $total == 0;
        printf STDERR "$used of $total phrases pairs used (%.2f%s) - note: max length $MAX_LENGTH\n",(100*$used/$total),'%';
    }

    if(defined($binarizer)) {
      print STDERR "binarizing...";
      # translation model
      if ($KNOWN_TTABLE{$i}) {
        # ... hierarchical translation model
        if ($opt_hierarchical) {
          my $cmd = "$binarizer $new_file $new_file.bin";
          print STDERR $cmd."\n";
          print STDERR `$cmd`;
        }
        # ... phrase translation model
        elsif ($binarizer =~ /processPhraseTableMin/) {
          #compact phrase table
          my $cmd = "LC_ALL=C sort -T $dir $new_file > $new_file.sorted; $binarizer -in $new_file.sorted -out $new_file -nscores $TABLE_WEIGHTS[$i]; rm $new_file.sorted";
          print STDERR $cmd."\n";
          print STDERR `$cmd`;
        } else { 
          my $cmd = "cat $new_file | LC_ALL=C sort -T $dir | $binarizer -ttable 0 0 - -nscores $TABLE_WEIGHTS[$i] -out $new_file";
          print STDERR $cmd."\n";
          print STDERR `$cmd`;
        }
      }
      # reordering model
      else {
        my $lexbin = $binarizer; 
        $lexbin =~ s/PhraseTable/LexicalTable/;
        my $cmd;
        if ($lexbin =~ /processLexicalTableMin/) {
          $cmd = "LC_ALL=C sort -T $dir $new_file > $new_file.sorted;  $lexbin -in $new_file.sorted -out $new_file; rm $new_file.sorted";
        } else {
          $lexbin =~ s/^\s*(\S+)\s.+/$1/; # no options
          $cmd = "$lexbin -in $new_file -out $new_file";
        }
        print STDERR $cmd."\n";
        print STDERR `$cmd`;
      }
    }

    close(FILE_OUT);
}

if ($opt_hierarchical)
{
    # Remove the temporary input files
    unlink values %TMP_INPUT_FILENAME;
}

open(INFO,">$dir/info");
print INFO "$config\n$input\n";
close(INFO);


print "To run the decoder, please call:
  moses -f $dir/moses.ini -i $input\n";

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
      exit(1);
  }
  else {
    my $exitcode = $? >> 8;
    print STDERR "Exit code: $exitcode\n" if $exitcode;
    return ! $exitcode;
  }
}
sub ensure_full_path {
    my $PATH = shift;
    return $PATH if $PATH =~ /^\//;
    my $dir = `pawd 2>/dev/null`;
    if (!$dir) {$dir = `pwd`;}
    chomp $dir;
    $PATH = $dir."/".$PATH;
    $PATH =~ s/[\r\n]//g;
    $PATH =~ s/\/\.\//\//g;
    $PATH =~ s/\/+/\//g;
    my $sanity = 0;
    while($PATH =~ /\/\.\.\// && $sanity++<10) {
        $PATH =~ s/\/+/\//g;
        $PATH =~ s/\/[^\/]+\/\.\.\//\//g;
    }
    $PATH =~ s/\/[^\/]+\/\.\.$//;
    $PATH =~ s/\/+$//;
    return $PATH;
}

