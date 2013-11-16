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
my $min_score = undef;
my $opt_min_non_initial_rule_count = undef;
my $opt_gzip = 1; # gzip output files (so far only phrase-based ttable until someone tests remaining models and formats)
my $opt_filter = 1; # enables skipping of filtering - useful for conf net or lattice

GetOptions(
    "gzip!" => \$opt_gzip,
    "filter!" => \$opt_filter,
    "Hierarchical" => \$opt_hierarchical,
    "Binarizer=s" => \$binarizer,
    "MinScore=s" => \$min_score,
    "MinNonInitialRuleCount=i" => \$opt_min_non_initial_rule_count
) or exit(1);

# get command line parameters
my $dir = shift;
my $config = shift;
my $input = shift;

if (!defined $dir || !defined $config || !defined $input) {
  print STDERR "usage: filter-model-given-input.pl targetdir moses.ini input.text [-Binarizer binarizer] [-Hierarchical] [-MinScore id:threshold[,id:threshold]*]\n";
  exit 1;
}
$dir = ensure_full_path($dir);

# decode min-score definitions
my %MIN_SCORE;
if ($min_score) {
  foreach (split(/ *, */,$min_score)) {
    my ($id,$score) = split(/ *: */);
    $MIN_SCORE{$id} = $score;
    print STDERR "score $id must be at least $score\n";
  }
}
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

my $inputStrippedXML = "$dir/input.$$";
my $cmd = "$RealBin/../generic/strip-xml.perl < $input > $inputStrippedXML";
print STDERR "Stripping XML: $cmd \n";
print `$cmd`;
$input = $inputStrippedXML;

# get tables to be filtered (and modify config file)
my (@TABLE,@TABLE_FACTORS,@TABLE_NEW_NAME,%CONSIDER_FACTORS,%KNOWN_TTABLE,@TABLE_WEIGHTS,%TABLE_NUMBER);

