#! /usr/bin/perl

sub PrintArgsAndDie () {
    print stderr "USAGE: reduce-field.pl [-h] \n";
    print stderr "This scripts reduce the number of active fields for the mert procedure.\n";
    exit(1);
}

my $weightfile="";
my $size=-1;
my $activefields="";
my $debug=0;

while (@ARGV){
    if ($ARGV[0] eq "-h"){
        &PrintArgsAndDie();
    }
    if ($ARGV[0] eq "-debug"){
        $debug=1; 
	shift(@ARGV);
    }
    if ($ARGV[0] eq "-weight"){
        $weightfile=$ARGV[1];
        shift(@ARGV); shift(@ARGV);
    }
    if ($ARGV[0] eq "-d"){
        $size=$ARGV[1];
        shift(@ARGV); shift(@ARGV);
    }
    if ($ARGV[0] eq "-activate"){
        $activefields=$ARGV[1];
        shift(@ARGV);   shift(@ARGV);
    }

}

die "Cannot open/find weight file ($weightfile)\n" if ! -e $weightfile;

my @weight=();
open(IN,$weightfile);
chomp($weight=<IN>);
close(IN);
push @weight,split(/[ \t]+/,"1 $weight");
my @active=();
my @invertedactive=();

if ($activefields eq ""){
	for (my $i=1; $i<=$size; $i++){	$active[$i]=1; };
}else{
        @active=split(/,/,$activefields);
}

for (my $i=0; $i<=$size; $i++){	$invertedactive[$i]=0; };
for (my $i=0; $i<scalar(@active); $i++){	$invertedactive[$active[$i]]=1; };
my $j=0;
for (my $i=1; $i<=$size; $i++){	if (!$invertedactive[$i]){$notactive[$j]=$i; $j++}};

if ($debug>0){
	print STDERR "ORIGINAL SIZE: $size\n";
	print STDERR "ORIGINAL WEIGHTS: @weight\n";
	print STDERR "ORIGINAL ACTIVE: @active\n";
	print STDERR "ORIGINAL NOTACTIVE: @notactive\n";
	print STDERR "ORIGINAL INVERTEDACTIVE: @invertedactive\n";
}

while(chomp($_=<STDIN>)){
	my @field=(0,split(/[ \t]+/,$_));

	my $notactivedweightedsum=0.0;
	my $j;
	for (my $i=0; $i<scalar(@notactive); $i++){
		$j=$notactive[$i];
		$notactivedweightedsum+=($weight[$j]*$field[$j]);
		printf STDERR "notactive -> i:$i j:$j -> $weight[$j] - $field[$j] -> $notactivedweightedsum\n" if $debug>0;
	};

	printf STDOUT "%.3f",$notactivedweightedsum;
	printf STDERR "sum not active features: %.3f\n",$notactivedweightedsum if $debug>0;
	for (my $i=0; $i<scalar(@active); $i++){
		print STDOUT " $field[$active[$i]]";
		printf STDERR "active -> i:$i j:$active[$i] -> $field[$active[$i]]\n" if $debug>0;
	};
	for (my $i=scalar(@active)+scalar(@notactive)+1; $i< scalar(@field); $i++){
		print STDOUT " $field[$i]";
		printf STDERR "extra -> i:$i -> $field[$i]\n" if $debug>0;
	};
	print STDOUT "\n";
}
