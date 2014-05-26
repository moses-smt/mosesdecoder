#!/usr/bin/perl -w

use strict;

use utf8;
use Getopt::Std;
use IO::Handle;
binmode(STDIN,  ':utf8');
binmode(STDOUT, ':utf8');
binmode(STDERR, ':utf8');

my @source;
my @target;
my @words;
my $tPath = $ARGV[0];
my $tFile = $ARGV[1];
my $inp_ext = $ARGV[2];
my $op_ext = $ARGV[3];
my $src;
my $tgt;
my $t;
my $s;

`mkdir $tPath/training`;
`mkdir $tPath/tuning`;

open FH,  "<:encoding(UTF-8)", "$tPath/$tFile" or die "Can't open $tPath/$tFile: $!\n";
open MYSFILE,  ">:encoding(UTF-8)", "$tPath/training/corpus.$inp_ext" or die "Can't open $tPath/training/corpus.$inp_ext: $!\n";
open MYTFILE,  ">:encoding(UTF-8)", "$tPath/training/corpus.$op_ext" or die "Can't open $tPath/training/corpus.$op_ext: $!\n";

while (<FH>) 
{
    chomp;    
    my ($src,$tgt) = split(/\t/);
    
    $s = join(' ', split('',$src)); 
    $t = join(' ', split('',$tgt)); 
    print MYSFILE "$s\n";
    print MYTFILE "$t\n";	  
    push(@source, $s);
    push(@target, $t);
}

close (FH);
close (MYSFILE);
close (MYTFILE);

open MYSFILE,  ">:encoding(UTF-8)", "$tPath/training/corpusA.$inp_ext" or die "Can't open $tPath/training/corpusA.$inp_ext: $!\n";
open MYTFILE,  ">:encoding(UTF-8)", "$tPath/training/corpusA.$op_ext" or die "Can't open $tPath/training/corpusA.$op_ext: $!\n";

open MYSDEVFILE,  ">:encoding(UTF-8)", "$tPath/tuning/input" or die "Can't open $tPath/tuning/input: $!\n";
open MYTDEVFILE,  ">:encoding(UTF-8)", "$tPath/tuning/reference" or die "Can't open $tPath/tuning/reference: $!\n";

my $corpus_size = @source;
my $count = 11;
my $dev_size = 0;


   foreach (@source)
   {
         if ($count % 5 == 0 && $dev_size < 1000)
	  {
		print MYSDEVFILE "$source[$count-11]\n";
		print MYTDEVFILE "$target[$count-11]\n";
		$dev_size++;
	  }
	  else
	  {
		print MYSFILE "$source[$count-11]\n";
		print MYTFILE "$target[$count-11]\n";
	  }
	$count++;
   }

close (MYSFILE);
close (MYTFILE);
close (MYSDEVFILE);
close (MYTDEVFILE);

if ($corpus_size < 6000)
{
	`rm $tPath/training/corpusA.$inp_ext`;
	`rm $tPath/training/corpusA.$op_ext`;
}


