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

sub CountNonTerminals
{
    my @e = @{ $_[0] };
    my $countNonTerm = 0;

 	foreach(@e)
	{
		#print "LOOKING AT ENGLISH TOKEN : ".$_."\n";
		if($_ eq "[X][X]")
		{
		    $countNonTerm++;
		}
	}
	return $countNonTerm;  
}

sub ReplaceValueWithIndex()
{	
	# when creating these vectors, we can already cut at the number of non-terminals
	my @sourceNonTermAlignment  = @{ $_[0] };
	my @targetNonTermAlignment  = @{ $_[1] };

	#FB : debugging
	#foreach(@sourceNonTermAlignment)
	#{
	#			print "SOURCE NON TERM ALIGN BEFORE PROCESSING : ".$_."\n",
	#}

	my $sourceIndex = 0;

	my $minIndexPos = -1;
	# FB : do the same for source and then compare both
	# my $minIndexPos = -1;
	for(my $i = 0; $i < scalar(@sourceNonTermAlignment); $i++)
	{
		
		my $min = 1000;
		my $indexPos = 0;
		foreach(@sourceNonTermAlignment)
		{
			#print "SOURCE : LOOKING AT ALIGNMENT : ".$_."\n";
			#print "WITH SOURCE INDEX : ".$sourceIndex."\n";
			if( ($_ <  $min) && ($_ > ($sourceIndex-1) ) )
			{	
				$min =  $_;
				$minIndexPos = $indexPos;
			#	print "IS MINIMUM ".$_." : INDEX POS : ".$minIndexPos."\n";
			}
			$indexPos++;
		}
		#print "SOURCE : SPLICING AT : ".$minIndexPos."\t".$sourceIndex."\n";
		#print "SOURCE : SIZE OF ALIGNMENT ARRAY BEFORE SPLICE: ".scalar(@sourceNonTermAlignment)."\n";
		splice(@sourceNonTermAlignment,$minIndexPos,1,$sourceIndex);
		$sourceIndex++;
		$indexPos++;	
		#print "SOURCE : SIZE OF ALIGNMENT ARRAY AFTER SPLICE: ".scalar(@sourceNonTermAlignment)."\n";	
	}

	#FB : debugging
	#foreach(@sourceNonTermAlignment)
	#{
	#		print "SOURCE NON TERM ALIGN AFTER PROCESSING : ".$_."\n";
	#}

	#FB : debugging
	#foreach(@targetNonTermAlignment)
	#{
	#	print "TARGET NON TERM ALIGN BEFORE PROCESSING: ".$_."\n";
	#}

	my $targetIndex = 0;

	# FB : inefficient O(n^2) can be done in n(log(n)) but alignment vector is usually small
	for(my $j = 0; $j < scalar(@targetNonTermAlignment); $j++)
	{
		my $min = 1000;
		my $indexPos = 0;
		foreach(@targetNonTermAlignment)
		{
			#print "LOOKING AT ALIGNMENT : ".$_."\n";
			#print "WITH TARGET INDEX : ".$targetIndex."\n";
			if( ($_ <  $min) && ($_ > ($targetIndex-1) ) )
			{	
				$min =  $_;
				$minIndexPos = $indexPos;
				#print "IS MINIMUM ".$_." : INDEX POS : ".$minIndexPos."\n";
			}
			$indexPos++;
		}
		#print "SPLICING AT : ".$minIndexPos."\t".$targetIndex."\n";
		#print "SIZE OF ALIGNMENT ARRAY BEFORE SPLICE: ".scalar(@targetNonTermAlignment)."\n";
		splice(@targetNonTermAlignment,$minIndexPos,1,$targetIndex);
		$targetIndex++;	
		#print "SIZE OF ALIGNMENT ARRAY AFTER SPLICE: ".scalar(@targetNonTermAlignment)."\n";	
	}
	
	return (\@sourceNonTermAlignment,\@targetNonTermAlignment);

}	

# Corresponding non-terminal indexes can be attached directly to non-terminals
sub CreateAlignedTarget()
{
	# Get references into arrays
	my @e = @{ $_[0] };
	my @sourceNonTermAlignment = @{ $_[1] }; 
	my @targetNonTermAlignment = @{ $_[2] }; 

	my @newEn;
	my $sourceNonTermCounter = shift(@sourceNonTermAlignment);

	#check source counter here
	#get the associated source and attach target non-terminal	
	foreach(@e)
	{
		if($_ eq "[X][X]")
		{
			#print "GETTING AT COUNTER : ".$sourceNonTermCounter."\n";
			my $NonTermAlign = "[X][X]".$sourceNonTermCounter;
			#print "Non term align : $NonTermAlign \n";
			push(@newEn,$NonTermAlign);
			$sourceNonTermCounter = shift(@sourceNonTermAlignment);
		}
		else
		{push(@newEn,$_)}
	}
	return @newEn;
}

while(<STDIN>){	

    	my $sourceTermCounter = 0;	
	@e = ();
	@a = ();
	@f = ();
	@s = ();
	@r = ();
	# all target sides of alignments
	@sourceNonTermAlignAll = ();
	@targetNonTermAlignAll = ();
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
			# split alignment and put into separate arrays
			my @align = split("-",$_);
			#print "PUSH ALIGNMENT : ".pop(@align)."\n";
			push(@sourceNonTermAlignAll,shift(@align));
			push(@targetNonTermAlignAll,pop(@align));	
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

    # count non-terminals in english (target) side of rule
    my $countNonTerms = &CountNonTerminals(\@e);
    #print "COUNT NON TERMS : $countNonTerms \n";

    #FB : debugging
    #foreach(@targetNonTermAlignAll)
    #{
	# print "TARGET NON TERM ALIGN ALL: ".$_."\n",
    #}
	
   # if there are no non-terminals then don't do anything
	
   my @newEn = ();
	
   if($countNonTerms == 0)
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
	 # only get alignments between non-terminals
    	@sourceNonTermAlign = splice(@sourceNonTermAlignAll, 0, $countNonTerms);
    	@targetNonTermAlign = splice(@targetNonTermAlignAll, 0, $countNonTerms);	
	
   	my ($sourceNonTermIndexRef,$targetNonTermIndexRef) = &ReplaceValueWithIndex(\@sourceNonTermAlign,\@targetNonTermAlign);

   	# Dereference returned references

   	my @sourceNonTermIndex = @$sourceNonTermIndexRef;
   	my @targetNonTermIndex = @$targetNonTermIndexRef;
   	
	@newEn = &CreateAlignedTarget(\@e,\@sourceNonTermIndex,\@targetNonTermIndex);

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