my %new_name_used = ();
open(INI_OUT,">$dir/moses.ini") or die "Can't write $dir/moses.ini";
open(INI,$config) or die "Can't read $config";
while(my $line = <INI>) {
  chomp($line);
  my @toks = split(/ /, $line);
  if ($line =~ /PhraseDictionaryMemory /
     || $line =~ /PhraseDictionaryBinary /
     || $line =~ /PhraseDictionaryOnDisk /
     || $line =~ /PhraseDictionarySCFG /
     ) {
    print STDERR "pt:$line\n";

		my ($phrase_table_impl,$source_factor,$t,$w,$file,$table_flag,$skip); # = ($1,$2,$3,$4,$5,$6,$7);
    $table_flag = "";
    $phrase_table_impl = $toks[0];
    $skip = 0;
    
    for (my $i = 1; $i < scalar(@toks); ++$i) {
      my @args = split(/=/, $toks[$i]);
      chomp($args[0]);
      chomp($args[1]);

      if ($args[0] eq "num-features") {
			  $w = $args[1];
			}
			elsif ($args[0] eq "input-factor") {
			  $source_factor = $args[1];
			}
			elsif ($args[0] eq "output-factor") {
			  $t = $args[1];
			}
			elsif ($args[0] eq "path") {
			  $file = $args[1];
			}
			elsif ($args[0] eq "filterable" && $args[1] eq "false") {
			  $skip = 1;
			}
    } #for (my $i = 1; $i < scalar(@toks); ++$i) {
    
		if (($phrase_table_impl ne "PhraseDictionaryMemory" && $phrase_table_impl ne "PhraseDictionarySCFG") || $file =~ /glue-grammar/ || $skip) {
				# Only Memory ("0") and NewFormat ("6") can be filtered.
				print INI_OUT "$line\n";
				next;
		}

		push @TABLE, $file;
		push @TABLE_WEIGHTS,$w;
		$KNOWN_TTABLE{$#TABLE}++;

  	my $new_name = "$dir/phrase-table.$source_factor-$t.".(++$TABLE_NUMBER{"$source_factor-$t"});
		my $cnt = 1;
		$cnt ++ while (defined $new_name_used{"$new_name.$cnt"});
		$new_name .= ".$cnt";
		$new_name_used{$new_name} = 1;
		if ($binarizer && $phrase_table_impl eq "PhraseDictionarySCFG") {
		  $phrase_table_impl = "PhraseDictionaryOnDisk";
		  @toks = set_value(\@toks, "path", "$new_name.bin$table_flag");
		}
		elsif ($binarizer && $phrase_table_impl eq "PhraseDictionaryMemory") {
			if ($binarizer =~ /processPhraseTableMin/) {
  		  $phrase_table_impl = "PhraseDictionaryCompact";
  		  @toks = set_value(\@toks, "path", "$new_name$table_flag");
			}
			elsif ($binarizer =~ /CreateOnDiskPt/) {
  		  $phrase_table_impl = "PhraseDictionaryOnDisk";
		  @toks = set_value(\@toks, "path", "$new_name.bin$table_flag");
			}
			else {
  		  $phrase_table_impl = "PhraseDictionaryBinary";
			  @toks = set_value(\@toks, "path", "$new_name$table_flag");
			}
		}
		else {
			$new_name .= ".gz" if $opt_gzip;
		  @toks = set_value(\@toks, "path", "$new_name$table_flag");
		}

    $toks[0] = $phrase_table_impl;

    print INI_OUT join_array(\@toks)."\n";

		push @TABLE_NEW_NAME,$new_name;

		$CONSIDER_FACTORS{$source_factor} = 1;
			print STDERR "Considering factor $source_factor\n";
		push @TABLE_FACTORS, $source_factor;
		
  } #if (/PhraseModel /) {
  elsif ($line =~ /LexicalReordering /) {
    print STDERR "ro:$line\n";
		my ($source_factor, $t, $w, $file); # = ($1,$2,$3,$4);

    for (my $i = 1; $i < scalar(@toks); ++$i) {
      my @args = split(/=/, $toks[$i]);
      chomp($args[0]);
      chomp($args[1]);
      
			if ($args[0] eq "num-features") {
			  $w = $args[1];
			}
			elsif ($args[0] eq "input-factor") {
			  $source_factor = chomp($args[1]);
			}
			elsif ($args[0] eq "output-factor") {
			  #$t = chomp($args[1]);
			}
			elsif ($args[0] eq "type") {
			  $t = $args[1];
			}
			elsif ($args[0] eq "path") {
			  $file = $args[1];
			}

		} # for (my $i = 1; $i < scalar(@toks); ++$i) {
		
  	push @TABLE, $file;
	push @TABLE_WEIGHTS,$w;
		
		$file =~ s/^.*\/+([^\/]+)/$1/g;
		my $new_name = "$dir/$file";
		$new_name =~ s/\.gz//;
		
		#print INI_OUT "$source_factor $t $w $new_name\n";
	  @toks = set_value(\@toks, "path", "$new_name");
	  print INI_OUT join_array(\@toks)."\n";

		push @TABLE_NEW_NAME,$new_name;

		$CONSIDER_FACTORS{$source_factor} = 1;
			print STDERR "Considering factor $source_factor\n";
		push @TABLE_FACTORS,$source_factor;

		
  } #elsif (/LexicalReordering /) {
  else {
    print INI_OUT "$line\n";  
  }
} # while(<INI>) {
close(INI);
close(INI_OUT);

my %TMP_INPUT_FILENAME;

if ($opt_hierarchical) {
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
	} # foreach my $key (keys %CONSIDER_FACTORS) {
} #if ($opt_hierarchical) {

my %PHRASE_USED;
if ($opt_filter) {
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
}

