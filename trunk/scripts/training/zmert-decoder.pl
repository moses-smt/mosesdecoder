#!/usr/bin/perl -w

use strict;


if( !defined $ENV{"TMT_ROOT"}) {
  die "Cannot find TMT_ROOT. Is TectoMT really initialized?";
}
if( !defined $ENV{"DECODER_CFG_INTER"} || !defined $ENV{"DECODER_CMD"} ||
  !defined $ENV{"EXTRACT_SEMPOS"} || !defined $ENV{"NBEST_FILE"}) {
  print "You must set:
    \$DECODER_CFG_INTER - the intermediate decoder configuration file name
    \$DECODER_CMD - command to launch decoder after each ZMERT iteration
    \$EXTRACT_SEMPOS - way how to get SemPOS factors ('none' for no extraction)
    \$NBEST_FILE - file with nbest-lists produced by decoder\n";
}

# variables are initialized from zmert-moses.pl before the launch of zmert by
# substituting the placeholder values by the specific values
my $decoder_cfg_inter = $ENV{"DECODER_CFG_INTER"}; # file to extract updates lambdas from
my $decoder_cmd = $ENV{"DECODER_CMD"}; # command that launches decoder
my $extract_sempos = $ENV{"EXTRACT_SEMPOS"}; # how shall we get the SemPOS factor for nbest-list
      # options: 1) 'none' - moses generates sentences in required format (<word_form>|<SemPOS> for SemPOS metric)
      #          2) 'moses:<factor_index>' - extract SemPOS factor from the <factor_index> position
      #          (moses output is <word_form>|<factor_1>|...|<factor_n>)
      #          3) 'tmt' - moses outputs only <word_form> and we need to generate SemPOS with TectoMT
my $nbest_file = $ENV{"NBEST_FILE"}; # file with nbest-lists
my $TMT_ROOT = $ENV{"TMT_ROOT"};

my $srunblocks = "$TMT_ROOT/tools/srunblocks_streaming/srunblocks";
my $scenario_file = "scenario"; # scenario must be in the same directory
my $sentence_placeholder = "_SENTENCE_PLACEHOLDER_";

# extract feature weights from last zmert iteration (stored in $decoder_cfg_inter)
print "Updating decoder config file from file $decoder_cfg_inter";

open( IN, $decoder_cfg_inter) or die "Cannot open file $decoder_cfg_inter (reading updated lambdas)";
my %lambdas = ();
my $lastName = "";
while( my $line = <IN>) {
  chomp($line); 
  my ($name, $val) = split( /\s+/, $line);
  $name =~ s/_\d+$//;      # remove index of the lambda
  if( $name =~ /iteration/) {
    $iteration = $val;
  } else {
    push( $val, @{$lambdas{$name}});
  }
}
close(IN);

my $moses_ini_old = "$moses_ini.run$iteration";
safesystem("mv $moses_ini $moses_ini_old");
# update moses.ini
open( INI_OLD, $moses_ini_old) or die "Cannot open config file $moses_ini_old";
open( INI, $moses_ini) or die "Cannot open config file $moses_ini";
while( my $line = <INI_OLD>) {
  if( $line =~ m/^\[weight-(.+)\]$/) {
    my $name = $1;
    print INI "$line";
    foreach( @{$lambdas{$name}}) {
      print INI "$_\n";
      $line = <INI_OLD>;
    }
  } else {
    print INI $line;
  }
}
close(INI_OLD);
close(INI);

print "Executing: $decoder_cmd";
safesystem( $decoder_cmd);

# update iteration number in intermediate config file
++$iteration;
safesystem("sed -i 's/^iteration \d+/iteration $iteration/' $decoder_dfg_inter");

# modify the nbest-list to conform the zmert required format
# <i> ||| <candidate_translation> ||| featVal_1 featVal_2 ... featVal_m
my $nbest_file_orig = $nbest_file.".orig";
safesystem( "mv $nbest_file $nbest_file_orig");
open( NBEST_ORIG, "<$nbest_file_orig") or die "Cannot open original nbest-list $nbest_file_orig";
my $out = "";
my $line_num = 0;
if( $extract_sempos =~ /tmt/) {
  # run TectoMT to analyze sentences
  my $command = "|$srunblocks $scenario_file czech_source_sentence factored_output > $nbest_file.factored";
  open( TMT, "$command") or die "Cannot fork: $!";
}
foreach( my $line = <NBEST_ORIG>) {
  my @array = split( /|||/, $line);
  # remove feature names from the feature scores string
  $array[2] = s/\S*:\s//g;
  pop( @array); # remove sentence score
  if( $extract_sempos =~ /none/) {
    # do nothing
  } else if( $extract_sempos =~ /moses/) {
    # extract factor on position $factor_index
    my (undef, $factor_index) = split( /:/, $extract_sempos);
    my @tokens = split( /\s/, $array[1]); # split sentence into words
    foreach( my $token = @tokens) {
      my @factors = split( /|/, $token);
      $array[1] = join( "\s", $factors[0], $factors[$factor_index]);
    }
  } else if( $extract_sempos =~ /tmt/) {
    # analyze sentence via TectoMT using scenario in file $scerario_file
    print TMT "$array[1]\n";
    $array[1] = $sentence_placeholder;
    ++$line_num;
    # collect the results
  } else {
    die "Unknown type of factor extraction: $extract_sempos";
  }
  $out .= join( '|||', @array)."\n";
}
close( NBEST_ORIG);

if( $extract_sempos =~ /tmt/) {
  close( TMT); # we have already written all sentences
  my $cont = 1;
  my $len = 0;
  while( $cont && $cont < 100) {
    sleep(1);
    my $len_new = `wc -l < $nbest_file.factored`; # get the number of analyzed sentences
    if( $len_new == $line_num) { 
      $cont = 0; # we have all sentences analyzed -> stop
    } else if( $len_new > $len) {
      $cont = 1; # we analyzed another sentence -> reset the counter
      $len = $len_new;
    } else {
      ++$cont; # still analyzing the same sentence
    }
  }
  if( $cont == 100) { die "Waiting too long for TMT analysis. Check file $nbest_file.factored";
}
  # replace $sentence_placeholder with word_form and SemPOS factor
  
}


open( NBEST, ">$nbest_file") or die "Cannot open modified nbest-list $nbest_file";
print NBEST $out;
close( NBEST);

# END OF BODY

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
