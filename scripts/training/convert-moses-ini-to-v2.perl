#!/usr/bin/perl -w

use strict;

my $header = "";
my @INI = <STDIN>;

my %TTABLE_IMPLEMENTATION = ( 0 => "PhraseDictionaryMemory",
			      1 => "PhraseDictionaryBinary" ,
			      6 => "PhraseDictionaryMemory");
my %LM_IMPLEMENTATION = ( 0 => "SRILM",
			  8 => "KENLM lazyken=0" );


my (%FEATURE,%WEIGHT);
my $i=0;
my ($has_sparse_ttable_features,$sparse_weight_file) = (0);

for(; $i<scalar(@INI); $i++) {
    my $line = $INI[$i];
    if ($line =~ /^\[(.+)\]/) {
	my $section = $1;
	if ($section eq "ttable-file" ||
	    $section eq "distortion-file" ||
	    $section eq "generation-file" ||
	    $section eq "lmodel-file" || 
	    $section eq "ttable-limit" ||
	    $section eq "target-word-insertion-feature" ||
	    $section eq "source-word-deletion-feature" ||
	    $section eq "word-translation-feature" ||
	    $section eq "phrase-length-feature") {
	    $FEATURE{$section} = &get_data();
	}
	elsif ($section eq "weight-file") {
	  print $header.$line;
	  my $WEIGHT_FILE = &get_data();
	  $sparse_weight_file = $$WEIGHT_FILE[0];
	  $has_sparse_ttable_features = `cat $sparse_weight_file | grep ^stm | wc -l`;
	  if ($has_sparse_ttable_features) {
	    print STDERR "sparse weight feature file has translaton model features\n -> creating new sparse weight file '$sparse_weight_file.new'\n";
	    print "$sparse_weight_file.new\n";
	  }
	  else {
	    print "$sparse_weight_file\n";
	  }
	}
	elsif ($section =~ /weight-(.+)/ && $section ne "weight-file") {
	    $WEIGHT{$1} = &get_data();
	}
	elsif ($section eq "report-sparse-features") {
    &get_data(); # ignore
  } 
	else {
	    print STDERR "include section [$section] verbatim.\n";
	    print $header.$line;
	    my $SECTION = &get_data();
	    foreach (@{$SECTION}) {
		print $_."\n";
	    }
	}
	$header = "";
    }
    else {
	$header .= $line;
    }
}
print $header;

if ($has_sparse_ttable_features) {
  open(SPARSE,$sparse_weight_file);
  open(NEW,">$sparse_weight_file.new");
  while(<SPARSE>) {
    if (!/^stm/) {
      print NEW $_;
    }
    else {
      s/^stm//;
      for (my $i=0;$i<scalar@{$FEATURE{"ttable-file"}};$i++) {
	print NEW "TranslationModel$i$_";
      }
    }
  }
  close(NEW);
  close(SPARSE);
}

my ($feature,$weight) = ("","");
$feature .= "UnknownWordPenalty\n";
$weight .= "UnknownWordPenalty0= 1\n";

$feature .= "WordPenalty\n";
$weight .= "WordPenalty0= ".$WEIGHT{"w"}[0]."\n";

$feature .= "Distortion\n";
$weight .= "Distortion0= ".$WEIGHT{"d"}[0]."\n";

