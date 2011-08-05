#!/usr/bin/perl -w

# $Id$

use strict;

my @LINE = <STDIN>;

print "# Moses configuration file\n";
print "# automatic exodus from pharaoh.ini ".`date`;
print "\n";

# replicate the old header
my $header = 0;
while($LINE[$header] =~ /^$/ || $LINE[$header] =~ /^\#/) {
    $header++;
}
for(my $i=0;$i<$header;$i++) { print $LINE[$i]; }

# read Pharaoh parameters that will be absorbed
my(@LMODEL_TYPE, @DISTORTION_TYPE);
for(my $i=$header;$i<=$#LINE;$i++) {
    # language model specification
    if ($LINE[$i] =~ /^\[lmodel-type\]/) {
	$i = &read(\@LMODEL_TYPE,$i);
	foreach (@LMODEL_TYPE) {
	    $_ = "3gram" if $_ eq "normal";
	    $_ = "3gram-".$_ unless /\d/;
            $_ =~ s/gram//;
	}
    }
    # distortion model specification
    elsif ($LINE[$i] =~ /^\[distortion-type\]/) {
        my @DT;
	$i = &read(\@DT,$i);
        foreach (@DT) {
	    next if /distance/;
	    s/orientation/msd/;
            s/monotonicity/monotone/;
            push @DISTORTION_TYPE,$_;
        }
    }
}
# adapt/replicate Pharaoh parameters
for(my $i=$header;$i<=$#LINE;$i++) {
    # parameters to be dropped
    if ($LINE[$i] =~ /^\[lmodel-type\]/) {
	my @DUMMY;
	$i = &read(\@DUMMY,$i);
    }
    # 
		elsif ($LINE[$i] =~ /^\[distortion-type\]/) {
				my @DISTORTION_TYPE;
				$i = &read(\@DISTORTION_TYPE,$i);
        foreach (@DISTORTION_TYPE) {
						next if /distance/;
						s/orientation/msd/;
                                                s/monotonicity/monotone/;
                                                s/unidirectional/backward/;
				} 
    }
    # parameters to be changed
    elsif ($LINE[$i] =~ /^\[lmodel-file\]/) {
	print $LINE[$i];	
	# add language model type, factors
	my @LMODEL_FILE;
	$i = &read(\@LMODEL_FILE,$i);
	for(my $j=0;$j<=$#LMODEL_FILE;$j++) {
	    print "0 0 ";
	    if (defined($LMODEL_TYPE[$j])) {
		print $LMODEL_TYPE[$j];
	    }
	    else {
		print "3";
	    }
	    print " $LMODEL_FILE[$j]\n";
	}
        print "\n";
    }
    elsif ($LINE[$i] =~ /^\[ttable-file\]/) {
	print $LINE[$i];
	# add factors
	my @TTABLE_FILE;
	$i = &read(\@TTABLE_FILE,$i);
	my $first_line;
	if (-e $TTABLE_FILE[0]) {
	    if ($TTABLE_FILE[0] =~ /\.gz$/) { 
		$first_line = `zcat $TTABLE_FILE[0] | head -1`;
	    }
	    else {
		$first_line = `head -1 $TTABLE_FILE[0]`;
	    }
	}
	elsif (-e $TTABLE_FILE[0].".gz") {
	    $first_line = `zcat $TTABLE_FILE[0] | head -1`;
	}
	else {
	    print STDERR "ERROR: Thou shalt have a translation table in '$TTABLE_FILE[0]'\n";
	    exit;
	}
	chop($first_line);
	my ($f,$e,$p) = split(/ \|\|\| /,$first_line);
	$p =~ s/ +/ /g; $p =~ s/^ //; $p =~ s/ $//;
	my @P = split(/ /,$p);
	my $p_count = scalar @P;
	print "0 0 $p_count $TTABLE_FILE[0]\n\n";
    }
    elsif ($LINE[$i] =~ /^\[distortion-file\]/) {
	print $LINE[$i];
	my @DISTORTION_FILE;
	$i = &read(\@DISTORTION_FILE,$i);
	for(my $j=0;$j<=$#DISTORTION_FILE;$j++) {
	    if (!defined($DISTORTION_TYPE[$j])) {
                die("ERROR: no distortion type specified for distortion file $DISTORTION_FILE[$j]\n");
            }
            my $weight_count = 2;
	    $weight_count++  if $DISTORTION_TYPE[$j] =~ /msd/;
	    $weight_count*=2 if $DISTORTION_TYPE[$j] =~ /fe/;
	    print "0-0 $DISTORTION_TYPE[$j] $weight_count $DISTORTION_FILE[$j]\n";
	}
        print "\n";
    }
    elsif ($LINE[$i] =~ /\# distortion \(reordering\) type/) {}
    else {
	# keep unchanged
	print $LINE[$i];
    }
}

# add Moses-specific configuration

print "\n[input-factors]\n0\n\n";
print "[mapping]\nT 0\n\n";

# sub: read values for one parameter
sub read {
    my ($VALUE,$i) = @_;
    $i++;
    while($i<=$#LINE && $LINE[$i] !~ /^\[/) {
	if ($LINE[$i] !~ /^\s*$/ && # ignore comments and empty lines
	    $LINE[$i] !~ /^\#/) { 
	    # store value
	    my $line = $LINE[$i];
	    chop($line);
	    push @{$VALUE},$line;
	}
	$i++;
    }
    $i--;

    # leave comments above next parameter
    while($LINE[$i] =~ /^\#/) { $i--; }

    return $i;
}
