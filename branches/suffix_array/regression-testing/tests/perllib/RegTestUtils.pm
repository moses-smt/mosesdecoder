#RegTestUtils.pm: for moses regression testing
#Evan Herbst, 8 / 11 / 06

use strict;

package RegTestUtils;
return 1;

###############################################################

#arguments: chomped line of output that gives the best hypo and various scores
#return: a string to be compared with the correct total hypothesis score;
# it's formatted as a double if no error, or "FORMAT ERROR" if there is one
sub readHypoScore
{
	my $line = shift;
	#the 0.12 is hardcoded in Hypothesis.cpp because some parsing scripts still
	#expect a comma-separated list of scores -- EVH
	if($line =~ /\[total=\s*(-?\d+\.\d+)\]/) {return $1;}
	return "FORMAT ERROR";
}

#arguments: chomped line of output that gives a time in seconds
#return: a string to be compared with the correct time;
# it's formatted as a double if no error, or "FORMAT ERROR" if there is one
sub readTime
{
	my $line = shift;
	if($line =~ /\[(\d+\.\d+)\]\s*seconds$/) {return $1;}
	return "FORMAT ERROR";
}
