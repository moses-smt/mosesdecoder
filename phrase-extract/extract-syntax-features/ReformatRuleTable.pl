#!/usr/bin/perl -w
use strict;

my @e;
my @f;
my @a;
my @s;
my @r;

while(<STDIN>){	

    	my $sourceTermCounter = 0;	
	@e = ();
	@a = ();
	@f = ();
	@s = ();
	@r = ();
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
		if($stage == 2)
		{
		   if($_ ne "|||")
		   {	
			push(@s,$_);
		   }
		}
		if($stage == 4)
		{
		   if($_ ne "|||")
		   {	
			push(@r,$_);
		   }
		}
	}

    my @newEn;
    @newEn = &CreateAlignedTarget(\@e,\@a);
    my @newFr;
    @newFr = &ReplaceTermsInSource(\@f); 		

    my $f = join(" ",@newFr);	
    my $e = join(" ",@newEn);
    my $a = join(" ",@a);
    my $s = join(" ",@s);
    my $r = join(" ",@r);

    print "$f ||| $e ||| $s ||| $a ||| $r \n"
    #For hiero : add alignments to target phrases
    #$frPhrases{$f}++;
    #print "Align augmented line \n"; 
    #print $e;
    #print "\n";   
   
    #$enPhrases{$e}++;
}

sub CountNonTerminals
{
    my @e = @{ $_[0] };
    my $countNonTerm = 0;

 	foreach(@e)
	{
		if($_ eq "[X][X]")
		{
		    $countNonTerm++;
		}
	}
	return $countNonTerm;  
}

sub CreateAlignedTarget
{
	my @e = @{ $_[0] };
	my @a = @{ $_[1] }; 

	my $countNonTerms = &CountNonTerminals(\@e);
	#print "COUNT NON TERMS : $countNonTerms \n";

	my @nonTermAlign;

	my @newEn;	

	#make non-term index map
	my @nonTermMap = (1..scalar(@e));
	my $sourcePos = 0;
	foreach(@a)
	{
	        last if ($sourcePos == $countNonTerms); 
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
