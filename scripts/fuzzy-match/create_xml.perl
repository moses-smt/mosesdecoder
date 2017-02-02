#!/usr/bin/env perl
#
# This file is part of moses.  Its use is licensed under the GNU Lesser General
# Public License version 2.1 or, at your option, any later version.

binmode( STDIN,  ":utf8" );
binmode( STDOUT, ":utf8" );

use warnings;
use strict;
use FindBin qw($RealBin);
use File::Basename;

sub trim($);

print STDERR "HELLO ";
for ( my $i = 0 ; $i < scalar @ARGV ; ++$i ) {
  print STDERR $ARGV[$i] . " ";
}
print STDERR "GOODBYE \n";

############################################
# START

my $inPath = $ARGV[0];
open( IN, "<" . $inPath );

open( RULE,     ">$inPath.extract" );
open( RULE_INV, ">$inPath.extract.inv" );

my ( $sentenceInd, $score, $source, $input, $target, $align, $path, $count );

# MAIN LOOP
while ( $sentenceInd = <IN> ) {
  $score  = <IN>;
  $source = <IN>;
  $input  = <IN>;
  $target = <IN>;
  $align  = <IN>;
  $path   = <IN>;
  $count  = <IN>;
  chomp($sentenceInd);
  chomp($score);
  chomp($source);
  chomp($input);
  chomp($target);
  chomp($align);
  chomp($path);
  chomp($count);
  $source = trim($sentenceInd);
  $source = trim($score);
  $source = trim($source);
  $input  = trim($input);
  $target = trim($target);
  $align  = trim($align);
  $path   = trim($path);
  $count  = trim($count);

  my ( $frame, $rule_s, $rule_t, $rule_alignment, $rule_alignment_inv ) =
    &create_xml( $source, $input, $target, $align, $path );

  #print STDOUT $frame."\n";
  print RULE "$rule_s [X] ||| $rule_t [X] ||| $rule_alignment ||| $count\n";
  print RULE_INV
    "$rule_t [X] ||| $rule_s [X] ||| $rule_alignment_inv ||| $count\n";

  #print STDOUT "$sentenceInd ||| $score ||| $count\n";

}

close(IN);
close(RULE);
close(RULE_INV);

`LC_ALL=C sort $inPath.extract | gzip -c > $inPath.extract.sorted.gz`;
`LC_ALL=C sort $inPath.extract.inv | gzip -c > $inPath.extract.inv.sorted.gz`;

my $lex_file = "-";

my $cmd;
$cmd =
"$RealBin/../../scripts/training/train-model.perl -dont-zip -first-step 6 -last-step 6 -f en -e fr -hierarchical -extract-file $inPath.extract -lexical-file $lex_file -score-options \"--NoLex\" -phrase-translation-table $inPath.pt";
print STDERR "Executing: $cmd \n";
`$cmd`;