foreach my $section (keys %FEATURE) {
    if ($section eq "phrase-length-feature") {
	$feature .= "PhraseLengthFeature name=pl\n";
    }
    elsif ($section eq "target-word-insertion-feature") {
	my ($factor,$file) = split(/ /,$FEATURE{$section}[0]);
	$feature .= "TargetWordInsertionFeature name=twi factor=$factor";
	$feature .= " path=$file" if defined($file);
	$feature .= "\n";
    }
    elsif ($section eq "source-word-deletion-feature") {
	my ($factor,$file) = split(/ /,$FEATURE{$section}[0]);
	$feature .= "SourceWordDeletionFeature name=swd factor=$factor";
 	$feature .= " path=$file" if defined($file);
	$feature .= "\n";
   }
    elsif ($section eq "word-translation-feature") {
	my ($factors,$simple,$dummy1,$dummy2,$dummy3,$dummy4,$file_f,$file_e) = split(/ /,$FEATURE{$section}[0]);
	my ($input_factor,$output_factor) = split(/\-/, $factors);
	$feature .= "WordTranslationFeature name=wt input-factor=$input_factor output-factor=$output_factor simple=$simple source-context=0 target-context=0";
	$feature .= " source-path=$file_f target-path=$file_e" if defined($file_f);
	$feature .= "\n";
    }
    elsif ($section eq "ttable-file") {
	my $i = 0;
	my @TTABLE_LIMIT = @{$FEATURE{"ttable-limit"}};
	my @W = @{$WEIGHT{"t"}};
	foreach my $line (@{$FEATURE{$section}}) {
	    my ($imp, $input_factor, $output_factor, $weight_count, $file) = split(/ /,$line);
	    my $implementation = $TTABLE_IMPLEMENTATION{$imp};
	    if (!defined($implementation)) {
		print STDERR "ERROR: Unknown translation table implementation: $implementation\n";
		$implementation = "UNKNOWN";
	    }
	    $feature .= "$implementation name=TranslationModel$i num-features=$weight_count path=$file input-factor=$input_factor output-factor=$output_factor";
	    $feature .= " table-limit=".$TTABLE_LIMIT[$i] if $#TTABLE_LIMIT >= $i;
	    $feature .= "\n";
	    $weight .= "TranslationModel$i=".&get_weights(\@W,$weight_count)."\n";
	    $i++;
	}
    }
    elsif ($section eq "generation-file") {
	my $i = 0;
	my @W = @{$WEIGHT{"generation"}};
	foreach my $line (@{$FEATURE{$section}}) {
	    my ($input_factor,$output_factor,$weight_count,$file) = split(/ /,$line);
	    $feature .= "Generation name=GenerationModel$i num-features=$weight_count path=$file input-factor=$input_factor output-factor=$output_factor\n";
	    $weight .= "GenerationModel$i=".&get_weights(\@W,$weight_count)."\n";
	    $i++;
	}
    }
    elsif ($section eq "distortion-file") {
	my $i = 0;
	my @W = @{$WEIGHT{"d"}};
	my $ignore = shift @W;
	foreach my $line (@{$FEATURE{$section}}) {
	    my ($factors,$type,$weight_count,$file) = split(/ /,$line);
	    my ($input_factor,$output_factor) = split(/\-/, $factors);
	    $feature .= "LexicalReordering name=LexicalReordering$i num-features=$weight_count type=$type input-factor=$input_factor output-factor=$output_factor path=$file\n";	    
	    $weight .= "LexicalReordering$i=".&get_weights(\@W,$weight_count)."\n";
	    $i++;
	}
    }
	
    elsif ($section eq "lmodel-file") {
	my $i = 0;
	my @W = @{$WEIGHT{"l"}};
	foreach my $line (@{$FEATURE{$section}}) {
	    my ($imp,$factor,$order,$file) = split(/ /,$line);
	    my $implementation = $LM_IMPLEMENTATION{$imp};
	    if (!defined($implementation)) {
		print STDERR "ERROR: Unknown language model implementation: $implementation\n";
		$implementation = "UNKNOWN";
	    }
	    $feature .= "$implementation name=LM$i factor=$factor path=$file order=$order\n";
	    $weight .= "LM$i=".&get_weights(\@W,1)."\n";
	    $i++;
	}
    }
}

print "\n[feature]\n$feature\n";
print "\n[weight]\n$weight\n";

sub get_data {
    my ($pattern) = @_;
    my @DATA;
    while (++$i < scalar(@INI) &&
	   $INI[$i] !~ /^\s*$/ &&
	   $INI[$i] !~ /^\[/ &&
	   $INI[$i] !~ /^\#/) {
	push @DATA,$INI[$i];
    }
    $i--;
    chop(@DATA);
    return \@DATA;
}

sub get_weights {
    my ($W,$count) = @_;
    my $list = "";
    for(my $w=0;$w<$count;$w++) {
	my $value = shift @{$W};
	$list .= " $value";
    }
    return $list;
}
