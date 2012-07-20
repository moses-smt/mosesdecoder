#!/usr/bin/perl -w
use strict;

my $source_ext = shift;
my $target_ext = shift;
my $out = shift;
my $frStop = shift;

if (not defined($out)) {
    print STDERR "$0\n";
    print STDERR "select French and English phrases from phrase-table for PSD modeling\n";
    print STDERR "  - French phrases: all phrases from phrase-table, except those that start or end with numbers, punctuation or stopwords\n";
    print STDERR "  - English phrases: all translation candidates from the phrase-table for the selected French phrases\n\n";
    die "Usage: $0 source-ext target-ext output-name [French-stoplist]\n";
}

my %frStop = ();
if (defined $frStop) {
  open(FRS,$frStop) || die "Can't open $frStop:$!\n";
  while(<FRS>){
    chomp;
    $frStop{$_}++;
  }
  close(FRS);
}

my %frPhrases = (); my %enPhrases = ();
my @e;
my @f;
my @a;

while(<STDIN>){	

    	my $sourceTermCounter = 0;	
	@e = ();
	@a = ();
	@f = ();
	my $stage = 0;
    #print "New line : $_ \n";
    chomp;
    # This does not work for hiero because we can have empty alignments	 
    # my ($f,$e,$s,$a) = split(/ \|\|\| /);
    my @token = split('\s', $_);

	foreach(@token)
	{
		#print "Obtained Token : $_ \n";

		if ($_ eq "|||")
		{
			++$stage;
		}
		if($stage == 0)
		{
		   if($_ ne "|||")
		   {	
			#print "Pushed : $_ \n";
			push(@f,$_);
		   }
		}
		if($stage == 1)
		{
		   if($_ ne "|||")
		   {	
			push(@e,$_);
		   }
		}
		if($stage == 3)
		{
		   if($_ ne "|||")
		   {	
			push(@a,$_);
		   }
		}
	}

    my @newEn;
    @newEn = &CreateAlignedTarget(\@e,\@a);
    my @newFr;
    @newFr = &ReplaceTermsInSource(\@f); 		

    my $f = join(" ",@newFr);	
    my $e = join(" ",@newEn);

    #For hiero : add alignments to target phrases
    $frPhrases{$f}++;
    #print "Align augmented line \n"; 
    #print $e;
    #print "\n";   
   
    $enPhrases{$e}++;
}

sub CreateAlignedTarget
{
	my @e = @{ $_[0] };
	my @a = @{ $_[1] }; 

	my @newEn;	

	#make non-term index map
	my @nonTermMap = (1..scalar(@e));
	my $sourcePos = 0;
	foreach(@a)
	{
		my @align = split("-",$_);
		splice(@nonTermMap,pop(@align),0,$sourcePos++);	
	}	

	my $loopCounter = 0;
	foreach(@e)
	{
		if($_ eq "[X][X]")
		{
			my $NonTermAlign = "X".$nonTermMap[$loopCounter];
			#print "Non term align : $NonTermAlign \n";
			push(@newEn,$NonTermAlign);
		}
		else
		{push(@newEn,$_)}
		$loopCounter++;
		#if($loopCounter < scalar(@targetToken))
		#{print " ";}
	}
	return @newEn;
}

sub ReplaceTermsInSource
{
	my @f = @{ $_[0] };
	my @newFr;

	foreach(@f)
	{
		if($_ eq "[X][X]")
		{
			my $NonTerm = "X";
			#print "Non term align : $NonTermAlign \n";
			push(@newFr,$NonTerm);
		}
		else
		{push(@newFr,$_)}
	}
	return @newFr;
}	

open(EN,">$out.$target_ext") || die "Can't write to $out.$target_ext:$!\n";
foreach my $e (sort keys %enPhrases){
    print EN "$e\n";
}
close(EN);

open(FR,">$out.$source_ext") || die "Can't write to $out.$source_ext:$!\n";
foreach my $f (sort keys %frPhrases){
    print FR "$f\n";
}
close(FR);
