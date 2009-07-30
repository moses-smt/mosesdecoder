#!/usr/bin/perl -w

# $Id$
# Given a moses.ini file and an input text prepare minimized translation
# tables and a new moses.ini, so that loading of tables is much faster.

# original code by Philipp Koehn
# changes by Ondrej Bojar
# adapted for hierarchical models by Philip James Williams

use strict;

use Getopt::Long;

my $opt_hierarchical = 0;
my $opt_max_nts = 2;
my $opt_max_span = 10;
my $opt_max_symbols_source = 5;
my $quick_hierarchical = 0;

GetOptions(
    "Hierarchical" => \$opt_hierarchical,
    "MaxNonTerm=i" => \$opt_max_nts,
    "MaxSpan=i" => \$opt_max_span,
    "MaxSymbolsSource=i" => \$opt_max_symbols_source,
    # FIXME Add NonTermConsecSource as we're implicitly filtering out rules
    # that contain consecutive NTs.
    # TODO Add NoNonTermFirstWord.
    # TODO Add MinWords.
) or exit(1);

$quick_hierarchical = 1 if $opt_hierarchical && ($opt_max_span>10 || $opt_max_symbols_source>5 || $opt_max_nts>2);

# consider phrases in input up to $MAX_LENGTH
# in other words, all non-hierarchical phrase-tables will be truncated at least
# to 10 words per phrase and hierarchical rules will be truncated to 5 symbols
# (by default).
# TODO Allow options to override $MAX_LENGTH for non-hierarchical translation
# tables.  Defaults should probably be different though(?)
my $MAX_LENGTH = 10;

my $dir = shift; 
my $config = shift;
my $input = shift;

if (!defined $dir || !defined $config || !defined $input) {
  print STDERR "usage: filter-model-given-input.pl targetdir moses.ini input.text\n";
  exit 1;
}

$dir = ensure_full_path($dir);