#######################################################
sub create_xml {
  my ( $source, $input, $target, $alignment, $path ) = @_;

  my @INPUT  = split( / /, $input );
  my @SOURCE = split( / /, $source );
  my @TARGET = split( / /, $target );
  my %ALIGN  = &create_alignment($alignment);

  my %FRAME_INPUT;
  my ( @NT, @INPUT_BITMAP, @TARGET_BITMAP, %ALIGNMENT_I_TO_S );
  foreach (@TARGET) { push @TARGET_BITMAP, 1 }

  ### STEP 1: FIND MISMATCHES

  my ( $s, $i ) = ( 0, 0 );
  my $currently_matching = 0;
  my ( $start_s, $start_i ) = ( 0, 0 );

  $path .= "X";    # indicate end
  print STDERR "$input\n$source\n$target\n$path\n";
  for ( my $p = 0 ; $p < length($path) ; $p++ ) {
    my $action = substr( $path, $p, 1 );

    # beginning of a mismatch
    if ( $currently_matching && $action ne "M" && $action ne "X" ) {
      $start_i            = $i;
      $start_s            = $s;
      $currently_matching = 0;
    }

    # end of a mismatch
    elsif ( !$currently_matching
      && ( $action eq "M" || $action eq "X" ) )
    {

      # remove use of affected target words
      for ( my $ss = $start_s ; $ss < $s ; $ss++ ) {
        foreach my $tt ( keys %{ ${ $ALIGN{'s'} }[$ss] } ) {
          $TARGET_BITMAP[$tt] = 0;
        }

        # also remove enclosed unaligned words?
      }

      # are there input words that need to be inserted ?
      print STDERR "($start_i<$i)?\n";
      if ( $start_i < $i ) {

        # take note of input words to be inserted
        my $insertion = "";
        for ( my $ii = $start_i ; $ii < $i ; $ii++ ) {
          $insertion .= $INPUT[$ii] . " ";
        }

        # find position for inserted input words

        # find first removed target word
        my $start_t = 1000;
        for ( my $ss = $start_s ; $ss < $s ; $ss++ ) {
          foreach my $tt ( keys %{ ${ $ALIGN{'s'} }[$ss] } ) {
            $start_t = $tt if $tt < $start_t;
          }
        }

        # end of sentence? add to end
        if ( $start_t == 1000 && $i > $#INPUT ) {
          $start_t = $#TARGET;
        }

        # backtrack to previous words if unaligned
        if ( $start_t == 1000 ) {
          $start_t = -1;
          for ( my $ss = $s - 1 ; $start_t == -1 && $ss >= 0 ; $ss-- ) {
            foreach my $tt ( keys %{ ${ $ALIGN{'s'} }[$ss] } ) {
              $start_t = $tt if $tt > $start_t;
            }
          }
        }
        $FRAME_INPUT{$start_t} .= $insertion;
        my %NT = (
          "start_t" => $start_t,
          "start_i" => $start_i
        );
        push @NT, \%NT;
      }
      $currently_matching = 1;
    }

    print STDERR "$action $s $i ($start_s $start_i) $currently_matching";
    if ( $action ne "I" ) {
      print STDERR " ->";
      foreach my $tt ( keys %{ ${ $ALIGN{'s'} }[$s] } ) {
        print STDERR " " . $tt;
      }
    }
    print STDERR "\n";
    $s++ unless $action eq "I";
    $i++ unless $action eq "D";
    $ALIGNMENT_I_TO_S{$i} = $s unless $action eq "D";
    push @INPUT_BITMAP, 1 if $action eq "M";
    push @INPUT_BITMAP, 0 if $action eq "I" || $action eq "S";
  }

  print STDERR $target . "\n";
  foreach (@TARGET_BITMAP) { print STDERR $_; }
  print STDERR "\n";
  foreach ( sort keys %FRAME_INPUT ) {
    print STDERR "$_: $FRAME_INPUT{$_}\n";
  }

  ### STEP 2: BUILD RULE AND FRAME

  # hierarchical rule
  my $rule_s     = "";
  my $rule_pos_s = 0;
  my %RULE_ALIGNMENT_S;
  for ( my $i = 0 ; $i < scalar(@INPUT_BITMAP) ; $i++ ) {
    if ( $INPUT_BITMAP[$i] ) {
      $rule_s .= $INPUT[$i] . " ";
      $RULE_ALIGNMENT_S{ $ALIGNMENT_I_TO_S{$i} } = $rule_pos_s++;
    }
    foreach my $NT (@NT) {
      if ( $i == $$NT{"start_i"} ) {
        $rule_s .= "[X][X] ";
        $$NT{"rule_pos_s"} = $rule_pos_s++;
      }
    }
  }

  my $rule_t     = "";
  my $rule_pos_t = 0;
  my %RULE_ALIGNMENT_T;
  for ( my $t = -1 ; $t < scalar(@TARGET_BITMAP) ; $t++ ) {
    if ( $t >= 0 && $TARGET_BITMAP[$t] ) {
      $rule_t .= $TARGET[$t] . " ";
      $RULE_ALIGNMENT_T{$t} = $rule_pos_t++;
    }
    foreach my $NT (@NT) {
      if ( $t == $$NT{"start_t"} ) {
        $rule_t .= "[X][X] ";
        $$NT{"rule_pos_t"} = $rule_pos_t++;
      }
    }
  }

  my $rule_alignment = "";
  foreach my $s ( sort { $a <=> $b } keys %RULE_ALIGNMENT_S ) {
    foreach my $t ( keys %{ $ALIGN{"s"}[$s] } ) {
      next unless defined( $RULE_ALIGNMENT_T{$t} );
      $rule_alignment .=
        $RULE_ALIGNMENT_S{$s} . "-" . $RULE_ALIGNMENT_T{$t} . " ";
    }
  }
  foreach my $NT (@NT) {
    $rule_alignment .= $$NT{"rule_pos_s"} . "-" . $$NT{"rule_pos_t"} . " ";
  }

  chop($rule_s);
  chop($rule_t);
  chop($rule_alignment);

  my $rule_alignment_inv = "";
  foreach ( split( / /, $rule_alignment ) ) {
    /^(\d+)\-(\d+)$/;
    $rule_alignment_inv .= "$2-$1 ";
  }
  chop($rule_alignment_inv);

  # frame
  my $frame = "";
  $frame = $FRAME_INPUT{-1} if defined $FRAME_INPUT{-1};

  my $currently_included = 0;
  my $start_t            = -1;
  push @TARGET_BITMAP, 0;    # indicate end

  for ( my $t = 0 ; $t <= scalar(@TARGET) ; $t++ ) {

    # beginning of tm target inclusion
    if ( !$currently_included && $TARGET_BITMAP[$t] ) {
      $start_t            = $t;
      $currently_included = 1;
    }

    # end of tm target inclusion (not included word or inserted input)
    elsif ( $currently_included
      && ( !$TARGET_BITMAP[$t] || defined( $FRAME_INPUT{$t} ) ) )
    {
      # add xml (unless change is at the beginning of the sentence
      if ( $start_t >= 0 ) {
        my $target = "";
        print STDERR "for(tt=$start_t;tt<$t+$TARGET_BITMAP[$t]);\n";
        for ( my $tt = $start_t ; $tt < $t + $TARGET_BITMAP[$t] ; $tt++ ) {
          $target .= $TARGET[$tt] . " ";
        }
        chop($target);
        $frame .= "<xml translation=\"$target\"> x </xml> ";
      }
      $currently_included = 0;
    }

    $frame .= $FRAME_INPUT{$t} if defined $FRAME_INPUT{$t};
    print STDERR "$TARGET_BITMAP[$t] $t ($start_t) $currently_included\n";
  }

  print STDERR $frame . "\n-------------------------------------\n";
  return ( $frame, $rule_s, $rule_t, $rule_alignment, $rule_alignment_inv );
}

sub create_alignment {
  my ($line) = @_;
  my ( @ALIGNED_TO_S, @ALIGNED_TO_T );
  foreach my $point ( split( / /, $line ) ) {
    my ( $s, $t ) = split( /\-/, $point );
    $ALIGNED_TO_S[$s]{$t}++;
    $ALIGNED_TO_T[$t]{$s}++;
  }
  my %ALIGNMENT = ( 's' => \@ALIGNED_TO_S, 't' => \@ALIGNED_TO_T );
  return %ALIGNMENT;
}

# Perl trim function to remove whitespace from the start and end of the string
sub trim($) {
  my $string = shift;
  $string =~ s/^\s+//;
  $string =~ s/\s+$//;
  return $string;
}

# Left trim function to remove leading whitespace
sub ltrim($) {
  my $string = shift;
  $string =~ s/^\s+//;
  return $string;
}

# Right trim function to remove trailing whitespace
sub rtrim($) {
  my $string = shift;
  $string =~ s/\s+$//;
  return $string;
}
