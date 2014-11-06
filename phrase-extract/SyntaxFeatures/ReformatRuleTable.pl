#!/usr/bin/perl -w
use strict;

#TODO : rewrite so that it does not rely on alignments...
# Remember position of source non-terminal and get right alignment

my @e;
my @f;
my @a;
my @s;
my @r;

my @targetNonTermAlignAll;
my @targetNonTermAlign;

my @sourceNonTermAlignAll;
my @sourceNonTermAlign;

sub FindSourceNonTerminals
{
    my @f = @{ $_[0] };
    my $index = 0;
    my @sourceNonTermPos = ();

 	foreach(@f)
	{
		if($_ eq "[X][X]")
		{
		    push(@sourceNonTermPos,$index);
		}
		$index++;
	}
	return @sourceNonTermPos;  
}

sub ReplaceValueWithIndex()
{	
	#take the minimal value of the alignment between target non-terminals and replace by 0, the second one by 1 etc. 
	my @targetNonTermAlignment  = @{ $_[0] };

	my $targetIndex = 0;
	my $minIndexPos = 0;
	# FB : inefficient O(n^2) can be done in n(log(n)) but alignment vector is usually small
	for(my $j = 0; $j < scalar(@targetNonTermAlignment); $j++)
	{
		my $min = 1000;
		my $indexPos = 0;
		foreach(@targetNonTermAlignment)
		{
			if( ($_ <  $min) && ($_ > ($targetIndex-1) ) )
			{	
				$min =  $_;
				$minIndexPos = $indexPos;
			}
			$indexPos++;
		}
		splice(@targetNonTermAlignment,$minIndexPos,1,$targetIndex);
		$targetIndex++;	
	}
	
	return (\@targetNonTermAlignment);

}	

# Corresponding non-terminal indexes can be attached directly to non-terminals
sub CreateAlignedTarget()
{
	my @e = @{ $_[0] };
	my @targetIndexes = @{ $_[1] }; 

	my @newEn;

	foreach(@e)
	{
		if($_ eq "[X][X]")
		{
			#print "GETTING AT COUNTER : ".$sourceNonTermCounter."\n";
			my $targetIndex = shift(@targetIndexes);
			my $NonTermAlign = "[X][X]".$targetIndex;
			push(@newEn,$NonTermAlign);
		}
		else
		{push(@newEn,$_)}
	}
	return @newEn;
}

while(<STDIN>){	

	@e = ();
	@a = ();
	@f = ();
	@s = ();
	@r = ();
	# all target sides of alignments
	my @sourceAlignAll = ();
	my @targetAlignAll = ();
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
			#print "FRENCH PART : $_ \n";
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
			# split alignment and put into separate arrays
			my @align = split("-",$_);
			push(@sourceAlignAll,shift(@align));
			push(@targetAlignAll,pop(@align));	
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

    # count non-terminals in frencg (source) side of rule
    my @sourceNonTerms = &FindSourceNonTerminals(\@f);
	
   my @newEn = ();
	
   #if there are no nonterminals just take as is
   if(scalar(@sourceNonTerms) == 0)
   {
	my $f = join(" ",@f);	
   	my $e = join(" ",@e);
    	my $a = join(" ",@a);
   	my $s = join(" ",@s);
    	my $r = join(" ",@r);

    	print "$f ||| $e ||| $s ||| $a ||| $r \n"

   }
   else		
   {	
	my @targetNonTerms = ();
		
	#TODO : this should be implemented in a more efficient way
	#Find the index of source non-terminals to find target non-terminals
	foreach(@sourceNonTerms)
	{
		my $nonTermToFind = $_;
		my $index = 0;
		foreach(@sourceAlignAll)
		{
			#find index of source non-terminal
			#get target at this index
			if($_ eq $nonTermToFind)
			{
				my $targetToInsert = $targetAlignAll[$index];
				push(@targetNonTerms,$targetToInsert);	
			}
			$index++;
		}
		
	}	
	
   	my ($targetNonTermIndexRef) = &ReplaceValueWithIndex(\@targetNonTerms);
   	my @targetNonTermIndex = @$targetNonTermIndexRef;
   	
	@newEn = &CreateAlignedTarget(\@e,\@targetNonTermIndex);

    my $f = join(" ",@f);	
    my $e = join(" ",@newEn);
    my $a = join(" ",@a);
    my $s = join(" ",@s);
    my $r = join(" ",@r);

    print "$f ||| $e ||| $s ||| $a ||| $r \n"
}
    #For hiero : add alignments to target phrases
    #$frPhrases{$f}++;
    #print "Align augmented line \n"; 
    #print $e;
    #print "\n";   
   
    #$enPhrases{$e}++;
}	