# filter files
for(my $i=0;$i<=$#TABLE;$i++) {
    my ($used,$total) = (0,0);
    my $file = $TABLE[$i];
    my $factors = $TABLE_FACTORS[$i];
    my $new_file = $TABLE_NEW_NAME[$i];
    print STDERR "filtering $file -> $new_file...\n";
    my $mid_file = $new_file; # used when both filtering and binarizing
    if (!$opt_filter) {
      # check if original file was gzipped
      if ($file !~ /\.gz$/ && -e "$file.gz") {
        $file .= ".gz";
      }
      $mid_file .= ".gz" if $file =~ /\.gz$/;
      safesystem("ln -s $file $mid_file");
    } else {

      $mid_file .= ".gz"
        if $mid_file !~ /\.gz/
           && $binarizer && $binarizer =~ /processPhraseTable/;

      my $openstring = mk_open_string($file);

      my $mid_openstring;
      if ($mid_file =~ /\.gz$/) {
        $mid_openstring = "| gzip -c > $mid_file";
      } else {
        $mid_openstring = ">$mid_file";
      }


      open(FILE_OUT,$mid_openstring) or die "Can't write to $mid_openstring";

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
                  # handle min_score thresholds
                  if ($min_score) {
                     my @ITEM = split(/ *\|\|\| */,$rest);
                     if(scalar (@ITEM)>2) { # do not filter reordering table
                       my @SCORE = split(/ /,$ITEM[1]);
                       my $okay = 1;
                       foreach my $id (keys %MIN_SCORE) {
                         $okay = 0 if $SCORE[$id] < $MIN_SCORE{$id};
                       }
                       next unless $okay;
                     }
                  }
                  print FILE_OUT $entry;
                  $used++;
              }
              $total++;
          }
          close(FILE);
          die "No phrases found in $file!" if $total == 0;
          printf STDERR "$used of $total phrases pairs used (%.2f%s) - note: max length $MAX_LENGTH\n",(100*$used/$total),'%';
      }

      close(FILE_OUT);

    }

    my $catcmd = ($mid_file =~ /\.gz$/ ? "$ZCAT" : "cat");
    if(defined($binarizer)) {
      print STDERR "binarizing...";
      # translation model
      if ($KNOWN_TTABLE{$i}) {
        # ... hierarchical translation model
        if ($opt_hierarchical) {
          my $cmd = "$binarizer $mid_file $new_file.bin";
          print STDERR $cmd."\n";
          print STDERR `$cmd`;
        }
        # ... phrase translation model
        elsif ($binarizer =~ /processPhraseTableMin/) {
          #compact phrase table
          my $cmd = "$catcmd $mid_file | LC_ALL=C sort -T $dir > $mid_file.sorted; $binarizer -in $mid_file.sorted -out $new_file -nscores $TABLE_WEIGHTS[$i]; rm $mid_file.sorted";
          print STDERR $cmd."\n";
          print STDERR `$cmd`;
        } elsif ($binarizer =~ /CreateOnDiskPt/) {
      	  my $cmd = "$binarizer $mid_file $new_file.bin";
          print STDERR $cmd."\n";
          print STDERR `$cmd`;
        } else { 
          my $cmd = "$catcmd $mid_file | LC_ALL=C sort -T $dir | $binarizer -ttable 0 0 - -nscores $TABLE_WEIGHTS[$i] -out $new_file";
          print STDERR $cmd."\n";
          print STDERR `$cmd`;
        }
      }
      # reordering model
      else {
        my $lexbin;
        $lexbin = $binarizer;
        if ($binarizer =~ /CreateOnDiskPt/) {
          $lexbin =~ s/CreateOnDiskPt/processLexicalTable/;
        }

        $lexbin =~ s/PhraseTable/LexicalTable/;
        my $cmd;
        if ($lexbin =~ /processLexicalTableMin/) {
          $cmd = "$catcmd $mid_file | LC_ALL=C sort -T $dir > $mid_file.sorted;  $lexbin -in $mid_file.sorted -out $new_file; rm $mid_file.sorted";
        } else {
          $lexbin =~ s/^\s*(\S+)\s.+/$1/; # no options
          $cmd = "$lexbin -in $mid_file -out $new_file";
        }
        print STDERR $cmd."\n";
        print STDERR `$cmd`;
      }
    }
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

# functions
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

sub join_array {
  my @outside = @{$_[0]};
   
  my $ret = "";
  for (my $i = 0; $i < scalar(@outside); ++$i) {
    my $tok = $outside[$i];    
    $ret .= "$tok ";
  }
  
  return $ret;
}

sub set_value {
  my @arr = @{$_[0]};
  my $keySought = $_[1];
  my $newValue = $_[2];

  for (my $i = 1; $i < scalar(@arr); ++$i) {
    my @inside = split(/=/, $arr[$i]);

		my $key = $inside[0];
		if ($key eq $keySought) {
		  $arr[$i] = "$key=$newValue";
		  return @arr;
		}
  }
  return @arr;
}