# buggy directory in place?
if (-d $dir && ! -e "$dir/info") {
    print STDERR "The directory $dir exists but does not belong to me. Delete $dir!\n";
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
my (@TABLE,@TABLE_FACTORS,@TABLE_NEW_NAME,%CONSIDER_FACTORS);
my %new_name_used = ();
open(INI_OUT,">$dir/moses.ini") or die "Can't write $dir/moses.ini";
open(INI,$config) or die "Can't read $config";
while(<INI>) {
    print INI_OUT $_;
    if (/ttable-file\]/) {
        while(1) {	       
    	my $table_spec = <INI>;
    	if ($table_spec !~ /^(\d+) ([\d\,\-]+) ([\d\,\-]+) (\d+) (\S+)$/) {
    	    print INI_OUT $table_spec;
    	    last;
    	}
    	my ($phrase_table_impl,$source_factor,$t,$w,$file) = ($1,$2,$3,$4,$5);

        # FIXME Need to handle other non-memory implementation types here.
        if ($phrase_table_impl eq "3") {  # Glue rule
            print INI_OUT $table_spec;
            next;
        }

    	chomp($file);
    	push @TABLE, $file;

    	my $new_name = "$dir/phrase-table.$source_factor-$t";
        my $cnt = 1;
        $cnt ++ while (defined $new_name_used{"$new_name.$cnt"});
        $new_name .= ".$cnt";
        $new_name_used{$new_name} = 1;
    	print INI_OUT "$phrase_table_impl $source_factor $t $w $new_name\n";
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


# get the phrases appearing in the input text, up to the $MAX_LENGTH, or
# if hierarchical, generate the source sides of all possible rules.
my %PHRASE_USED;
open(INPUT,$input) or die "Can't read $input";
while(my $line = <INPUT>) {
    chomp($line);
    $line =~ s/<\S[^>]+>//g;
    $line =~ s/^ +//;
    $line =~ s/ +$//;
    my @WORD = split(/ +/,$line);
    if ($opt_hierarchical) {
        # Generate the source side of every possible rule given the input
        # text, using "[NT]" to stand for any NT.
        for(my $start=0;$start<=$#WORD;$start++) {
            my $end = &min($#WORD,$start+$opt_max_span-1);
            my $PHRASES = &generate_phrases(\@WORD,$start,$end,
                                          $opt_max_symbols_source,$opt_max_nts);
            foreach my $PHRASE (@$PHRASES) {
                foreach (keys %CONSIDER_FACTORS) {
                    my @FACTOR = split(/,/);
                    my $phrase = "";
                    foreach my $symbol (@$PHRASE) {
                        if ($symbol eq "[NT]") {
                            $phrase .= $symbol;
                        } else {
                            my @WORD_FACTOR = split(/\|/,$symbol);
                            for(my $f=0;$f<=$#FACTOR;$f++) {
                                $phrase .= $WORD_FACTOR[$FACTOR[$f]]."|";
                            }
                            chop($phrase);
                        }
                        $phrase .= " ";
                    }
                    chop($phrase);
                    $PHRASE_USED{$_}{$phrase}++;
                }
            }
	}
    } else {
	my $max = $MAX_LENGTH;
	$max = $opt_max_symbols_source if $quick_hierarchical;
        for(my $i=0;$i<=$#WORD;$i++) {
            for(my $j=0;$j<$max && $j+$i<=$#WORD;$j++) {
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
}
close(INPUT);

# glue grammar phrases
if ($quick_hierarchical) {
    $PHRASE_USED{0}{"<s>"} ++;
    $PHRASE_USED{0}{"</s>"} ++;
}
elsif ($opt_hierarchical) {
    $PHRASE_USED{0}{"<s> [NT] </s>"} ++;
    $PHRASE_USED{0}{"<s> [NT]"} ++;
    $PHRASE_USED{0}{"[NT] </s>"} ++;
    $PHRASE_USED{0}{"<s>"} ++;
    $PHRASE_USED{0}{"</s>"} ++;	
    $PHRASE_USED{0}{"[NT] [NT]"} ++;
}

# filter files
for(my $i=0;$i<=$#TABLE;$i++) {
    my ($used,$total) = (0,0);
    my $file = $TABLE[$i];
    my $factors = $TABLE_FACTORS[$i];
    my $new_file = $TABLE_NEW_NAME[$i];
    print STDERR "filtering $file -> $new_file...\n";

    my $openstring;
    if ($file !~ /\.gz$/ && -e "$file.gz") {
      $openstring = "zcat $file.gz |";
    } elsif ($file =~ /\.gz$/) {
      $openstring = "zcat $file |";
    } else {
      $openstring = "< $file";
    }

    open(FILE,$openstring) or die "Can't open '$openstring'";
    open(FILE_OUT,">$new_file") or die "Can't write $new_file";

    while(my $entry = <FILE>) {
        my $use_entry;
	if ($quick_hierarchical) {
	    my ($lhs,$foreign,$rest) = split(/ \|\|\| /,$entry,3);
	    $use_entry = 1;
	    foreach my $subphrase (split(/\s*\[[^\]]+\]\s*/,$foreign)) {
		$use_entry = 0
		    if ($subphrase ne ""
			&& !defined($PHRASE_USED{$factors}{$subphrase}));
	    }
	}
        elsif ($opt_hierarchical) {
            my ($lhs,$foreign,$rest) = split(/ \|\|\| /,$entry,3);
            $foreign =~ s/^\s+//;
            $foreign =~ s/\s+$//;
            $foreign =~ s/\[[^\]]+\]/\[NT\]/g;
            $use_entry = defined($PHRASE_USED{$factors}{$foreign});
        } else {
            my ($foreign,$rest) = split(/ \|\|\| /,$entry,2);
            $foreign =~ s/ $//;
            $use_entry = defined($PHRASE_USED{$factors}{$foreign});
        }
        if ($use_entry) {
           print FILE_OUT $entry;
           $used++;
        }
        $total++;
    }
    close(FILE);
    close(FILE_OUT);
    die "No phrases found in $file!" if $total == 0;
    # FIXME Change message for $opt_hierarchical (doesn't use $MAX_LENGTH)
    printf STDERR "$used of $total phrases pairs used (%.2f%s) - note: max length $MAX_LENGTH\n",(100*$used/$total),'%';
}

open(INFO,">$dir/info");
print INFO "$config\n$input\n";
close(INFO);


print "To run the decoder, please call:
  moses -f $dir/moses.ini < $input\n";

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

sub min {
    my ($a,$b) = @_;
    return ($a < $b) ? $a : $b;
}

# Generate the source side phrase of every possible allowed rule from the
# sentence @$WORD, using "[NT]" to stand for any NT.
sub generate_phrases {
    my ($WORD,$start,$end,$max_symbols,$max_nts) = @_;

    my $w = &generate_phrases_w($WORD,$start,$end,$max_symbols,$max_nts);
    my $nt = &generate_phrases_nt($WORD,$start,$end,$max_symbols,$max_nts);

    return [@$w, @$nt];
}

# Generate all allowed hierarchical phrases that begin with a word
sub generate_phrases_w {
    my ($WORD,$start,$end,$max_symbols,$max_nts) = @_;

    return () if ($max_symbols < 1);

    my @PHRASES = ();
    my $max_words = &min($max_symbols,$end-$start+1);
    for (my $num_words=1;$num_words<=$max_words;$num_words++) {
        my @PHRASE = @$WORD[$start..$start+$num_words-1];
        push(@PHRASES,\@PHRASE);
        my $tails=&generate_phrases_nt($WORD,$start+$num_words,$end,
                                       $max_symbols-$num_words,$max_nts);
        foreach my $tail (@$tails) {
            push(@PHRASES,[@PHRASE,@$tail]);
        }
    }

    return \@PHRASES;
}

# Generate all allowed hierarchical phrases that begin with a NT
# FIXME Can't generate phrases containing consecutive NTs (which will be
# present if the --NonTermConsecSource option was given to the extractor).
sub generate_phrases_nt {
    my ($WORD,$start,$end,$max_symbols,$max_nts) = @_;

    return () if ($max_symbols < 1 || $max_nts < 1 || $end < $start);

    my @PHRASES = ();

    my $head = "[NT]";
    push(@PHRASES,[$head]);

    for (my $nt_span=1;$nt_span<=$end-$start;$nt_span++) {
        my $new_start = $start + $nt_span;
        my $tails = &generate_phrases_w($WORD,$new_start,$end,$max_symbols-1,
                                        $max_nts-1);
        foreach my $tail (@$tails) {
            push(@PHRASES,[$head,@$tail]);
        }
    }

    return \@PHRASES;
}
