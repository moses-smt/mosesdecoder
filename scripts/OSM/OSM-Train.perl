#!/usr/bin/perl -w

use strict;
use Getopt::Long "GetOptions";
use FindBin qw($RealBin);

print STDERR "Training OSM - Start\n".`date`;

my $ORDER = 5;
my $OUT_DIR = "/tmp/osm.$$";
my $___FACTOR_DELIMITER = "|";
my ($MOSES_SRC_DIR,$CORPUS_F,$CORPUS_E,$ALIGNMENT,$SRILM_DIR,$FACTOR,$LMPLZ);

# utilities
my $ZCAT = "gzip -cd";
my $BZCAT = "bzcat";

die("ERROR: wrong syntax when invoking OSM-Train.perl")
    unless &GetOptions('moses-src-dir=s' => \$MOSES_SRC_DIR,
		       'corpus-f=s' => \$CORPUS_F,
		       'corpus-e=s' => \$CORPUS_E,
		       'alignment=s' => \$ALIGNMENT,
		       'order=i' => \$ORDER,
		       'factor=s' => \$FACTOR,
		       'srilm-dir=s' => \$SRILM_DIR,
		       'lmplz=s' => \$LMPLZ,
		       'out-dir=s' => \$OUT_DIR);

# check if the files are in place
die("ERROR: you need to define --corpus-e, --corpus-f, --alignment, --srilm-dir or --lmplz, and --moses-src-dir") 
    unless (defined($MOSES_SRC_DIR) && 
	    defined($CORPUS_F) && 
	    defined($CORPUS_E) && 
	    defined($ALIGNMENT)&& 
	    (defined($SRILM_DIR) || defined($LMPLZ)));
die("ERROR: could not find input corpus file '$CORPUS_F'") 
    unless -e $CORPUS_F;
die("ERROR: could not find output corpus file '$CORPUS_E'") 
    unless -e $CORPUS_E;
die("ERROR: could not find algnment file '$ALIGNMENT'") 
    unless -e $ALIGNMENT;
die("ERROR: could not find OSM scripts in '$MOSES_SRC_DIR/scripts/OSM") 
    unless -e "$MOSES_SRC_DIR/scripts/OSM/flipAlignment.perl";

# create factors
`mkdir $OUT_DIR`;
`$MOSES_SRC_DIR/scripts/OSM/flipAlignment.perl $ALIGNMENT > $OUT_DIR/align`;

if (defined($FACTOR)) {
  
   my @factor_values = split(/\+/, $FACTOR);
 
    foreach my $factor_val (@factor_values) {
    `mkdir $OUT_DIR/$factor_val`;
  my ($factor_f,$factor_e) = split(/\-/,$factor_val);
    
    $CORPUS_F =~ /^(.+)\.([^\.]+)/;
    my ($corpus_stem_f,$ext_f) = ($1,$2);
    $CORPUS_E =~ /^(.+)\.([^\.]+)/;
    my ($corpus_stem_e,$ext_e) = ($1,$2);
    &reduce_factors($CORPUS_F,"$corpus_stem_f.$factor_val.$ext_f",$factor_f);
    &reduce_factors($CORPUS_E,"$corpus_stem_e.$factor_val.$ext_e",$factor_e);

    `ln -s $corpus_stem_f.$factor_val.$ext_f $OUT_DIR/$factor_val/f`;
    `ln -s $corpus_stem_e.$factor_val.$ext_e $OUT_DIR/$factor_val/e`;
     create_model($factor_val);
  }
}
else {
    `ln -s $CORPUS_F $OUT_DIR/f`;
    `ln -s $CORPUS_E $OUT_DIR/e`;
     create_model("");	
}

# create model

print "Training OSM - End".`date`;

sub create_model{
my ($factor_val) = @_;

print "Creating Model ".$factor_val."\n";

print "Extracting Singletons\n";
`$MOSES_SRC_DIR/scripts/OSM/extract-singletons.perl $OUT_DIR/$factor_val/e $OUT_DIR/$factor_val/f $OUT_DIR/align > $OUT_DIR/$factor_val/Singletons`;

print "Converting Bilingual Sentence Pair into Operation Corpus\n";
`$MOSES_SRC_DIR/bin/generateSequences $OUT_DIR/$factor_val/e $OUT_DIR/$factor_val/f $OUT_DIR/align $OUT_DIR/$factor_val/Singletons > $OUT_DIR/$factor_val/opCorpus`;

print "Learning Operation Sequence Translation Model\n";
if (defined($LMPLZ)) {
  `$LMPLZ --order $ORDER --text $OUT_DIR/$factor_val/opCorpus --arpa $OUT_DIR/$factor_val/operationLM --prune 0 0 1`;
}
else {
  `$SRILM_DIR/ngram-count -kndiscount -order $ORDER -unk -text $OUT_DIR/$factor_val/opCorpus -lm $OUT_DIR/$factor_val/operationLM`;
}

print "Binarizing\n";
`$MOSES_SRC_DIR/bin/build_binary $OUT_DIR/$factor_val/operationLM $OUT_DIR/$factor_val/operationLM.bin`;


}

# from train-model.perl
sub reduce_factors {
    my ($full,$reduced,$factors) = @_;

    my @INCLUDE = sort {$a <=> $b} split(/,/,$factors);

    print "Reducing factors to produce $reduced  @ ".`date`;
    while(-e $reduced.".lock") {
	sleep(10);
    }
    if (-e $reduced) {
        print STDERR "  $reduced in place, reusing\n";
        return;
    }
    if (-e $reduced.".gz") {
        print STDERR "  $reduced.gz in place, reusing\n";
        return;
    }

    # peek at input, to check if we are asked to produce exactly the
    # available factors
    my $inh = open_or_zcat($full);
    my $firstline = <$inh>;
    die "Corpus file $full is empty" unless $firstline;
    close $inh;
    # pick first word
    $firstline =~ s/^\s*//;
    $firstline =~ s/\s.*//;
    # count factors
    my $maxfactorindex = $firstline =~ tr/|/|/;
    if (join(",", @INCLUDE) eq join(",", 0..$maxfactorindex)) {
	# create just symlink; preserving compression
	my $realfull = $full;
	if (!-e $realfull && -e $realfull.".gz") {
            $realfull .= ".gz";
            $reduced =~ s/(\.gz)?$/.gz/;
	}
	safesystem("ln -s '$realfull' '$reduced'")
            or die "Failed to create symlink $realfull -> $reduced";
	return;
    }

    # The default is to select the needed factors
    `touch $reduced.lock`;
    *IN = open_or_zcat($full);
    open(OUT,">".$reduced) or die "ERROR: Can't write $reduced";
    my $nr = 0;
    while(<IN>) {
        $nr++;
        print STDERR "." if $nr % 10000 == 0;
        print STDERR "($nr)" if $nr % 100000 == 0;
	chomp; s/ +/ /g; s/^ //; s/ $//;
	my $first = 1;
	foreach (split) {
	    my @FACTOR = split /\Q$___FACTOR_DELIMITER/;
              # \Q causes to disable metacharacters in regex
	    print OUT " " unless $first;
	    $first = 0;
	    my $first_factor = 1;
            foreach my $outfactor (@INCLUDE) {
              print OUT "|" unless $first_factor;
              $first_factor = 0;
              my $out = $FACTOR[$outfactor];
              die "ERROR: Couldn't find factor $outfactor in token \"$_\" in $full LINE $nr" if !defined $out;
              print OUT $out;
            }
	} 
	print OUT "\n";
    }
    print STDERR "\n";
    close(OUT);
    close(IN);
    `rm -f $reduced.lock`;
}

sub open_or_zcat {
  my $fn = shift;
  my $read = $fn;
  $fn = $fn.".gz" if ! -e $fn && -e $fn.".gz";
  $fn = $fn.".bz2" if ! -e $fn && -e $fn.".bz2";
  if ($fn =~ /\.bz2$/) {
      $read = "$BZCAT $fn|";
  } elsif ($fn =~ /\.gz$/) {
      $read = "$ZCAT $fn|";
  }
  my $hdl;
  open($hdl,$read) or die "Can't read $fn ($read)";
  return $hdl;
}

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